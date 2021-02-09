#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

#include "private.h"
#include "Configuration.h"
#include "Sampler.h"

#define MSG_SIZE 250
#define NTP_PACKET_SIZE 48

#define VERSION 100
#define MS_DELAY_FOR_WIFI_CONNECTION  500
#define MS_DELAY_FOR_MQTT_CONNECTION  500
#define MS_DELAY_FOR_MQTT_RECEIVE     500
#define MS_DELAY_FOR_NTP_RESPONSE     100
#define MS_WAIT_TIME_FOR_MESSAGES    5000
#define MS_WAIT_TIME_FOR_WIFI        5000
#define MS_WAIT_TIME_FOR_MQTT        5000

WiFiClient espClient;
WiFiUDP udpClient;
PubSubClient mqttClient(espClient);
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
  Serial.print("Local port:\t");
  Serial.println(udpClient.localPort());

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

boolean setupMqtt() {
  if (WiFi.status() != WL_CONNECTED) return false;
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  // Loop until we're reconnected
  int32_t waitTime = MS_WAIT_TIME_FOR_WIFI; 
  boolean connected = false;
  while (waitTime > 0 && !connected) {
    waitTime -= MS_DELAY_FOR_MQTT_CONNECTION;
    Serial.print("\nAttempting MQTT connection...");
    // Attempt to connect
    connected = mqttClient.connect(MQTT_CLIENT_ID);
    if (connected) {
      Serial.println("connected");
      mqttClient.setCallback(mqttReceiveMsg);
      mqttClient.subscribe(MQTT_IN_TOPIC);
    } else {
      Serial.printf("failed, rc=%d try again in %d ms",mqttClient.state(),MS_DELAY_FOR_MQTT_CONNECTION);
      delay(MS_DELAY_FOR_MQTT_CONNECTION);
    }
  }
  return connected;
}

void ntpReceiveMsg(byte* ntpBuffer) {
  unsigned long  NTPTime = 0 ;
  NTPTime |= (unsigned long) (ntpBuffer[40] << 24);
  NTPTime |= (unsigned long)(ntpBuffer[41] << 16);
  NTPTime |= (unsigned long)(ntpBuffer[42] << 8); 
  NTPTime |= (unsigned long) ntpBuffer[43]; 
  sampler.synchronise(NTPTime); 
  Serial.printf("NTP response received, time: %u\n", NTPTime);
}

void mqttReceiveMsg(char* topic, uint8_t *payload, unsigned int length) {
  Serial.printf("\nMessage arrived on: %s:\n", topic);
  char configJson[MAX_EXPECTED_CONFIG_STRING];
  for (int i=0; i < length; i++) {
    Serial.print((char) payload[i]);
    configJson[i] = (char) payload[i];
  }
  configJson[length] =0;
  config.fromJson(configJson);
  config.populateStatusMsg(msg, MSG_SIZE);
  Serial.println(msg);
}

uint16_t takeSample() {
  Serial.printf("\nTaking sample... ");
  return digitalRead(D6);
}

uint16_t takeMeasurement(uint16_t * sample, uint32_t n) {
  Serial.printf("\nTaking measurement as mode... ");
  int sum = 0;
  for (int i=0; i < n; i++) sum += sample[i] > 0? 1: -1;
  return (sum > 0)?1:0;
}

void waitForResponse() {
  int32_t waitTime = MS_WAIT_TIME_FOR_MESSAGES;
  boolean ntpReceived = false;
  while(waitTime > 0) {
    waitTime -= ntpReceived?MS_DELAY_FOR_MQTT_RECEIVE:MS_DELAY_FOR_NTP_RESPONSE;
    delay(ntpReceived?MS_DELAY_FOR_MQTT_RECEIVE:MS_DELAY_FOR_NTP_RESPONSE);
    if (!ntpReceived) {
      ntpReceived = udpClient.parsePacket() >= NTP_PACKET_SIZE;
      if (ntpReceived) {
        udpClient.read(NTPBuffer, NTP_PACKET_SIZE);
        ntpReceiveMsg(NTPBuffer);
      }
    }
    mqttClient.loop();
  }
  Serial.println("Finished waiting for responses.");
}

void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nCommunicating with base... ");
  setupWifi();
  setupNtp();
  setupMqtt();
  waitForResponse();

  uint16_t status = measurement[0];
  float battery = analogRead(A0) / 1023.0;
  unsigned version = config.getVersion();
  snprintf (msg, 155, "firmware: %u, value: %hu, voltage: %f", version, status, battery*5.0);
  mqttClient.publish(MQTT_OUT_TOPIC, msg);
  mqttClient.disconnect();
  Serial.println(msg);
}

// ===============  Arduino Pattern ===================================================
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Switch off the LED
  digitalWrite(BUILTIN_LED,HIGH);
  pinMode(D6, INPUT);
  pinMode(A0, INPUT);
  Serial.begin(115200);
  config.setParameters(180000,5000,5,1);  // Default parameters - used first time round.
  config.setVersion(VERSION);
  boolean isFirstTime = !config.checkMemory();
  Serial.printf("\nSetup: configuration taken from %s.", !isFirstTime?"memory":"defaults");
  sampler.setup();
  sampler.onTakeSample(takeSample);
  sampler.onTakeMeasurement(takeMeasurement);
  sampler.onTransmit(transmit);
//  if (isFirstTime) {        
//    setupWifi();
//    setupNtp();
//    setupMqtt();
//    waitForResponse();
//    mqttClient.disconnect();
//    udpClient.stopAll();
//  }
  config.populateStatusMsg(msg, MSG_SIZE);
  Serial.println(msg);
}

void loop() {
  sampler.loop();
}
