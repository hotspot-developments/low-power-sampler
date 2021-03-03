// This example verfies that the WiFi is switched off - unless we are about to transmit.
// The Sampler is configured as follows:
//    Every 2 minutes, take 3 samples 15 seconds apart, this counts as 1 measurement
//    Every 3rd measurement, transmit to a server somewhere.
// The wifi on the ESP8266 chip should be disabled _except_ for the time we transmit.
// The only way I found to be sure it was switched off was to try connected to the Wifi
// in each callback anyway. The sequence of events should look like
// 00:00 take a sample
// 00:15 take a sample
// 00:30 take a sample - take a measurement
// 02:00 take a sample
// 02:15 take a sample
// 02:30 take a sample - take a measurement
// 04:00 take a sample
// 04:15 take a sample
// 04:30 take a sample - take a measurement - transmit
// 06:00 take a sample
// 06:15 take a sample
// 06:30 take a sample - take a measurement
// 08:00 take a sample
// 08:15 take a sample
// 08:30 take a sample - take a measurement
// 10:00 take a sample
// 10:15 take a sample
// 10:30 take a sample - take a measurement - transmit
// ... and repeat
// Its only the events that invoke transmit that have the Wifi enabled, which can be verified by following 
// the Serial debug statements.

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#define SENSOR_PIN D6
#else
#include <WiFi.h>
#define SENSOR_PIN 34
#endif

#include "private.h"
#include "Configuration.h"
#include "Sampler.h"

#define MSG_SIZE 250

#define VERSION 100
#define MS_DELAY_FOR_WIFI_CONNECTION  500
#define MS_WAIT_TIME_FOR_WIFI       10000


WiFiClient espClient;
Configuration config;
Sampler sampler(config);
char msg[MSG_SIZE];                   // buffer to hold outgoing debug/mqtt messages.

boolean setupWifi() {
  Serial.printf("connecting to: %s ..", WIFI_SSID);
  boolean connected = WiFi.status() == WL_CONNECTED;
  if (!connected) {
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
    
    int32_t waitTime = MS_WAIT_TIME_FOR_WIFI; 
    while (waitTime > 0 && !connected) {
      waitTime -= MS_DELAY_FOR_WIFI_CONNECTION;
      delay(MS_DELAY_FOR_WIFI_CONNECTION);
      Serial.print(".");
      connected = WiFi.status() == WL_CONNECTED;
    }
  }
  if (connected) {
    Serial.print("WiFi connected at: ");
    Serial.println(WiFi.localIP());
  }
  return connected;
}

uint16_t takeSample() {
  Serial.printf("\nIn sample... ");
  boolean connection = setupWifi();
  Serial.printf("Connection %s \n", connection?"succeeded":"failed");
  return 1;
}

uint16_t takeMeasurement(uint16_t * sample, uint32_t n) {
  Serial.printf("\nIn measurement... ");
  boolean connection = setupWifi();
  Serial.printf("connection %s \n", connection?"succeeded":"failed");
  return 1;
}

void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nIn transmit... ");
  boolean connection = setupWifi();
  Serial.printf("connection %s \n", connection?"succeeded":"failed");
}

// ===============  Arduino Pattern ===================================================
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Switch off the LED
  digitalWrite(BUILTIN_LED,HIGH);
  Serial.begin(115200);
  config.setParameters(120000,15000,3,3);  // Default parameters - used first time round.
  config.setVersion(VERSION);
  sampler.setup();
  sampler.onTakeSample(takeSample);
  sampler.onTakeMeasurement(takeMeasurement);
  sampler.onTransmit(transmit);
  config.populateStatusMsg(msg, MSG_SIZE);
  Serial.println();
  Serial.println(msg);
}

void loop() {
  sampler.loop();
}
