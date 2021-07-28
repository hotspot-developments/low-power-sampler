#include "Espx.h"
#include <Arduino.h>


#if defined(ESP32)

#ifndef MAX_RTC_SIZE 
#define MAX_RTC_SIZE 512
#endif

RTC_DATA_ATTR uint8_t RTC[MAX_RTC_SIZE];

void Espx::deepSleep(uint64_t time_us, bool wakeWithWifi = true) {
    esp_sleep_enable_timer_wakeup(time_us);
    esp_deep_sleep_start();
}

bool Espx::rtcUserMemoryRead(uint32_t offset, uint32_t *data, size_t size) {
    memcpy(data, &(RTC[offset*4]), size);
    return true;
}

bool Espx::rtcUserMemoryWrite(uint32_t offset, uint32_t *data, size_t size){
    memcpy( &(RTC[offset*4]), data, size);
    return true;
}

t_httpUpdate_return Espx::httpUpdate(WiFiClient& client, const String& host, uint16_t port, const String& uri,
                               const String& currentVersion) {
    return ESPhttpUpdate.update(host, port, uri, currentVersion);
}
#elif defined(ESP8266)

void Espx::deepSleep(uint64_t time_us, bool wakeWithWifi = true) {
    ESP.deepSleep(time_us, wakeWithWifi?RF_DEFAULT: RF_DISABLED);
}

bool Espx::rtcUserMemoryRead(uint32_t offset, uint32_t *data, size_t size) {
    return ESP.rtcUserMemoryRead(offset, data, size);
}
bool Espx::rtcUserMemoryWrite(uint32_t offset, uint32_t *data, size_t size){
    return ESP.rtcUserMemoryWrite(offset, data, size);
}

t_httpUpdate_return Espx::httpUpdate(WiFiClient& client, const String& host, uint16_t port, const String& uri,
                               const String& currentVersion) {
    return ESPhttpUpdate.update(client, host, port, uri, currentVersion);
}
#endif