/* ESP32 Remote Monitor and locking control for MOGLabs Diode Laser Controller

(c) Patrick Gleeson 2022

General notes:
- See TODO comments scattered throughout.
- HIGH and LOW are aliases for 1 and 0, respectively.
- TODO: track, report and client-command of LASER_ON and HOLD status.
*/

#include <Arduino.h>           // Framework
#include <ArduinoJSON.h>       // https://arduinojson.org/
#include <AsyncJson.h>         // For handling JSON packets
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <LittleFS.h>          // File-system
#include <SPI.h>
#include <WiFi.h>

#include <secrets.h> // in scr make a seacrets.h file and define the SSID and password MAke sure this is part of .gitignore


/* Pin mappings. With the Arduino framework, the pin number is the
ESP32 GPIO number (at least by default in PlatformIO)*/
#define PD_INPUT_PIN 39
#define LOCK_INPUT_PIN 36
#define TRIG_PIN 34

#define LED_PIN 2

#define SLOW_LOCK_PIN 14
#define FAST_LOCK_PIN 4
#define LASER_ON_PIN 21
#define HOLD_PIN 27

#define LDACN_PIN 22
#define VSPI_MOSI 23
#define VSPI_SCK 18
#define VSPI_CS 5
#define VSPI_MISO 19 // Not used
SPIClass *vspi = NULL; // Initialised in setup(). For DAC control.

/* Tracking the output state by reading the appropriate output pin
values is unreliable, so record it explicitly: */
bool slow_lock = false;
bool fast_lock = false;
bool hold = false;
bool laser_on = false;

/* Settings and storage for data sampling. Resolution and duration are specified externally (client, config file) in milliseconds, but are handled
internally in microseconds.*/
#define MIN_RES 100               // microseconds. int.
#define MIN_DURATION 30000.0      // Else ESP32 can become unresponsive. Float.
#define MAX_DURATION 20000000.0   // 20 sec. (float)
#define CHANNEL_BUFFER_SIZE 16384 // 16KB per channel, enough for 10s data at 1ms resolution.
/* A new packet will be started if the buffer gets full, regardless of sampling
settings*/
uint8_t input_buffer[2 * CHANNEL_BUFFER_SIZE]; // Alternating PD & error samples.

unsigned int time_resolution = 2000;  // Microseconds
unsigned int next_resolution = 2000;  // For next packet.
unsigned int sample_duration = 40000; // Microseconds

uint64_t packet_start;  // Packet start time (microseconds);
uint64_t elapsed = 0;   // Since start of packet (microseconds)
unsigned int N = 0;     // Datapoint index in current packet
uint64_t trig_time = 0; // Most recent trigger time (microseconds)

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Send value to external MCP4821 DAC.
// Similar to https://cyberblogspot.com/how-to-use-mcp4921-dac-with-arduino/
// but the bytes sent have been adjusted so it works.
void set12BitDAC(uint16_t value) {
  Serial.println(value);
  uint16_t data = 0b0001000000000000 | value; // could go directly to secondByte
  // 4 config bits + most significant 4 value bits
  uint8_t firstByte = 0b00010000 | ((uint8_t)((value & 0xFFF) >> 8));
  // remaining 8 value bits
  uint8_t secondByte = (uint8_t) data;
  //Serial.println(firstByte, BIN);
  //Serial.println(secondByte, BIN);
  digitalWrite(vspi->pinSS(), LOW); // VSPI_CS; pull low to prep other end for transfer
  /* Use SPI clock rate 16MHz (MCP4821 supports up to 20MHz). We use SPI_MODE0,
  where the clock is LOW when idle and data is transferred on the rising edge.*/
  vspi->beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  vspi->transfer(firstByte);
  vspi->transfer(secondByte);
  vspi->endTransaction();
  // pull ss (CS) high to signify end of data transfer
  digitalWrite(vspi->pinSS(), HIGH); 
  /* Data value will be immediately translated to VOUT, as LDACn is set
   permanently LOW in setup().*/
}

/* Handle an incoming WebSocket message. These are expected to be 12-bit
level values for the external MCP4821 DAC. */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_BINARY) {
    /* Received a single binary packet. This is expected to contain two bytes,
    forming a 16 bit integer (most-significant byte first). The 12
    least-significant bits are our DAC value. */
    set12BitDAC((((uint16_t)data[0]) << 8) | ((uint16_t)data[1]));
    // Obsolete: dacWrite(pin, 8-bit value) for ESP32's inbuilt 8-bit DAC.
  }
}

