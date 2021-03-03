#define ESP8266
#define Arduino_h

#include <gtest/gtest.h>
#include "./fake/Esp.h"
#include "../src/Espx.cpp"
#include "../src/Configuration.cpp"
#include "../src/Sampler.cpp"


#define ASSERT_TAKE_SAMPLE_NOT_CALLED() ASSERT_FALSE(SamplerTest::sampleCalled);
#define ASSERT_TAKE_MEASUREMENT_NOT_CALLED() ASSERT_FALSE(SamplerTest::measurementCalled);
#define ASSERT_TRANSMIT_NOT_CALLED() ASSERT_FALSE(SamplerTest::transmitCalled);

#define ASSERT_TAKE_SAMPLE_CALLED() ASSERT_TRUE(SamplerTest::sampleCalled); SamplerTest::sampleCalled = false;

#define ASSERT_TAKE_MEASUREMENT1_CALLED(V1) \
ASSERT_TRUE(SamplerTest::measurementCalled); \
ASSERT_EQ(V1, SamplerTest::samplesReceived[0]); \
SamplerTest::measurementCalled = false;

#define ASSERT_TAKE_MEASUREMENT3_CALLED(V1,V2,V3) \
ASSERT_TRUE(SamplerTest::measurementCalled); \
ASSERT_EQ(V1, SamplerTest::samplesReceived[0]); \
ASSERT_EQ(V2, SamplerTest::samplesReceived[1]); \
ASSERT_EQ(V3, SamplerTest::samplesReceived[2]); \
SamplerTest::measurementCalled = false;

#define ASSERT_TAKE_MEASUREMENT5_CALLED(V1,V2,V3,V4,V5) \
ASSERT_TRUE(SamplerTest::measurementCalled); \
ASSERT_EQ(V1, SamplerTest::samplesReceived[0]); \
ASSERT_EQ(V2, SamplerTest::samplesReceived[1]); \
ASSERT_EQ(V3, SamplerTest::samplesReceived[2]); \
ASSERT_EQ(V4, SamplerTest::samplesReceived[3]); \
ASSERT_EQ(V5, SamplerTest::samplesReceived[4]); \
SamplerTest::measurementCalled = false;

#define ASSERT_TRANSMIT1_CALLED(V1) \
ASSERT_TRUE(SamplerTest::transmitCalled); \
ASSERT_EQ(V1, SamplerTest::measurementsReceived[0]); \
SamplerTest::transmitCalled = false;

#define ASSERT_TRANSMIT2_CALLED(V1, V2) \
ASSERT_TRUE(SamplerTest::transmitCalled); \
ASSERT_EQ(V1, SamplerTest::measurementsReceived[0]); \
ASSERT_EQ(V2, SamplerTest::measurementsReceived[1]); \
SamplerTest::transmitCalled = false;

#define ASSERT_TRANSMIT3_CALLED(V1, V2, V3) \
ASSERT_TRUE(SamplerTest::transmitCalled); \
ASSERT_EQ(V1, SamplerTest::measurementsReceived[0]); \
ASSERT_EQ(V2, SamplerTest::measurementsReceived[1]); \
ASSERT_EQ(V3, SamplerTest::measurementsReceived[2]); \
SamplerTest::transmitCalled = false;

class SamplerTest : public testing::Test {
    public:
    static uint16_t samplesReceived[MAX_DATA_ELEMENTS];
    static uint16_t _nSamples;
    static uint16_t measurementsReceived[MAX_DATA_ELEMENTS];
    static uint16_t _nMeasurements;
    static uint16_t returnedSample ;
    static uint16_t returnedMeasurement ;

    static bool sampleCalled;
    static uint16_t takeSample() {
        SamplerTest::sampleCalled = true;
        return SamplerTest::returnedSample;
    }

    static bool measurementCalled;
    static uint16_t takeMeasurement(uint16_t* samples, uint32_t nSamples) {
        SamplerTest::measurementCalled = true;
        memcpy(&SamplerTest::samplesReceived, samples, nSamples * sizeof(uint16_t));
        SamplerTest::_nSamples = nSamples;
        return SamplerTest::returnedMeasurement;
    }

