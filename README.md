# Arduino Low-power Sampler
This project contains code for an ESP8266 to take samples from a sensor at regular intervals using a deepsleep between samples to save battery life.
## Motivation
My mate, Paul, has a swimming pool that needs topping up every now and again, and he hit on the idea of using the OpenSprinkler<sup>1</sup> project to activate a 'pseudo-sprinkler' based on a sensor reading in order to add water to the pool. 
Since the pool was away from the main villa, we wanted a self contained device running off a battery (recharged with a small solar panel)<sup>2</sup> that would periodically check to see if the pool needed filling.

Paul found a simple water sensor that we could connect to a NodeMCU dev board that simply indicated whether it was currently in contact with the water. Positioning the sensor in the right place would enable us to figure out when the water level dropped so that the sprinkler system could refill it. Now we just needed the code to efficiently check the level now and again without draining the battery too much, and periodically report the level to base via wifi (and MQTT<sup>4</sup>).

## Code
The ESP has a deepsleep facility. The great thing about deepsleep is that it consumes very little power. The drawback with deepsleep is the waking up - it does this via a reset, so that your program effectively starts from scratch with each wake. The only way I found to preserve some values between deepsleeps is to use some memory in the RTC hardware module<sup>3</sup>. So I divided the code into two modules; one to hold, update, save and retrieve the sampling configuration, and the second to use the configuration to work out when to invoke a sensor reading, and when to transmit values over the wifi.

### Configuration 
As pool-sensor project took shape we realized that taking a single reading wasn't necessarily reliable. So we took 5 samples, and used the mode average to represent the measurement. The samples were taken at 5 second intervals and the resulting measurement transmitted to an MQTT<sup>5</sup> server each hour. Note that we didn't really need hourly measurements, we could store those up and transmit, say, 6 hourly-measurements every 6 hours - saving even more battery.

The Configuration class has a method to set the Sampling config:  
```
void setParameters(  
          uint32_t measurementInterval,  
          uint32_t sampleInterval,  
          uint16_t nSamples,  
          uint16_t transmitFrequency)  
```
where  

| Parameter | Description |  
| --------- | ----------- |  
| measurementInterval | How long in ms between measurements (e.g. an hour 3600000 ms) |  
| sampleInterval | How long in ms between samples (e.g 5000 ms)|  
| nSamples | The number of samples to take for each measurement |  
| transmitFrequency | How frequently to transmit the measurements |  
<br/>  

### Sampler
The Sampler is passed it's configuration as it is constructed, and then has two methods that need to be called:
```
void setup()
void loop()
```
| Method | Description |
| ------ | ----------- |
| setup  | Reads the RTC memory for the last saved configuration, or if none found, uses the values set on the configuration passed into the Sampler constructor. |
| loop   | Invokes sample/measurement/transmit callbacks as appropriate and then sends the ESP into deepsleep for the appropriate amount of time. |

These correspond to the usual Arduino pattern, where `setup` should be called in the Arduino setup function, and `loop` <i>as the last statement</i> in the Arduino loop function. 

Note that the loop method is a bit of a misnomer, since it is called once, resulting in a deep sleep - causing the code to 'reset' and start from the beginning once it awakes. (I called it `loop` to abide by convention).

### The Callbacks
The Sampler `loop` will call zero or more callbacks on each iteration, depending on whether its time to take a sample, convert samples to a measurement, or transmit the measurement.
### onTakeSample
```
    void onTakeSample(std::function<uint16_t()> fnSample);
```
Write a function to take a sensor reading, and return it as a 16-bit unsigned integer. The sampler will store the reading over deepsleeps. If sampling is not required - this callback can be skipped i.e. doesn't need to be defined. An example sampling function might look like:  
```
uint16_t takeSample() {
  Serial.printf("\nTaking sample... ");
  return digitalRead(D6);
}
```
In the Arduino setup function, assign this function as our sample callback like this:  
```
  sampler.onTakeSample(takeSample);
```

### onTakeMeasurement
```
    void onTakeMeasurement(std::function<uint16_t(uint16_t*, uint32_t)> fnMeasurement);
```
Write a function to make a measurement (optionally based on the `n` samples we have previously taken) and return it as a 16-bit unsigned integer. The Sampler will store multiple measurements until its time to transmit them. If sampling is not required, and we transmit each measurement straight away, this callback can be skipped - doesn't need to be defined. An example measurement function might look like:  
```
uint16_t takeMeasurement(uint16_t * sample, uint32_t n) {
  Serial.printf("\nTaking measurement as mode... ");
  int sum = 0;
  for (int i=0; i < n; i++) sum += sample[i] > 0? 1: -1;
  return (sum > 0)?1:0;
}
```
In the Arduino setup function, assign this function as our measurement callback like this:  
```
  sampler.onTakeMeasurement(takeMeasurement);
```