// Handler for WebSocket events
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    // Could initiate sleep here.
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
    // I'm fairly sure this exhausts all possible event types.
  }
}

// Update
void setSampleSettings(double resolution, double duration) {
  /* Arguments are in milliseconds and converted to microseconds for
  next_resolution and sample_duration. Duration takes effect immediately but
  resolution waits until a new packet, so that samples remain evenly spaced. */
  next_resolution = max((int)(resolution * 1000), MIN_RES); // Hard limit 0.1ms res
  sample_duration = (int)min(max(
                                 max(duration * 1000, 2.0 * next_resolution), MIN_DURATION),
                             MAX_DURATION);
  /* Hard duration min 30ms and max 20sec, else ESP can become unresponsive.
  Also enforce duration > 2*resolution, to ensure >1 samples. */
  Serial.println(" Sampling settings set to:");
  Serial.printf("  Resolution: %.1f ms\n", next_resolution / 1000.0);
  Serial.printf("  Duration: %.1f ms\n", sample_duration / 1000.0);
  Serial.println();
}

// Handle a HTTP request to update sampling settings
void settingsHandler(AsyncWebServerRequest *request, JsonVariant &json) {
  const JsonObject &jsonObj = json.as<JsonObject>();
  setSampleSettings(jsonObj["resolution"], jsonObj["duration"]);
  request->redirect("/get_sample_settings");
}

/* Record the time of a trigger interrupt. Interrupts need to be in IRAM for
fast access (hence IRAM_ATTR). */
void IRAM_ATTR onTrig() {
  trig_time = micros();
}


//#################################SETUP#############################################//