    static bool transmitCalled;
    static void transmit(uint16_t* measurements, uint32_t nMeasurements) {
        SamplerTest::transmitCalled = true;
        memcpy(&SamplerTest::measurementsReceived, measurements, nMeasurements * sizeof(uint16_t));
        SamplerTest::_nMeasurements = nMeasurements;
    }

    protected:
    virtual void SetUp() {
        uint32_t BadNumber = 0xDEADDEAD;
        ESP.rtcUserMemoryWrite(OTA_OFFSET, &BadNumber, 4);
    }

    virtual void TearDown() {}

    void setVersionCounter(Configuration* config, uint16_t version, uint16_t counter) {
        uint16_t fudged[2];
        fudged[0] = version;
        fudged[1] = counter;
        config->save();
        ESP.rtcUserMemoryWrite(OTA_OFFSET + 1, (uint32_t*) &fudged, sizeof(uint32_t));
        config->fromMemory();
    }    
};


class SamplerNtpSyncTest: public testing::Test {

    public:
    static unsigned long presyncMillis;
    static unsigned long postsyncMillis;
    static uint32_t syncTimeSeconds;
    static Sampler *sampler;

    static void transmit(uint16_t* measurements, uint32_t nMeasurements) {
        ticks = SamplerNtpSyncTest::presyncMillis;
        SamplerNtpSyncTest::sampler->synchronise(SamplerNtpSyncTest::syncTimeSeconds);
        ticks = SamplerNtpSyncTest::postsyncMillis;
    }


    protected:
    virtual void SetUp() {
        uint32_t BadNumber = 0xDEADDEAD;
        ESP.rtcUserMemoryWrite(OTA_OFFSET, &BadNumber, 4);
        SamplerNtpSyncTest::sampler = NULL;
    }

    virtual void TearDown() {}

};

uint16_t SamplerTest::samplesReceived[MAX_DATA_ELEMENTS];
uint16_t SamplerTest::_nSamples;
uint16_t SamplerTest::measurementsReceived[MAX_DATA_ELEMENTS];
uint16_t SamplerTest::_nMeasurements;
uint16_t SamplerTest::returnedSample ;
uint16_t SamplerTest::returnedMeasurement ;
bool SamplerTest::sampleCalled;
bool SamplerTest::measurementCalled;
bool SamplerTest::transmitCalled;
unsigned long SamplerNtpSyncTest::presyncMillis;
unsigned long SamplerNtpSyncTest::postsyncMillis;
uint32_t SamplerNtpSyncTest::syncTimeSeconds;
Sampler* SamplerNtpSyncTest::sampler;


TEST_F(SamplerTest, SetupLoadValidConfigFromMemory) {
    Configuration config;
    Configuration savedConfig;
    Sampler sampler(config);

    config.setParameters(0,0,0,0);
    savedConfig.setParameters(9000000, 15000, 5, 2);
    savedConfig.setVersion(10);
    savedConfig.save();

    sampler.setup();
    
    char msg[250];
    config.populateStatusMsg(msg,sizeof(msg));
    ASSERT_STRCASEEQ(
        "Version: 10, counter: 1, measurementInterval: 9000000, sampleInterval: 15000, nSamples: 5, transmitFrequency: 2, calibration: 1.000000",
        msg);
}

TEST_F(SamplerTest, SetupDoesntLoadInvalidConfigFromMemory) {
    Configuration config;
    Configuration savedConfig;
    Sampler sampler(config);

    savedConfig.setParameters(9000000, 15000, 5, 2);
    savedConfig.setVersion(10);
    savedConfig.save();
    uint32_t BadNumber = 0xDEADDEAD;
    ESP.rtcUserMemoryWrite(32, &BadNumber, 4);

    config.setParameters(3600000,10000,3,1);
    config.setVersion(2);

    sampler.setup();
    
    char msg[250];
    config.populateStatusMsg(msg,sizeof(msg));
    ASSERT_STRCASEEQ(
        "Version: 2, counter: 1, measurementInterval: 3600000, sampleInterval: 10000, nSamples: 3, transmitFrequency: 1, calibration: 1.000000",
        msg);
}

