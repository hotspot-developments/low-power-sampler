// This example is used to allow the monitoring of power consumption in each of the three modes
// in which the low-power sampler operates:
// - Deepsleep
// - Active but with no WiFi
// - Fully active with WiFi.
// No sensor readings are taken in this example.
// To give the WiFi something to do - an ntp server is used to synchronize the sampler.
// The configuration ensures that there is a prolonged period in each mode (at least 15 seconds)
// so that a multimeter can be employed to check power consumption.
// Example figures I got for a NodeMCU ESP8266 development board:
// - Deepsleep: 9mA
// - Active no Wifi: 24mA
// - Active with Wifi: 78mA
// Configuration: take a measurement every 60 seconds and transmit every other one.
// There is a 15 second busyish period in the measurement and transmit callbacks to allow for metering.
// After the initial setup the programs should follow the following pattern
// 15 seconds active no wifi
// 45 seconds deepsleep
// 30 seconds active with wifi
// 30 seconds deepsleep
// 15 seconds active no wifi
// 45 seconds deepsleep
// 30 seconds active with wifi
// 30 seconds deepsleep
// etc..etc..


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#define SENSOR_PIN D6
#else
#include <WiFi.h>
#define SENSOR_PIN 34
#endif
#include <WiFiUdp.h>

#include "private.h"
#include "Configuration.h"
#include "Sampler.h"

#define MSG_SIZE 250
#define NTP_PACKET_SIZE 48

#define VERSION 100
#define MS_DELAY_FOR_WIFI_CONNECTION  500
#define MS_DELAY_FOR_NTP_RESPONSE     100
#define MS_DELAY_FOR_BUSY_WAIT         10
#define MS_WAIT_TIME_FOR_MESSAGES    5000
#define MS_WAIT_TIME_FOR_WIFI        5000

WiFiClient espClient;
WiFiUDP udpClient;
Configuration config;
Sampler sampler(config);
char msg[MSG_SIZE];                   // buffer to hold outgoing debug/mqtt messages.
byte NTPBuffer[NTP_PACKET_SIZE];      // buffer to hold incoming and outgoing ntp packets.
IPAddress timeServerIP;               // IP address of NTP server.

boolean setupWifi() {
  Serial.printf("\nConnecting to: %s ..", WIFI_SSID);
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

  int32_t waitTime = MS_WAIT_TIME_FOR_WIFI; 
  boolean connected = WiFi.status() == WL_CONNECTED;
  while (waitTime > 0 && !connected) {
    waitTime -= MS_DELAY_FOR_WIFI_CONNECTION;
    delay(MS_DELAY_FOR_WIFI_CONNECTION);
    Serial.print(".");
    connected = WiFi.status() == WL_CONNECTED;
  }
  if (connected) {
    Serial.print("WiFi connected at: ");
    Serial.println(WiFi.localIP());
  }
  return connected;
}

boolean setupNtp() {
  if (WiFi.status() != WL_CONNECTED) return false;
  Serial.println("Starting UDP");
  udpClient.begin(123);                          // Start listening for UDP messages on port 123

  boolean ntpServerFound = WiFi.hostByName(NTP_SERVER_NAME, timeServerIP);
  if(ntpServerFound) {  
    memset(NTPBuffer, 0, NTP_PACKET_SIZE);       // set all bytes in the buffer to 0
    NTPBuffer[0] = 0b11100011;                   // Initialize values needed to form NTP request: LI, Version, Mode
    udpClient.beginPacket(timeServerIP, 123);    // send a packet requesting a timestamp:NTP requests are to port 123
    udpClient.write(NTPBuffer, NTP_PACKET_SIZE);
    udpClient.endPacket();    
    Serial.println("NTP message sent");
  } else {
    Serial.println("DNS lookup failed for NTP");
  }
  return ntpServerFound;
}

void ntpReceiveMsg(byte* ntpBuffer) {
  unsigned long  NTPTime = 0 ;
  NTPTime |= (unsigned long) (ntpBuffer[40] << 24);
  NTPTime |= (unsigned long)(ntpBuffer[41] << 16);
  NTPTime |= (unsigned long)(ntpBuffer[42] << 8); 
  NTPTime |= (unsigned long) ntpBuffer[43]; 
  sampler.synchronise(NTPTime); 
  Serial.printf("NTP response received, time: %lu\n", NTPTime);
}

void waitForResponse() {
  int32_t waitTime = MS_WAIT_TIME_FOR_MESSAGES;
  boolean ntpReceived = false;
  while(waitTime > 0 && !ntpReceived) {
    waitTime -= MS_DELAY_FOR_NTP_RESPONSE;
    delay(MS_DELAY_FOR_NTP_RESPONSE);
    if (!ntpReceived) {
      ntpReceived = udpClient.parsePacket() >= NTP_PACKET_SIZE;
      if (ntpReceived) {
        udpClient.read(NTPBuffer, NTP_PACKET_SIZE);
        ntpReceiveMsg(NTPBuffer);
      }
    }
  }
  Serial.println("Finished waiting for responses.");
}

void busyWaitMsFor(unsigned long n) {
  unsigned long initial = millis();
  while( (millis() - initial) < n) delay(MS_DELAY_FOR_BUSY_WAIT);
}

uint16_t takeMeasurement(uint16_t * sample, uint32_t n) {
  uint32_t count = config.getCounter();
  Serial.printf("\nTaking measurement %s wifi... ",count%2==0?"no":"with" );
  busyWaitMsFor(15000);
  Serial.printf("\nDone measurement. %s", count%2==0?"Going to deepsleep\n":"");
  return 1;
}

void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nCommunicating with base... ");
  setupWifi();
  setupNtp();
  waitForResponse();
  busyWaitMsFor(15000);
  Serial.printf("Done transmitting.\nGoing to deepsleep\n");
}

// ===============  Arduino Pattern ===================================================
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);            // Switch off the LED
  digitalWrite(BUILTIN_LED,HIGH);
  Serial.begin(115200);
  config.setParameters(60000,0,1,2);  // Default parameters - used first time round.
  config.setVersion(VERSION);
  Serial.printf("\nSetup: configuration taken from %s.", config.checkMemory()?"memory":"defaults");
  sampler.setup();
  sampler.onTakeMeasurement(takeMeasurement);
  sampler.onTransmit(transmit);
  config.populateStatusMsg(msg, MSG_SIZE);
  Serial.println(msg);
}

void loop() {
  sampler.loop();
}
