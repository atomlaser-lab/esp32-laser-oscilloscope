/* ESP8266 Remote Laser Oscilloscope and Monitor for
MOGLabs Diode Laser Controller

Misc notes:
- What happens if the unsigned long timestamps become too long for
  JSON/JavaScript on the clientside to handle them with precision?
*/

#include <Arduino.h>     // Framework
#include <ESP8266WiFi.h> // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
#include <LittleFS.h>    // File-system
// https://github.com/me-no-dev/ESPAsyncWebServer
#include <ArduinoJSON.h> // https://arduinojson.org/
#include <AsyncJson.h>   // For handling JSON packets
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Pin mappings
const int LED_PIN = 2; // For LED only, LOW is on and HIGH is off.
const int TRIG_PIN = D5;
const int SLOW_LOCK_PIN = D1;
const int FAST_LOCK_PIN = D2;

// Trigger state (Note 'HIGH' and 'LOW' are just aliases for '1' and '0')
bool trig = LOW;

/* There are two restrictions on the sample resolution and duration:
  1. Calling analogRead too frequently on the ESP8266 interferes with its
  WiFi connection (known bug, see e.g. https://arduino.stackexchange.com/questions/56399/cant-add-more-code-to-loop-when-use-server-handleclient).

  2. The ESP has limited memory

Also note that all *external inputs* (config file, client) for resolution and
duration are in milliseconds, but internally microseconds are used as the
millis() function only goes to millisecond precision.
*/
const int buffer_size = 4096; // TODO: choose buffer size.
uint8_t input_buffer[buffer_size];

unsigned int time_resolution = 2000;  // Microseconds
unsigned int next_resolution = 2000;  // Pending resolution.
unsigned int sample_duration = 40000; // Microseconds

uint64_t packet_start;  // Packet start time (microseconds);
uint64_t elapsed = 0;   // Since start of packet (microseconds)
unsigned int N = 0;     // Datapoint in current packet
uint64_t trig_time = 0; // Most recent trigger time (microseconds)

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

/* // Handle a WebSocket message
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  return;
}*/

// Handler for WebSocket events
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    // handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
    // I'm fairly sure this exhausts all possible event types.
  }
}

void setSampleSettings(double resolution, double duration) {
  /* Duration takes effect immediately but resolution waits until a
  new packet. Note arguments are in milliseconds but next_resolution
  and sample_duration are in microseconds.*/
  next_resolution = max((int)(resolution * 1000), 100); //Hard limit 0.1ms res
  sample_duration = (int) min(max(duration * 1000, 2.0 * next_resolution), 200000000.0);
  /*Max 20sec and minimum *twice* the *current* resolution*/
  Serial.println(" Sampling settings set to:");
  Serial.printf("  Resolution: %.1f ms\n", next_resolution / 1000.0);
  Serial.printf("  Duration: %.1f ms\n", sample_duration / 1000.0);
  Serial.println();
}

void settingsHandler(AsyncWebServerRequest *request, JsonVariant &json) {
  const JsonObject &jsonObj = json.as<JsonObject>();
  setSampleSettings(jsonObj["resolution"], jsonObj["duration"]);
  request->redirect("/get_sample_settings");
};