TEST_F(SamplerTest, SetupInitialisesDataWhenConfigNotDerivedFromMemory) {
    Configuration config;
    Sampler sampler(config);

    config.setParameters(3600000,10000,3,1);
    config.setVersion(2);
    uint16_t* data = config.getData();
    for (int i=0; i < MAX_DATA_ELEMENTS; i++) data[i] = i;
    ASSERT_EQ(data[121], 121);

    sampler.setup();
    for(int i=0; i < MAX_DATA_ELEMENTS; i++) {
        ASSERT_EQ(data[i],0);
    }
}

TEST_F(SamplerTest, LoopIncrementsConfigurationCounter) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(1000,0,1,1);
    
    sampler.setup();
    for (int i=1; i <= 100; i++) {
        ASSERT_EQ(i, config.getCounter());
        sampler.loop();
    }
}

TEST_F(SamplerTest, LoopSleepsForAppropriateAmountOfTime1) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(60000,0,1,1);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        uint64_t actual = ESP.getSleepTime();
        ASSERT_EQ(actual, (uint64_t) 60000000);
    }
}

TEST_F(SamplerTest, LoopSleepsForAppropriateAmountOfTime2) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(3600000,1000,5,3);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3596000000);
    }
}

TEST_F(SamplerTest, LoopSleepsForAppropriateAmountOfTime3) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(21600000,5000,3,2);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 5000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 5000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3590000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
    }
}

TEST_F(SamplerTest, LoopSleepsForAppropriateAmountOfTime4) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(9000000,15000,5,2);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3540000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1800000000);
    }
}


TEST_F(SamplerTest,LoopResetsCounterWhenApproaching16BitLimit) {
    Configuration config;
    Parameters params;
    Sampler sampler(config);
    config.setParameters(9000000,15000,5,2);
    setVersionCounter(&config, 2, 65530);
    
    sampler.setup();
    config.populateParameters(&params);
    ASSERT_EQ(65530, params.counter);

    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65531, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65532, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65533, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65534, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(1, params.counter);
}

TEST_F(SamplerTest,LoopResetsCounterWhenApproaching16BitLimit2) {
    Configuration config;
    Parameters params;
    Sampler sampler(config);
    config.setParameters(86400000,0,1,1);
    setVersionCounter(&config, 2, 65515);
    
    sampler.setup();
    config.populateParameters(&params);
    ASSERT_EQ(65515, params.counter);

    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65516, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65517, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65518, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65519, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65520, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(1, params.counter);
}

TEST_F(SamplerTest,LoopResetsCounterWhenApproaching16BitLimit3) {
    Configuration config;
    Parameters params;
    Sampler sampler(config);
    config.setParameters(60000,0,1,1);
    setVersionCounter(&config, 2, 65530);
    
    sampler.setup();
    config.populateParameters(&params);
    ASSERT_EQ(65530, params.counter);

    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65531, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65532, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65533, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65534, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(65535, params.counter);
    
    sampler.loop();
    config.populateParameters(&params);
    ASSERT_EQ(1, params.counter);
}


TEST_F(SamplerTest, LoopSetsRFModeAppropriately1) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(60000,0,1,1);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 60000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
    }
}

TEST_F(SamplerTest, LoopSetsRFModeAppropriately2) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(3600000,1000,5,3);
    sampler.setup();
    int transmitCounter = 0;
    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        transmitCounter++;
        if (transmitCounter == 3) {
            ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
            transmitCounter = 0;
        } else {
            ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        }
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3596000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
    }
}

TEST_F(SamplerTest, LoopSetsRFModeAppropriately3) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(21600000,5000,3,2);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 5000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 5000000);
        if (c % 2 == 0) { 
            ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
        } else {
            ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        }
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3590000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
    }
}

