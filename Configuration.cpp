#define NOT_FOUND __UINT32_MAX__

#include <cstdio>
#include <ctype.h>
#include <Esp.h>

#include "Configuration.h"

Configuration::Configuration() {
  rtcData.config.counter = 1;
  rtcData.config.currentVersion = 0;
  rtcData.config.measurementInterval = 0;
  rtcData.config.sampleInterval = 0;
  rtcData.config.nSamples = 0;
  rtcData.config.transmitFrequency = 0;
  rtcData.sync.calibrationFactor = 1.0;
}


void Configuration::setParameter(const char* key, const char* value) {
  if (strcmp(key, "sampleInterval") == 0) {
    if (strlen(value) > 0) sscanf(value, "%u" , &rtcData.config.sampleInterval);
  }
  else if (strcmp(key, "nSamples") == 0) {
    if (strlen(value) > 0) sscanf(value, "%hu", &rtcData.config.nSamples);
  }
  else if (strcmp(key, "measurementInterval") == 0) {
    if (strlen(value) > 0) sscanf(value, "%u", &rtcData.config.measurementInterval);
  }
  else if (strcmp(key, "transmitFrequency") == 0) {
    if (strlen(value) > 0) sscanf(value, "%hu", &rtcData.config.transmitFrequency);
  }
  else if (strcmp(key, "version") == 0) {
    if (strlen(value) > 0) sscanf(value, "%hu", &rtcData.config.currentVersion);
  }
}

size_t Configuration::indexOf(const char chr, const char* strng, size_t start) {
  size_t len = strlen(strng);
  size_t pos = start;
  while (pos < len) {
    if (strng[pos] == chr) return pos;
    pos++;
  }
  return NOT_FOUND;
}

size_t Configuration::nextSeparator(const char* json, const size_t& start, const size_t& length)
{
  size_t colon = indexOf(':', json, start);
  size_t comma = indexOf(',', json, start);
  return (colon == NOT_FOUND && comma == NOT_FOUND) ? length :
         ((colon != NOT_FOUND && comma != NOT_FOUND) ? (comma < colon ? comma : colon) :
          (colon == NOT_FOUND ? comma : colon));
}


void Configuration::parseToken(const char * json, size_t& pos, const size_t length, char* token) {
  size_t start, end;
  token[0] = 0;
  while (pos < length && isspace(json[pos])) pos++;
  if (pos == length) return;

  if (json[pos] == '\"' || json[pos] == '\'') {
    char quote = json[pos];
    start = pos + 1;
    end = indexOf(quote, json, start);
    if (end < 0) end = length - 1;
    pos = end + 1;
    end--;
  }
  else {
    start = pos;
    end = nextSeparator(json, start, length);
    pos = end;
    end--;
    while (isspace(json[end])) end--;
  }
  size_t size = (end - start) + 1;
  strncpy(token, &(json[start]), size);
  token[size] = 0;
}

size_t Configuration::trim(const char* json, size_t &length) {
  size_t pos = 0;
  length = strlen(json);
  while (pos < length && isspace(json[pos])) pos++;
  if (json[pos] == '{') {
    pos++;
    while (pos < length && isspace(json[pos])) pos++;
    size_t end = length - 1;
    while (end > pos && isspace(json[end])) end--;
    if (json[end] != '}') end++;
    length = end;
  }

  return pos;
}

uint32_t Configuration::calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}


void Configuration::fromJson(const char * json) {
  char key[MAX_KEY_LENGTH];
  char value[MAX_VALUE_LENGTH];
  size_t length;
  size_t pos = trim(json, length);

  while (pos < length) {
    parseToken(json, pos, length, key);
    if (strlen(key) > 0) {
      pos = nextSeparator(json, pos, length);
      if (pos == length || json[pos] == ',') {
        value[0] = 0;
      }
      else {
        pos++;
        parseToken(json, pos, length, value);
      }
      setParameter(key, value);
    }
    pos = indexOf(',', json, pos);
    pos = (pos != NOT_FOUND) ? pos + 1 : length;
  }
  this->rtcData.config.counter = 1;
}


