#include "Espx.h"
#include <PubSubClient.h>
#include <WiFiUdp.h>

#include "private.h"
#include "Configuration.h"
#include "Sampler.h"

#if defined(ESP8266)
#define SENSOR_PIN D6
#else
#define SENSOR_PIN 34
#endif
#define MSG_SIZE 250
#define NTP_PACKET_SIZE 48

#define VERSION 104
#define MS_DELAY_FOR_WIFI_CONNECTION   500
#define MS_DELAY_FOR_MQTT_CONNECTION   500
#define MS_DELAY_FOR_MQTT_RECEIVE      500
#define MS_DELAY_FOR_NTP_RESPONSE      100
#define MS_WAIT_TIME_FOR_MESSAGES    10000
#define MS_WAIT_TIME_FOR_WIFI        10000
#define MS_WAIT_TIME_FOR_MQTT        10000

WiFiClient espClient;
WiFiUDP udpClient;
PubSubClient mqttClient(espClient);
Configuration config;
Sampler sampler(config);
char msg[MSG_SIZE];                   // buffer to hold outgoing debug/mqtt messages.
byte NTPBuffer[NTP_PACKET_SIZE];      // buffer to hold incoming and outgoing ntp packets.
IPAddress timeServerIP;               // IP address of NTP server.
uint16_t currentVersion = VERSION;

void ntpReceiveMsg(byte* ntpBuffer) {
  unsigned long  NTPTime = 0 ;
  NTPTime |= (unsigned long) (ntpBuffer[40] << 24);
  NTPTime |= (unsigned long)(ntpBuffer[41] << 16);
  NTPTime |= (unsigned long)(ntpBuffer[42] << 8); 
  NTPTime |= (unsigned long) ntpBuffer[43]; 
  sampler.synchronise(NTPTime); 
  Serial.printf("NTP response received, time: %lu\n", NTPTime);
}

void mqttReceiveMsg(char* topic, uint8_t *payload, unsigned int length) {
  Configuration updateConfig;
  Serial.printf("\nMessage arrived on: %s:\n", topic);
  char configJson[MAX_EXPECTED_CONFIG_STRING];
  for (unsigned int i=0; i < length; i++) {
    Serial.print((char) payload[i]);
    configJson[i] = (char) payload[i];
  }
  configJson[length] =0;
  updateConfig.fromMemory();
  updateConfig.fromJson(configJson);
  if (!updateConfig.equivalentTo(config)) {
    config.fromJson(configJson);
    config.populateStatusMsg(msg, MSG_SIZE);
    Serial.printf("\nUpdated - %s\n",msg);
  }
}

boolean setupWifi() {
  Serial.printf("\nConnecting to: %s ..", WIFI_SSID);
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

  int32_t waitUntil = millis() + MS_WAIT_TIME_FOR_WIFI; 
  boolean connected = WiFi.status() == WL_CONNECTED;
  while (millis() < waitUntil && !connected) {
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

boolean setupMqtt() {
  if (WiFi.status() != WL_CONNECTED) return false;
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  // Loop until we're reconnected
  int32_t waitUntil = millis() + MS_WAIT_TIME_FOR_MQTT; 
  boolean connected = false;
  while (millis() < waitUntil && !connected) {
    Serial.print("Attempting MQTT connection...");
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

uint16_t takeSample() {
  Serial.printf("\nTaking sample...... ");
  return digitalRead(SENSOR_PIN);
}

uint16_t takeMeasurement(uint16_t * sample, uint32_t n) {
  Serial.printf("\nTaking measurement as mode... ");
  int sum = 0;
  for (unsigned int i=0; i < n; i++) sum += sample[i] > 0? 1: -1;
  return (sum > 0)?1:0;
}

void waitForResponse(bool ntpRequired, bool mqttRequired) {
  int32_t waitUntil = millis() + MS_WAIT_TIME_FOR_MESSAGES;
  Serial.printf("\nCheck responses %s ",ntpRequired && mqttRequired? "" : (ntpRequired? "for NTP": "for MQTT"));
  while(millis() < waitUntil && (ntpRequired || mqttRequired)) {
    if (ntpRequired) {
      if (udpClient.parsePacket() >= NTP_PACKET_SIZE) {
        udpClient.read(NTPBuffer, NTP_PACKET_SIZE);
        ntpReceiveMsg(NTPBuffer);
        ntpRequired = false;
      }
    }
    if (mqttRequired) mqttClient.loop();
    if (ntpRequired || mqttRequired) {
      Serial.printf("%c", ntpRequired && mqttRequired?'.':(ntpRequired?'n':'m'));
      delay(ntpRequired?MS_DELAY_FOR_NTP_RESPONSE:MS_DELAY_FOR_MQTT_RECEIVE);
    }
  }
  Serial.println("\nFinished waiting for responses.");
}

void doUpdate() {
  Serial.printf("\nPerforming update of firmware.\n");
  ESPhttpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = Espx::httpUpdate(espClient, UPDATE_SERVER, UPDATE_PORT, UPDATE_PATH);
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    config.setVersion(currentVersion);
    break;
  
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;
  
  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    config.save();
    ESP.restart();
    break;
  }
}


void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nCommunicating with base... ");
  setupWifi();
  bool ntpInitiated = setupNtp();
  bool mqttConnected = setupMqtt();

  float battery = analogRead(A0) / 4096.0;
  unsigned version = config.getVersion();
  int nchars = snprintf (msg, MSG_SIZE, "firmware: %u, values:[", version);
  nchars+= snprintf(msg+nchars, MSG_SIZE - nchars, "%hu", measurement[0]);
  for (unsigned int i=1; i < n; i++) {
    nchars+= snprintf(msg+nchars, MSG_SIZE - nchars, ",%hu", measurement[i]);
  }
  nchars += snprintf(msg+nchars, MSG_SIZE - nchars,"], voltage: %f",  battery*3.3 );
  Synchronisation sync; Parameters params;
  config.populateSynchronisation(&sync);
  config.populateParameters(&params);
  snprintf(msg +nchars, MSG_SIZE - nchars, " counter: %hu, syncTime: %u, nominal: %u, factor: %f", 
            params.counter, sync.syncTime, sync.nominalElapsed, sync.calibrationFactor);
  if (mqttConnected) {
    bool published = mqttClient.publish(MQTT_OUT_TOPIC, msg);
    Serial.printf("Publish %s: %s", published?"succeeded":"failed", msg);
  } else {
    Serial.println("Could not publish to mqtt");
  }
  waitForResponse(ntpInitiated, mqttConnected);
  if (mqttConnected)  mqttClient.disconnect();
  if (config.getVersion() > currentVersion) {
    doUpdate();
  }
}

// ===============  Arduino Pattern ===================================================
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Switch off the LED
  digitalWrite(LED_BUILTIN,HIGH);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(A0, INPUT);
  Serial.begin(115200);
  config.setParameters(180000,5000,5,1);  // Default parameters - used first time round.
  config.setVersion(VERSION);
  boolean isFirstTime = !config.checkMemory();
  Serial.printf("\nSetup: Configuration taken from %s.", !isFirstTime?"memory":"defaults");
  sampler.setup();
  sampler.onTakeSample(takeSample);
  sampler.onTakeMeasurement(takeMeasurement);
  sampler.onTransmit(transmit);
  config.populateStatusMsg(msg, MSG_SIZE);
  Serial.println(msg);
  currentVersion = config.getVersion();
}

void loop() {
  sampler.loop();
}