// Setup code, run once upon restart.
void setup() {
  ESP.eraseConfig(); // Erase previous WiFi config (unsure if necessary)

  // Pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, INPUT);
  pinMode(SLOW_LOCK_PIN, OUTPUT);
  pinMode(FAST_LOCK_PIN, OUTPUT);
  pinMode(A0, INPUT);

  // Initial pin outputs
  digitalWrite(SLOW_LOCK_PIN, LOW); // Must begin low
  digitalWrite(FAST_LOCK_PIN, LOW);

  // Indicate that board is running
  digitalWrite(LED_PIN, LOW); // Inverted: LOW is on.

  // Serial port for debugging purposes
  Serial.begin(115200); // 115200 is baud rate (i.e. Serial communication rate)

  // Begin filesystem
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS.");
  }

  // Load config from JSON file.
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Unable to access configuration file.");
  }
  StaticJsonDocument<512> configDoc;
  DeserializationError error = deserializeJson(configDoc, configFile);
  if (error) {
    Serial.println("Invalid configuration file.");
  }

  const char *name = configDoc["name"]; // Display name for laser. Defaults to NULL

  // Default sampling settings
  const double configRes = configDoc["default_resolution"];
  const double configDur = configDoc["default_duration"];
  setSampleSettings(
      configRes ? configRes : 2.0,
      configDur ? configDur : 60.0);

  // WiFi details
  const bool host = configDoc["host"]; // Whether to host own network (mainly for testing). If not found in the config file, this value will default to zero, i.e. false.

  // Network defaults
  IPAddress local_ip(192, 168, 1, 1); // How we appear to other devices on the network
  IPAddress gateway(192, 168, 1, 1);  /* local device through which connections
  external to the local network are routed (i.e. in this case, the board).
  These connections will fail, because the board isn't connected to the
  internet.*/
  IPAddress subnet(255, 255, 255, 0); // Allows extraction of network address from local address

  const char *ip_str = configDoc["default_ip"];
  if (ip_str) {
    local_ip.fromString(ip_str);
    // gateway.fromString(ip_str);
  }

  /* Connect to Wi-Fi. Modes:
  - Station (STA): connects to existing WiFi.
  - Access Point (AP): hosts its own network. By default limited to 4
    connected devices; this can be increased up to 8.
  */

  WiFi.disconnect();
  // WiFi.setAutoConnect(false);

  if (host) { // Set up own network
    // wifi_set_phy_mode(PHY_MODE_11G);
    const char *host_ssid = configDoc["host_ssid"];
    const char *host_password = configDoc["host_password"];
    if (!(host_ssid && host_password)) {
      Serial.println("Missing host_ssid or host_password.");
    }
    if (strlen(host_password) < 8) {
      Serial.println("Warning: WiFi.softAP (hosting) will fail if host_password has fewer than 8 characters.");
    }
    int host_channel = configDoc["host_channel"]; // Specifies the WiFi frequency band. 1-11 is fairly safe in most countries. Default is 1.
    if (!host_channel) {
      host_channel = 1;
    }
    const int max_connection = 1;
    /* Prevents multiple clients. From the documentation: "Once the max number
    has been reached, any other station that wants to connect will be forced
    to wait until an already connected station disconnects."*/
    WiFi.mode(WIFI_AP);
    Serial.print("Setting soft-AP configuration ... ");
    Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Ready" : "Failed!");
    Serial.print("Setting soft-AP ... ");
    Serial.println(WiFi.softAP(host_ssid, host_password, host_channel, false, max_connection) ? "Ready" : "Failed!");
    // Pretty sure this uses WPA2 security protocol (i.e. the most common).
    Serial.print("Soft-AP IP address = ");
    Serial.println(WiFi.softAPIP());

  } else { // Connect to existing network
    const char *ssid = configDoc["ssid"];
    const char *password = configDoc["password"]; // NULL if missing
    if (!ssid) { // If password missing, will be assumed no password.
      Serial.println("Missing WiFi ssid.");
    }
    WiFi.mode(WIFI_STA);
    if (!WiFi.config(local_ip, gateway, subnet)) {
      Serial.println("IP config failed.");
    }
    if (password == NULL) { /*strcmp(password, "") == 0*/
      WiFi.begin(ssid);
    } else {
      WiFi.begin(ssid, password);
    }
    Serial.print("Connecting to WiFi.");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    // Print ESP Local IP Address
    Serial.print("\nConnected. Local IP Address: ");
    Serial.println(WiFi.localIP());

    WiFi.setAutoReconnect(true);
  }

  // Serve webpage and handle Websocket events
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  /*
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html", false);
    // Or: request->redirect("/index.html"); });
  });*/

  // Add WebSocket handler
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Handle commands (square bracket notation begins an anonymous function)
  server.on("/status", HTTP_GET, [&name](AsyncWebServerRequest *request) {
    StaticJsonDocument<200> statusDoc;
    statusDoc["name"] = "TestName"; //&name;
    statusDoc["slow"] = (bool)digitalRead(SLOW_LOCK_PIN);
    statusDoc["fast"] = (bool)digitalRead(FAST_LOCK_PIN);
    String statusStr = "";
    serializeJson(statusDoc, statusStr);
    request->send(200, "text/plain", statusStr);
  });
  server.on("/enable_slow", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(SLOW_LOCK_PIN, HIGH);
    request->send(200);
  });
  server.on("/enable_fast", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(FAST_LOCK_PIN, HIGH);
    request->send(200);
  });
  server.on("/disable_fast", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(FAST_LOCK_PIN, LOW);
    request->send(200);
  });
  server.on("/disable_slow", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(SLOW_LOCK_PIN, LOW);
    request->send(200);
  });
  server.on("/get_sample_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    /* Tell client what the current sampling settings are */
    StaticJsonDocument<200> settingsDoc;
    settingsDoc["duration"] = (double)(sample_duration / 1000.0);
    settingsDoc["resolution"] = (double)(time_resolution / 1000.0);
    String settingsStr = "";
    serializeJson(settingsDoc, settingsStr);
    request->send(200, "text/plain", settingsStr);
  });
  // Handler for receiving sampling settings as JSON.
  /* There are two possible methods for this. See https://github.com/me-no-dev/ESPAsyncWebServer/issues/195 */

  server.addHandler(new AsyncCallbackJsonWebHandler(
      "/set_sample_settings",
      settingsHandler, 1024)); // Last argument is max JSON bytesize
  /* Alternative pattern: handle chunked reception of body manually
  server.on("/set_sample_settings", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // Request handler code here. This will be run AFTER all body chunks have been received. The bodyHandler below collects the entire body into request->_tempObject, which you can use here to access the body.
      //e.g.
      DynamicJsonBuffer jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject((const char *)(request->_tempObject));
      if (root.success()) {
          if (root.containsKey("command")) {
            Serial.println(root["command"].asString()); // Hello
          }
        }
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) { // Body chunk handler
      // data is chunk bytes, len its bytelength, index which chunk it is, and total the total expected number of bytes.
      if (total > 0 && request->_tempObject == NULL && total < 10240) { // you may use your own size instead of 10240
      request->_tempObject = malloc(total); // Create temporary storage
      }
      if (request->_tempObject != NULL) {
        // Add chunk to temporary storage
        memcpy((uint8_t*)(request->_tempObject) + index, bodyData, bodyLen);
      }
    }
  });
  */

  // Other pages fail.
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.println("Requested page not found.");
    request->send(404);
  });

  // Start server
  server.begin();
  packet_start = micros64(); // Will be a slight delay for the first packet.
}