TEST_F(SamplerTest, LoopSetsRFModeAppropriately4) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(9000000,15000,5,2);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        if (c % 2 == 0) { 
            ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
        } else {
            ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        }
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3540000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_EQ(ESP.getSleepMode(), RF_DISABLED);
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1800000000);
    }
}

TEST_F(SamplerTest, LoopSavesConfigBeforeSleep) {
    Configuration config;
    Sampler sampler(config);
    Parameters params;
    config.setParameters(60000,0,1,1);
    sampler.setup();

    sampler.loop();
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 60000000);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);

    config.populateParameters(&params);
    ASSERT_EQ(2, params.counter);

    config.setParameters(30000,1000,3,10);
    sampler.setup();
    sampler.loop();

    config.populateParameters(&params);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 60000000);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
    ASSERT_EQ(params.counter, 3);
    ASSERT_EQ(params.measurementInterval, 60000);
    ASSERT_EQ(params.sampleInterval, 0);
    ASSERT_EQ(params.nSamples,1);
    ASSERT_EQ(params.transmitFrequency, 1);
}


TEST_F(SamplerTest, LoopAccountsForProcessingTimeInSleepTime) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(60000,0,1,1);
    sampler.setup();

    sampler.loop();
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 60000000);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);

    ticks = 123;
    sampler.setup();

    ticks = 10123;
    sampler.loop();
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 50000000);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
}

TEST_F(SamplerTest, LoopSleepsForNoTimeForExcessiveProcessingTime) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(60000,0,1,1);
    sampler.setup();

    sampler.loop();
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 60000000);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);

    ticks = 123;
    sampler.setup();

    ticks = 60123;
    sampler.loop();
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 0);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);

    sampler.setup();
    ticks = 121123;
    sampler.loop();
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 0);
    ASSERT_EQ(ESP.getSleepMode(), RF_DEFAULT);
}

TEST_F(SamplerTest, LoopInvokesCallBackFunctionsAppropriately1) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(1000,0,1,1);
    
    sampler.onTakeSample(&SamplerTest::takeSample);
    sampler.onTakeMeasurement(&SamplerTest::takeMeasurement);
    sampler.onTransmit(&SamplerTest::transmit);
    sampler.setup();
    for (int i=1; i <= 100; i++) {
        SamplerTest::returnedSample = i;
        SamplerTest::returnedMeasurement = i*2;
        ASSERT_EQ(i, config.getCounter());
        sampler.loop();
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT1_CALLED(i);
        ASSERT_TRANSMIT1_CALLED(i*2);
    }
}

TEST_F(SamplerTest, LoopInvokesCallBackFunctionsAppropriately2) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(3600000,1000,5,3);

    sampler.onTakeSample(&SamplerTest::takeSample);
    sampler.onTakeMeasurement(&SamplerTest::takeMeasurement);
    sampler.onTransmit(&SamplerTest::transmit);
    sampler.setup();
    int transmitCounter = 0;
    for (int c=1; c < 100; c++) {
        SamplerTest::returnedSample = c;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();

        SamplerTest::returnedSample = c + 1;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();

        SamplerTest::returnedSample = c + 2;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();

        SamplerTest::returnedSample = c + 3;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 1000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();

        SamplerTest::returnedSample = c + 4;
        SamplerTest::returnedMeasurement = c+2;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3596000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT5_CALLED(c,c+1,c+2,c+3,c+4);
        transmitCounter++;
        if (transmitCounter == 3) {
            transmitCounter = 0;
            ASSERT_TRANSMIT3_CALLED(c,c+1,c+2);
        } else {
            ASSERT_TRANSMIT_NOT_CALLED();
        }
    }
}

