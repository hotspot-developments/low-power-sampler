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

typedef struct  {
  uint32_t crc32;
  Parameters config;
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
    void setParameters(
          uint32_t measurementInterval,
          uint32_t sampleInterval,
          uint16_t nSamples,
          uint16_t transmitFrequency);
    void populateParameters(Parameters* params);
    void populateStatusMsg(char * msg, size_t length);
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
};

#endif  // _CONFIGURATION_H
