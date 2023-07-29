/* ESP32 Remote Monitor and locking control for MOGLabs Diode Laser Controller

(c) Patrick Gleeson 2022

General notes:
- See TODO comments scattered throughout.
- HIGH and LOW are aliases for 1 and 0, respectively.
- TODO: track, report and client-command of LASER_ON and HOLD status.
*/

#include <Arduino.h>           // Framework
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

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


//ESP32 has 512kbit of RAM
/* Settings and storage for data sampling. Resolution and duration are specified externally (client, config file) in milliseconds, but are handled
internally in microseconds.*/
#define MIN_RES 100               // microseconds. int.
#define MIN_DURATION 30000.0      // Else ESP32 can become unresponsive. Float.
#define MAX_DURATION 20000000.0   // 20 sec. (float)
#define CHANNEL_BUFFER_SIZE 1000 // 16KB per channel, enough for 10s data at 1ms resolution.
/* A new packet will be started if the buffer gets full, regardless of sampling
settings*/
uint8_t input_buffer[2 * CHANNEL_BUFFER_SIZE]; // Alternating PD, error and time samples.

unsigned int time_resolution = 2000;  // Microseconds
unsigned int next_resolution = 2000;  // For next packet.
unsigned int sample_duration = 40000; // Microseconds

uint64_t packet_start;  // Packet start time (microseconds);
uint64_t elapsed = 0;   // Since start of packet (microseconds)
unsigned int N = 0;     // Datapoint index in current packet
uint64_t trig_time = 0; // Most recent trigger time (microseconds)
int trig_last = 0;     // What the last trigger read was