TEST_F(SamplerTest, LoopInvokesCallBackFunctionsAppropriately3) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(21600000,5000,3,2);

    sampler.onTakeSample(&SamplerTest::takeSample);
    sampler.onTakeMeasurement(&SamplerTest::takeMeasurement);
    sampler.onTransmit(&SamplerTest::transmit);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        SamplerTest::returnedSample = c;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 5000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        
        SamplerTest::returnedSample = c + 1;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 5000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        
        SamplerTest::returnedSample = c + 2;
        SamplerTest::returnedMeasurement = c+1;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3590000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT3_CALLED(c,c+1,c+2);

        if (c % 2 == 0) { 
            ASSERT_TRANSMIT2_CALLED(c,c+1);
        } else {
            ASSERT_TRANSMIT_NOT_CALLED();
        }

        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();
        
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();
        
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3600000000);
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();
    }
}

TEST_F(SamplerTest, LoopInvokesCallBackFunctionsAppropriately4) {
    Configuration config;
    Sampler sampler(config);
    config.setParameters(9000000,15000,5,2);
    sampler.onTakeSample(&SamplerTest::takeSample);
    sampler.onTakeMeasurement(&SamplerTest::takeMeasurement);
    sampler.onTransmit(&SamplerTest::transmit);
    sampler.setup();

    for (int c=1; c < 100; c++) {
        SamplerTest::returnedSample = c;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        SamplerTest::returnedSample = c+1;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        SamplerTest::returnedSample = c+2;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        SamplerTest::returnedSample = c+3;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 15000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        SamplerTest::returnedSample = c+4;
        SamplerTest::returnedMeasurement = c+2;
        sampler.loop();
        ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 3540000000);
        ASSERT_TAKE_SAMPLE_CALLED();
        ASSERT_TAKE_MEASUREMENT5_CALLED(c,c+1,c+2,c+3,c+4);

        if (c % 2 == 0) { 
            ASSERT_TRANSMIT2_CALLED(c+1, c+2);
        } else {
            ASSERT_TRANSMIT_NOT_CALLED();
        }

        sampler.loop();
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();

        sampler.loop();
        ASSERT_TAKE_SAMPLE_NOT_CALLED();
        ASSERT_TAKE_MEASUREMENT_NOT_CALLED();
        ASSERT_TRANSMIT_NOT_CALLED();
    }
}

TEST_F(SamplerNtpSyncTest, SamplerConstructionInitialisesSyncronisation) {
    Configuration config;
    Sampler sampler(config);
    Synchronisation sync;

    config.populateSynchronisation(&sync);
    ASSERT_EQ(sync.syncTime, 0);
    ASSERT_EQ(sync.nominalElapsed, 0);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.000);
}


TEST_F(SamplerNtpSyncTest, RtcClockLagsBehindSyncedTime) {
    Configuration config;
    Sampler sampler(config);
    Synchronisation sync;
    config.setParameters(200000,0,1,1);
    sampler.setup();
    sampler.synchronise(1612100000);

    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
    ASSERT_EQ(sync.syncTime,1612100000);
    ASSERT_EQ(sync.nominalElapsed, 200);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 200000000);

    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
    ASSERT_EQ(sync.syncTime,1612100000);
    ASSERT_EQ(sync.nominalElapsed, 400);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 200000000);

    sampler.synchronise(1612100440);
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 0.90909090909);
    ASSERT_EQ(sync.syncTime,1612100440);
    ASSERT_EQ(sync.nominalElapsed, 160);
    //Assert sleeptime is 160 seconds*0.909090...
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 145455000);

    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 0.90909090909);
    ASSERT_EQ(sync.syncTime,1612100440);
    ASSERT_EQ(sync.nominalElapsed, 360);
    //Assert next sleep time is 200 * 0.909090909
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 181818000);

    sampler.synchronise(1612100804);
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 0.89910089910);
    ASSERT_EQ(sync.syncTime,1612100804);
    ASSERT_EQ(sync.nominalElapsed, 196);
    //Assert sleeptime is 200 - 4 seconds*0.900090...
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 176224000);

}

