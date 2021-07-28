// MIT License

// Low Power Sampler Configuration preserved over DeepSleep.
// Copyright (c) 2021 Simon Chinnick

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#define MAX_KEY_LENGTH 20
#define MAX_VALUE_LENGTH 20
#define MAX_EXPECTED_CONFIG_STRING 220
#define MAX_DATA_ELEMENTS 160
#define OTA_OFFSET 32

typedef struct {
  uint16_t currentVersion;
  uint16_t counter;
  uint32_t measurementInterval;
  uint32_t sampleInterval;
  uint16_t nSamples;
  uint16_t transmitFrequency;
} Parameters;

typedef struct {
  uint32_t startTimeOfDay;
  uint32_t syncTime;
  uint32_t nominalElapsed;
  float    calibrationFactor;
} Synchronisation;

typedef struct  {
  uint32_t crc32;
  Parameters config;
  Synchronisation sync;
  uint16_t data[MAX_DATA_ELEMENTS];
} RtcData;


class Configuration {
    
  private:
    RtcData rtcData;
    void setParameter(const char* key, const char* value);
    size_t indexOf(const char chr, const char* strng, size_t start = 0);
    size_t nextSeparator(const char* json, const size_t& start, const size_t& length);
    void parseToken(const char * json, size_t& pos, const size_t length, char* token);
    size_t trim(const char* json, size_t &length) ;
    uint32_t calculateCRC32(const uint8_t *data, size_t length) ;

  public:
    Configuration();
    void setParameters(
          uint32_t measurementInterval,
          uint32_t sampleInterval,
          uint16_t nSamples,
          uint16_t transmitFrequency);
    void populateParameters(Parameters* params);
    void populateSynchronisation(Synchronisation* sync);
    void populateStatusMsg(char * msg, size_t length);
    bool equivalentTo(Configuration& other);
    void fromJson(const char * json);
    bool checkMemory();
    bool fromMemory();
    bool save();
    void setVersion(unsigned version);
    uint16_t getVersion();
    void incrementCounter();
    void resetCounter();
    uint16_t getCounter();
    uint16_t* getData();
    void resetSynchronisation(uint32_t time, float factor);
    void incrementElapsed(uint32_t msSleepTime);
};

#endif  // _CONFIGURATION_H