bool Configuration::checkMemory() {
  uint32_t crc32;
  Parameters params;
  if (ESP.rtcUserMemoryRead(OTA_OFFSET, &crc32, sizeof(crc32)) &&
      ESP.rtcUserMemoryRead(OTA_OFFSET + 1, (uint32_t*) &params, sizeof(params))) {
    uint32_t crcOfParams = calculateCRC32((uint8_t*) &params, sizeof(params));
    return crcOfParams == crc32;
  }
  return false;
}

bool Configuration::fromMemory() {
  if (ESP.rtcUserMemoryRead(OTA_OFFSET, (uint32_t*) &rtcData, sizeof(rtcData))) {
    uint32_t crcOfConfig = calculateCRC32((uint8_t*) &rtcData.config, sizeof(rtcData.config));
    return crcOfConfig == rtcData.crc32;
  }
  return false;
}

bool Configuration::save() {
  rtcData.crc32 = calculateCRC32((uint8_t*) &rtcData.config, sizeof(rtcData.config));
  return ESP.rtcUserMemoryWrite(OTA_OFFSET, &rtcData.crc32, sizeof(rtcData) );
}


void Configuration::setParameters(
    uint32_t measurementInterval,
    uint32_t sampleInterval,
    uint16_t nSamples,
    uint16_t transmitFrequency) {
  rtcData.config.measurementInterval = measurementInterval;
  rtcData.config.sampleInterval = sampleInterval;
  rtcData.config.nSamples = nSamples;
  rtcData.config.transmitFrequency = transmitFrequency;
  rtcData.config.counter = 1;
}

uint16_t Configuration::getVersion() {
  return rtcData.config.currentVersion;
}

void Configuration::setVersion(unsigned version) {
  rtcData.config.currentVersion = version;
}

uint16_t Configuration::getCounter() {
  return rtcData.config.counter;
}

uint16_t* Configuration::getData() {
  return rtcData.data;
}

void Configuration::incrementCounter() {
  rtcData.config.counter++;
}

void Configuration::resetCounter() {
  rtcData.config.counter = 1;
}

void Configuration::populateStatusMsg(char * msg, size_t length) {
  snprintf(msg, length, 
          "Version: %hu, counter: %hu, measurementInterval: %u, sampleInterval: %u, nSamples: %hu, transmitFrequency: %hu, calibration: %f",
          rtcData.config.currentVersion, rtcData.config.counter,
          rtcData.config.measurementInterval, rtcData.config.sampleInterval, rtcData.config.nSamples, rtcData.config.transmitFrequency,
          rtcData.sync.calibrationFactor);
}

void Configuration::populateParameters(Parameters* params) {
  params->counter = this->rtcData.config.counter;
  params->currentVersion = this->rtcData.config.currentVersion;
  params->measurementInterval = this->rtcData.config.measurementInterval;
  params->sampleInterval = this->rtcData.config.sampleInterval;
  params->nSamples = this->rtcData.config.nSamples;
  params->transmitFrequency = this->rtcData.config.transmitFrequency;
}

void Configuration::populateSynchronisation(Synchronisation* sync) {
  sync->startTimeOfDay = this->rtcData.sync.startTimeOfDay;
  sync->syncTime = this->rtcData.sync.syncTime;
  sync->nominalElapsed = this->rtcData.sync.nominalElapsed;
  sync->calibrationFactor = this->rtcData.sync.calibrationFactor;
}

void Configuration::resetSynchronisation(uint32_t time, float factor) {
  this->rtcData.sync.syncTime = time;
  this->rtcData.sync.nominalElapsed = 0;
  this->rtcData.sync.calibrationFactor = factor;
}

void Configuration::incrementElapsed(uint32_t msSleepTime) {
  this->rtcData.sync.nominalElapsed += msSleepTime/1000;
}