TEST_F(SamplerNtpSyncTest, RtcClockRunsAheadOfSyncedTime) {
    Configuration config;
    Sampler sampler(config);
    Synchronisation sync;
    config.setParameters(200000,0,1,1);
    sampler.setup();
    sampler.synchronise(1612100000);

    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
    ASSERT_EQ(sync.syncTime,1612100000);
    ASSERT_EQ(sync.nominalElapsed, 200);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 200000000);

    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
    ASSERT_EQ(sync.syncTime,1612100000);
    ASSERT_EQ(sync.nominalElapsed, 400);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 200000000);

    sampler.synchronise(1612100360);
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.11111111);
    ASSERT_EQ(sync.syncTime,1612100360);
    ASSERT_EQ(sync.nominalElapsed, 240);
    //Assert sleeptime is 200 + 40 seconds*1.1111111...
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 266667000);

    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.11111111);
    ASSERT_EQ(sync.syncTime,1612100360);
    ASSERT_EQ(sync.nominalElapsed, 440);
    //Assert next sleep time is 200 * 0.909090909
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 222222000);

    sampler.synchronise(1612100796);
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.1213047910);
    ASSERT_EQ(sync.syncTime,1612100796);
    ASSERT_EQ(sync.nominalElapsed, 204);
    //Assert sleeptime is 200 + 4 seconds*1.12233445566...
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 228746000);
}


TEST_F(SamplerNtpSyncTest, RtcClockRunsAheadOfSyncedTimeWithProcessingTime) {
    Configuration config;
    Sampler sampler(config);
    Synchronisation sync;
    config.setParameters(200000,0,1,1);
    ticks = 123;
    sampler.setup();
    sampler.synchronise(1612100000);

    ticks = 2123;
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
    ASSERT_EQ(sync.syncTime,1612100000);
    ASSERT_EQ(sync.nominalElapsed, 200);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 198000000);

    ticks = 4123;
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
    ASSERT_EQ(sync.syncTime,1612100000);
    ASSERT_EQ(sync.nominalElapsed, 400);
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 198000000);

    ticks = 6123;
    sampler.synchronise(1612100362);
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.11111111);
    ASSERT_EQ(sync.syncTime,1612100360);
    ASSERT_EQ(sync.nominalElapsed, 240);
    //Assert sleeptime is 200 + 40 -2 seconds*1.1111111...
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 264444000);

    ticks = 8123;
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.11111111);
    ASSERT_EQ(sync.syncTime,1612100360);
    ASSERT_EQ(sync.nominalElapsed, 440);
    //Assert next sleep time is 200 -2 * 1.111111111
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 220000000);

    ticks = 10123;
    sampler.synchronise(1612100798);
    sampler.loop();
    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.1213047910);
    ASSERT_EQ(sync.syncTime,1612100796);
    ASSERT_EQ(sync.nominalElapsed, 204);
    //Assert sleeptime is 200 + 4 -2  seconds*1.1213047910...
    ASSERT_EQ(ESP.getSleepTime(), (uint64_t) 226504000);
}



TEST_F(SamplerNtpSyncTest, RtcClockRunsAheadOfSyncedTimeLoop) {
    Configuration config;
    Sampler sampler(config);
    Synchronisation sync;
    config.setParameters(100000,10000,5,2);
    SamplerNtpSyncTest::presyncMillis = 4000;
    SamplerNtpSyncTest::postsyncMillis = 6000;
    SamplerNtpSyncTest::sampler = &sampler;
    sampler.onTransmit(&SamplerNtpSyncTest::transmit);
    uint32_t time = 3825000000;
    for (int i=1; i <= 120; i++) {
        ticks=0;
        sampler.setup();
        SamplerNtpSyncTest::presyncMillis = 4000;
        SamplerNtpSyncTest::postsyncMillis = 6000;
        SamplerNtpSyncTest::syncTimeSeconds = time + 4;
        sampler.loop();
        time += millis() /1000UL;
        time += (uint32_t)((unsigned long)round(ESP.getSleepTime() * 1.1) /1000000UL);
    }
    ASSERT_EQ(time, 3825002414);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