// Run repeatedly after setup()
void loop() {
  if (ws.count() == 0) { //Nobody's listening, wait.
    // Otherwise will not allow the page to load.
    packet_start = micros64();
    N=0;
    return;
  }
  // Note micros64 will eventually roll over.
  elapsed = micros64() - packet_start;
  // Check trigger
  if (digitalRead(TRIG_PIN) != trig) {
    trig = !trig;
    if (trig && N && (trig_time == 0)) {
      trig_time = packet_start + elapsed;
      /* Trigger = rising edge (LOW->HIGH) on TRIG_PIN. For now, the trigger
      is ignored if it occurs between data packets, or has already occurred
      in the same packet; the client should handle this gracefully.*/
    }
  }
  if (elapsed >= time_resolution * N) {
    // Record a new measurement
    input_buffer[N++] = (uint8_t)(analogRead(A0) / 4);
    // Check if finished packet
    if ((elapsed >= sample_duration) || N >= buffer_size) {
      // Send data
      ws.cleanupClients();  // Release improperly-closed connections
      // Send metadata
      StaticJsonDocument<200> heraldDoc; // Meta-data to precede measurement packet
      heraldDoc["start"] = (double)(packet_start / 1000.0);
      heraldDoc["elapsed"] = (double)(elapsed / 1000.0);
      heraldDoc["trigTime"] = trig_time / 1000.0; // 0 if no trigger occurred.
      String heraldStr;
      serializeJson(heraldDoc, heraldStr);
      ws.textAll(heraldStr);
      // Send measurement packet
      ws.binaryAll(input_buffer, N); /* Final argument is number of
        BYTES to send */
      delay(10); // analogRead interferes with the WiFi, so give it a break.
      // Start new packet
      N = 0;
      trig_time = 0;
      time_resolution = next_resolution;
      packet_start = micros64();
    }
  }
  /* The ESP8266 runs background processes
  including WiFi on a separate context, and (unlike the Arduino)
  we must therefore yield at least occasionally to allow background processes to run. Both delay(0) and yield() do this. For short loops like this one though, it is unnecessary. */
}