// Send value to external MCP4821 DAC.
// Similar to https://cyberblogspot.com/how-to-use-mcp4921-dac-with-arduino/
// but the bytes sent have been adjusted so it works.
void set12BitDAC(uint16_t value) {
  Serial.printf("ADC: %d\n", value);
  uint16_t data = 0b0001000000000000 | value; // could go directly to secondByte
  // 4 config bits + most significant 4 value bits
  uint8_t firstByte = 0b00010000 | ((uint8_t)((value & 0xFFF) >> 8));
  // remaining 8 value bits
  uint8_t secondByte = (uint8_t) data;
  Serial.println(firstByte, BIN);
  Serial.println(secondByte, BIN);
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

void data_in(char* input){
  //first check for laser controlls
  //Serial.println(input);
  if(strcmp (input,"enable_slow")==0) {
    digitalWrite(SLOW_LOCK_PIN, HIGH);
    Serial.println("locking slow");
    return;
  }
    if(strcmp (input,"enable_fast")==0) {
    digitalWrite(FAST_LOCK_PIN, HIGH);
    Serial.println("locking fast");
    return;
  }
    if(strcmp (input,"disable_fast")==0) {
    digitalWrite(FAST_LOCK_PIN, LOW);
    Serial.println("unlocking fast");
    return;
  }
    if(strcmp (input,"disable_slow")==0) {
    digitalWrite(SLOW_LOCK_PIN, LOW);
    Serial.println("unlocking slow");
    return;
  }
  //now if possible convert to a number and then send to ADC
  uint16_t forADC = atoi(input);
  set12BitDAC(forADC);


}
// Specify maximum UDP packet size
#define MAX_PACKET_SIZE 512

// Specify UDP port to listen on
unsigned int localPort = 9999;

// Create data array for storing the received UDP packet data payload
char packetData[MAX_PACKET_SIZE];

WiFiUDP Udp;

//#################################SETUP#############################################//

// Setup code, is run once upon restart.
void setup() {
  // dac.begin(VSPI_CS);
  //  Serial port for debugging purposes
  Serial.begin(115200); // 115200 is baud rate (symbol communication rate)

  //clear out the packet data array
  for( int i = 0; i < sizeof(packetData);  ++i )
   packetData[i] = (char)0;

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
  //attachInterrupt(TRIG_PIN, onTrig, RISING); // Record any triggers


  /* Connect to Wi-Fi. Modes:
  - Station (STA): connects to existing WiFi.
  - Access Point (AP): hosts its own network. By default limited to 4
    connected devices; this can be increased up to 8. Below, we limit it to 1.
  */

  WiFi.disconnect();
  // WiFi.setAutoConnect(false);

  // Connect to existing network
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
  Serial.printf("Connecting to WiFi: %s.",ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  // Print ESP Local IP Address
  Serial.print("\nConnected. Local IP Address: ");
  Serial.println(WiFi.localIP());

  WiFi.setAutoReconnect(true);

  //start UDP listen
  Udp.begin(localPort);

}


//define a bit of stuff for in the loop
int trig_now;
bool measure;
bool send_data;
uint16_t trig_index;
int time_res = 1; //us - for now, lets hard code this
bool active;
int i;
uint16_t Photo_diode_ADC;
uint16_t Error_signal_ADC;
uint64_t time_start;
//#################################LOOP#############################################//

// Is run repeatedly after setup()
void loop() {
  //Serial.print("start\n");

  /*
  elapsed = micros() - packet_start;
  if (elapsed < 0) {
    // TODO: handle micros() rollover here.
  }
  */

  //at this point we want to measure a trace.
  //read the trigger input
  trig_now = digitalRead(TRIG_PIN);
  measure = false;
  send_data = false;

  if (!send_data){
    //start the measurment
    measure = true;
    trig_last = trig_now;
    delayMicroseconds(10);
    //Serial.print("starting measure\n");
  }
  trig_index = 0;
  N=0;
  packet_start = micros();
  while (measure) {

    //Start recording data for a new point
    active = true;
    i=0;
    Photo_diode_ADC = 0;
    Error_signal_ADC = 0;
    time_start = micros();
    while (active){
      //Check the trigger situation
      trig_now = digitalRead(TRIG_PIN);
      if (N==200){
        //center of scan reaced. Mark index
        trig_index = N;
        //Serial.printf("Trig N = %d\n",trig_index);
      }
      if (N==400){
        //end of scan reached. End measure
        //Serial.printf("End Measure N = %d\n",N);
        measure = false;
        send_data = true; //request to send data
        delayMicroseconds(10);
        break;
      }
      if(N>CHANNEL_BUFFER_SIZE){
        measure = false;
        send_data = true; //request to send data
        delayMicroseconds(10);
        break;
      }
      //average ADC down while less than time_res
      Photo_diode_ADC = ((uint16_t)analogRead(PD_INPUT_PIN) + Photo_diode_ADC*i)/(i+1);   // Photodiode
      Error_signal_ADC = ((uint16_t)analogRead(LOCK_INPUT_PIN) + Error_signal_ADC*i)/(i+1);   // Error signal
      if (micros()-time_start>=time_res){
        //Serial.printf("i = %d\n",i);
        //measurment point done. Store value and start again
        input_buffer[2 * N] = (uint8_t)(Photo_diode_ADC >> 4);           // Photodiode
        input_buffer[2 * N + 1] = (uint8_t)(Error_signal_ADC >> 4); // Error signal
        //TODO: actualy record the times as well instead of assuming
        active = false; //break out of the measurment
      }
      i++;
      trig_last = trig_now; //set what the last trigger measure was.
    }
    N++;
    //Serial.printf("Data N = %d\n",N);

  }
  //Serial.printf("Trig N = %d\n",trig_index);
  trig_last = trig_now; //set what the last trigger measure was.
  elapsed = micros() - packet_start;


  if (send_data){
    //broadcast the data over UDP (we realy dont care if we miss a few packets, we just want the data fast.)
    //Serial.printf("Trig N = %d\n",trig_index);
    //broadcast to 192.169.1.255:9001
    Udp.beginPacket(IPAddress(192,168,1,255), 9001);
    //packet structure first unit16 is remaining number of bytes
    uint8_t temp_array[2];
    uint16_t data_lengnth = N*2;
    temp_array[0]=data_lengnth & 0xff;
    temp_array[1]=(data_lengnth >> 8);
    Udp.write(temp_array[0]);
    Udp.write(temp_array[1]);

    //2nd uint16 is the trigger index
    temp_array[0]=trig_index & 0xff;
    temp_array[1]=(trig_index >> 8);
    Udp.write(temp_array[0]);
    Udp.write(temp_array[1]);
    //the remaining is uint8 interleaved chan1 and 2
    for (int i =0; i < 2*N; i++)
    {
      Udp.write(input_buffer[i]);
    }
    Udp.endPacket();   
    //Serial.printf("sent bytes: %d\n",(N+1)*2);
    

    // Process received packet
    int packetSize = Udp.parsePacket();

    if (packetSize > MAX_PACKET_SIZE)
    {
      packetSize = MAX_PACKET_SIZE;
    }
    // If a packet was received
    if (packetSize)
    {
      // Read the received UDP packet data
      Udp.read(packetData,MAX_PACKET_SIZE);
      //Serial.println(packetData);
      //check that this starts with a # otherwise ignore it
      if (packetData[0]!='#')
        return;

      //things to do, set ADC or lock.unlock fast or slow
      data_in(&packetData[1]);

    } 
    //clear out the packet
    packetData[0] = (char)0;
    // Start new packet
    N = 0;
    trig_time = 0;
    time_resolution = next_resolution;
    packet_start = micros();
    send_data = false;
    trig_now=0;
    trig_last=0;
    delay(10);
  }
}