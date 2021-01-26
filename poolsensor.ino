#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "private.h"
#include "Configuration.h"
#include "Sampler.h"

#define MSG_SIZE 250
#define VERSION 100
#define MS_DELAY_FOR_WIFI_CONNECTION  500
#define MS_DELAY_FOR_MQTT_CONNECTION  500
#define MS_DELAY_FOR_MQTT_RECEIVE    1000

WiFiClient espClient;
PubSubClient client(espClient);
Configuration config;
Sampler sampler(config);
char msg[MSG_SIZE];

void setupWifi() {
  Serial.printf("\nConnecting to: %s ..", WIFI_SSID);
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(MS_DELAY_FOR_WIFI_CONNECTION);
    Serial.print(".");
  }
  Serial.print("WiFi connected at: ");
  Serial.println(WiFi.localIP());
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

void reconnectToMqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("\nAttempting MQTT connection...");
    String clientId = MQTT_CLIENT_ID;
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(MQTT_IN_TOPIC);
    } else {
      Serial.printf("failed, rc=%d try again in %d ms",client.state(),MS_DELAY_FOR_MQTT_CONNECTION);
      delay(MS_DELAY_FOR_MQTT_CONNECTION);
    }
  }
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

void sendDataToBase(uint16_t * measurement, uint32_t n) {
  setupWifi();
  reconnectToMqtt();
  delay(MS_DELAY_FOR_MQTT_RECEIVE);
  for (int i=1; i < 1000; i++) client.loop();
  uint16_t status = measurement[0];
  float battery = analogRead(A0) / 1023.0;
  unsigned version = config.getVersion();
  snprintf (msg, 155, "firmware: %u, value: %hu, voltage: %f", version, status, battery*5.0);
  client.publish(MQTT_OUT_TOPIC, msg);
  client.disconnect();
  Serial.println(msg);
}

// ===============  Arduino Pattern ===================================================
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Switch off the LED
  digitalWrite(BUILTIN_LED,HIGH);
  pinMode(D6, INPUT);
  pinMode(A0, INPUT);
  config.setParameters(180000,5000,5,1);  // Default parameters - used first time round.
  config.setVersion(VERSION);
  Serial.begin(115200);
  Serial.printf("\nSetup: configuration taken from %s.", config.checkMemory()?"memory":"defaults");
  sampler.setup();
  sampler.onTakeSample(takeSample);
  sampler.onTakeMeasurement(takeMeasurement);
  sampler.onTransmit(sendDataToBase);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttReceiveMsg);
}

void loop() {
  sampler.loop();
}