### onTransmit
```
    void onTransmit(std::function<void(uint16_t*, uint32_t)> fnTransmit);
```
Write a function to transmit the measurements to a server - or do something else at regular intervals, it's designed to be based on the supplied set of `t` measurements. However, in the simplest of scenarios, this might be the only callback required in which it takes a single sensor reading and sends it via MQTT or HTTP to a server somewhere. An example transmit function could be:
```
void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nCommunicating with base... ");
  setupWifi();
  setupMqtt();

  uint16_t status = measurement[n - 1];
  float battery = analogRead(A0) / 1023.0;
  unsigned version = config.getVersion();
  snprintf (msg, 155, "firmware: %u, value: %hu, voltage: %f", version, status, battery*5.0);
  mqttClient.publish(MQTT_OUT_TOPIC, msg);
  mqttClient.disconnect();
  Serial.println(msg);
}
```
In the Arduino setup function, assign this function as our transmit callback like this:  
```
  sampler.onTransmit(transmit);
```
Note that the transmit callback is the only callback that is guaranteed to have the Wifi RF module enabled on the ESP chip as the Sampler disables it (on wake up) for all other occassions to save power.

## Usage
### Simplest Case
Take a single sensor measurement every hour and send to server. This only requires the onTransmit callback to be defined.
```
    :
    :
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Configuration config;
Sampler sampler(config);
char msg[MSG_SIZE];          // buffer to hold outgoing debug/mqtt messages.
    :
    :
void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nTaking a sensor reading... ");
  uint16_t status =  digitalRead(D6);
  
  Serial.printf("\nCommunicating with base... ");
  setupWifi();
  setupMqtt();

  float battery = analogRead(A0) / 1023.0;
  unsigned version = config.getVersion();
  snprintf (msg, 155, "firmware: %u, value: %hu, voltage: %f", version, status, battery*5.0);
  mqttClient.publish(MQTT_OUT_TOPIC, msg);
  mqttClient.disconnect();
  Serial.println(msg);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Switch off the LED
  digitalWrite(BUILTIN_LED,HIGH);
  pinMode(D6, INPUT);
  pinMode(A0, INPUT);
  Serial.begin(115200);
  config.setParameters(3600000,0,1,1);  // Initial parameters - used first time round.
  config.setVersion(VERSION);
  sampler.setup();
  sampler.onTransmit(transmit);
}

void loop() {
  sampler.loop();   // Called once and then invokes a deepsleep.
}
```
### More complex case
Take 5 sample readings 5 seconds apart once every 3 minutes use the mode of the 5 readings as our measurement. On the 10th occasion transmit the latest measurement to base.

```
    :
    :
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Configuration config;
Sampler sampler(config);
char msg[MSG_SIZE];          // buffer to hold outgoing debug/mqtt messages.
    :
    :
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

void transmit(uint16_t * measurement, uint32_t n) {
  Serial.printf("\nCommunicating with base... ");
  setupWifi();
  setupMqtt();

  uint16_t status = measurement[n-1];  // Just transmit the last measurement
  float battery = analogRead(A0) / 1023.0;
  unsigned version = config.getVersion();
  snprintf (msg, 155, "firmware: %u, value: %hu, voltage: %f", version, status, battery*5.0);
  mqttClient.publish(MQTT_OUT_TOPIC, msg);
  mqttClient.disconnect();
  Serial.println(msg);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Switch off the LED
  digitalWrite(BUILTIN_LED,HIGH);
  pinMode(D6, INPUT);
  pinMode(A0, INPUT);
  Serial.begin(115200);
  config.setParameters(180000,5000,5,10);  // Default parameters - used first time round.
  config.setVersion(VERSION);
  sampler.setup();
  sampler.onTakeSample(takeSample);
  sampler.onTakeMeasurement(takeMeasurement);
  sampler.onTransmit(transmit);
  config.populateStatusMsg(msg, MSG_SIZE);
  Serial.println(msg);
}

void loop() {
  sampler.loop();
}
```
## Coming soon
-  Synchronise with an NTP server
-  Update configuration via an MQTT JSON message
-  Update the software over the air
-  Use very small (sub-second) sample intervals, and delay rather than deepsleep.
-  More verified examples
-  Restructure in line with Arduino library

## References
### 1. OpenSprinkler 
https://opensprinkler.com/
### 2. Power ESP8266 with Solar Panels and battery
https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/
### 3. RTC User Memory example
https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/RTCUserMemory/RTCUserMemory.ino
### 4. MQTT 
https://mqtt.org/
### 5. MQTT Client
https://www.arduino.cc/reference/en/libraries/pubsubclient/