// Setup code, is run once upon restart.
void setup() {
  // dac.begin(VSPI_CS);
  //  Serial port for debugging purposes
  Serial.begin(115200); // 115200 is baud rate (symbol communication rate)

  // Output pins
  pinMode(LED_PIN, OUTPUT); // ESP32 devkit onboard LED
  pinMode(SLOW_LOCK_PIN, OUTPUT);
  pinMode(FAST_LOCK_PIN, OUTPUT);
  pinMode(LASER_ON_PIN, OUTPUT);
  pinMode(HOLD_PIN, OUTPUT);
  pinMode(LDACN_PIN, OUTPUT);

  vspi = new SPIClass(VSPI);
  vspi->begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_CS); // SCK, MISO, MOSI, SS
  pinMode(vspi->pinSS(), OUTPUT);

  /* SPI API info: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html and links therein */

  // Initial output values
  digitalWrite(SLOW_LOCK_PIN, LOW);
  digitalWrite(FAST_LOCK_PIN, LOW);
  digitalWrite(LASER_ON_PIN, LOW);
  digitalWrite(HOLD_PIN, LOW);
  digitalWrite(LDACN_PIN, LOW); // So DAC input is automatically applied.
  // LDACN control is a likely 
  // Laser will not turn on if SLOW is HIGH.

  digitalWrite(LED_PIN, LOW); // Indicate that board is running.
  // LED pin is inverted: LOW is on.

  // Input pins
  pinMode(PD_INPUT_PIN, INPUT);   // Photodiode channel.
  pinMode(LOCK_INPUT_PIN, INPUT); // Error signal.
  pinMode(TRIG_PIN, INPUT);
  attachInterrupt(TRIG_PIN, onTrig, RISING); // Record any triggers

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

  static char name[100];
  strcpy(name, configDoc["name"]); // Laser display name (Defaults to NULL)

  // Default sampling settings
  const double configRes = configDoc["default_resolution"];
  const double configDur = configDoc["default_duration"];
  setSampleSettings(
      configRes ? configRes : 2.0,
      configDur ? configDur : 60.0);

  // WiFi details
  const bool host = configDoc["host"]; /* Whether to host own network (mainly
  for testing). If not found in the config file, this value will default to
  false (zero). */

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
    gateway.fromString(ip_str);
  }

  /* Connect to Wi-Fi. Modes:
  - Station (STA): connects to existing WiFi.
  - Access Point (AP): hosts its own network. By default limited to 4
    connected devices; this can be increased up to 8. Below, we limit it to 1.
  */

  WiFi.disconnect();
  // WiFi.setAutoConnect(false);

  if (host) { // Set up own network
    // wifi_set_phy_mode(PHY_MODE_11G);
    //get these from the secrets.h file
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
    const char *ssid = SECRET_WIFI_SSID;   // set these in secrets.h
    const char *password = SECRET_WIFI_PASSWORD; // NULL if missing
    if (!ssid) {                                  // If password missing, will be assumed no password.
      Serial.println("Missing WiFi ssid.");
    }
    WiFi.mode(WIFI_STA);
    /*if (!WiFi.config(local_ip, gateway, subnet)) {
      Serial.println("IP config failed.");
    }*/
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
  /* Alternative method:
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html", false);
    // Or: request->redirect("/index.html"); });
  });*/

  // Add WebSocket handler
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Handle commands (square bracket notation begins an anonymous function)
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<300> statusDoc;
    statusDoc["name"] = name; // name is static, so can be used in lambda func.
    statusDoc["slow"] = slow_lock;
    statusDoc["fast"] = fast_lock;
    /* Would be convenient to just read the state of the pins directly,
    but this is unreliable as they are set to OUTPUT mode.*/
    String statusStr = "";
    serializeJson(statusDoc, statusStr);
    request->send(200, "text/plain", statusStr);
  });
  server.on("/enable_slow", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(SLOW_LOCK_PIN, HIGH);
    slow_lock = true;
    request->send(200);
  });
  server.on("/enable_fast", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(FAST_LOCK_PIN, HIGH);
    fast_lock = true;
    request->send(200);
  });
  server.on("/disable_fast", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(FAST_LOCK_PIN, LOW);
    fast_lock = false;
    request->send(200);
  });
  server.on("/disable_slow", HTTP_POST, [](AsyncWebServerRequest *request) {
    digitalWrite(SLOW_LOCK_PIN, LOW);
    slow_lock = false;
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

  /* Handler for receiving sampling settings as JSON. Two possible methods
   for this (see https://github.com/me-no-dev/ESPAsyncWebServer/issues/195) */
  server.addHandler(new AsyncCallbackJsonWebHandler(
      "/set_sample_settings",
      settingsHandler, 1024)); // Last argument is max JSON bytesize

  /* Alternative pattern: handle chunked reception of body manually.
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

  // All other requests should fail.
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.println("Requested page not found.");
    request->send(404);
  });

  // Start server
  server.begin();
  packet_start = micros(); // Will be a slight delay before the first packet.
}

//#################################LOOP#############################################//

// Is run repeatedly after setup()
void loop() {
  //Serial.print("start\n");
  if (ws.count() == 0) { // ws.count() is number of connected websocket clients
    // Nobody's listening, wait. Otherwise will not allow the page to load.
    delay(10);
    packet_start = micros();
    N = 0;
    return;
  }

  elapsed = micros() - packet_start;
  if (elapsed < 0) {
    // TODO: handle micros() rollover here.
  }

  if (elapsed >= time_resolution * N) {
    // Record a new measurement (reducing 12-bit ESP32 ADC resolution to 1 byte)
    input_buffer[2 * N] = (uint8_t)(analogRead(PD_INPUT_PIN) >> 4);           // Photodiode
    input_buffer[2 * (N++) + 1] = (uint8_t)(analogRead(LOCK_INPUT_PIN) >> 4); // Error signal
    // Check if finished packet
    if ((elapsed >= sample_duration) || N >= CHANNEL_BUFFER_SIZE) {
      // Send data
      ws.cleanupClients(); // Release improperly-closed connections
      if (ws.availableForWriteAll()) {
        // Send metadata
        StaticJsonDocument<200> heraldDoc; // Meta-data to precede measurement packet
        heraldDoc["start"] = (double)(packet_start / 1000.0);
        heraldDoc["elapsed"] = (double)(elapsed / 1000.0);
        heraldDoc["trigTime"] = trig_time / 1000.0; // will be 0 if no trigger occurred.
        String heraldStr;
        serializeJson(heraldDoc, heraldStr);
        ws.textAll(heraldStr);
        // Send measurement packet
        ws.binaryAll(input_buffer, 2 * N);
        /* Final argument is number of BYTES to send.*/
      }
      // Start new packet
      N = 0;
      trig_time = 0;
      time_resolution = next_resolution;
      packet_start = micros();
    }
  }
}