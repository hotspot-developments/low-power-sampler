#include <gtest/gtest.h>
#include "./fake/Esp.h"
#include "../src/Configuration.cpp"


TEST(ConfigurationTest, Construction) {
    Configuration config;
    Parameters params;
    Synchronisation sync;

    config.populateParameters(&params);
    ASSERT_EQ(params.counter, 1);
    ASSERT_EQ(params.currentVersion,0);
    ASSERT_EQ(params.measurementInterval,0);
    ASSERT_EQ(params.sampleInterval,0);
    ASSERT_EQ(params.nSamples,0);
    ASSERT_EQ(params.transmitFrequency,0);

    config.populateSynchronisation(&sync);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0);
}

TEST(ConfigurationTest, IncrementCounter) {
    Configuration config;
    config.setParameters(0,0,0,0);

    ASSERT_EQ(1, config.getCounter());
    
    for (uint16_t i=1; i < 1000; i++) {
        ASSERT_EQ(i, config.getCounter());
        config.incrementCounter();
    }
}

TEST(ConfigurationTest, ResetCounter) {
    Configuration config;
    config.setParameters(0,0,0,0);

    ASSERT_EQ(1, config.getCounter());
    
    for (uint16_t i=1; i < 10; i++) {
        ASSERT_EQ(i, config.getCounter());
        config.incrementCounter();
    }

    config.resetCounter();
    ASSERT_EQ(1, config.getCounter());
}



TEST(ConfigurationTest, SetParameters) {
    Configuration config;
    char msg[250];
    Parameters params;
    config.setParameters(3600000, 1000, 5, 3);
    config.setVersion(10);
    config.incrementCounter();
    config.populateStatusMsg(msg, 250);
    config.populateParameters(&params);

    ASSERT_STRCASEEQ(
        "Version: 10, counter: 2, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 3, calibration: 1.000000",
        msg);
    ASSERT_EQ(10, params.currentVersion);
    ASSERT_EQ(2, params.counter);
    ASSERT_EQ(3600000, params.measurementInterval);
    ASSERT_EQ(1000, params.sampleInterval);
    ASSERT_EQ(5, params.nSamples);
    ASSERT_EQ(3, params.transmitFrequency);
}

TEST(ConfigurationTest, SetParameters2) {
    Configuration config;
    char msg[250];
    Parameters params;
    config.setParameters(1000,0,1,1);
    config.setVersion(0);

    config.populateStatusMsg(msg, 250);
    config.populateParameters(&params);

    ASSERT_STRCASEEQ(
        "Version: 0, counter: 1, measurementInterval: 1000, sampleInterval: 0, nSamples: 1, transmitFrequency: 1, calibration: 1.000000",
        msg);
    ASSERT_EQ(0, params.currentVersion);
    ASSERT_EQ(1, params.counter);
    ASSERT_EQ(1000, params.measurementInterval);
    ASSERT_EQ(0, params.sampleInterval);
    ASSERT_EQ(1, params.nSamples);
    ASSERT_EQ(1, params.transmitFrequency);
}


TEST(ConfigurationTest, SaveConfiguration) {
    Configuration config;
    config.setParameters(3600000, 1000, 5, 3);
    bool saved = config.save();
    ASSERT_TRUE(saved);
    ASSERT_TRUE(config.checkMemory());
 }

TEST(ConfigurationTest, LoadConfiguration) {
    char msg[250];
    Configuration config;
    Configuration loadedConfiguration;
    loadedConfiguration.setParameters(0,0,0,0);
    loadedConfiguration.setVersion(0);
    config.setParameters(3600000, 1000, 5, 3);
    config.setVersion(1);
    config.incrementCounter();
    config.save();

    ASSERT_TRUE(config.checkMemory());
    loadedConfiguration.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 0, counter: 1, measurementInterval: 0, sampleInterval: 0, nSamples: 0, transmitFrequency: 0, calibration: 1.000000",
        msg);

    bool loaded = loadedConfiguration.fromMemory();
    ASSERT_TRUE(loaded);
    loadedConfiguration.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 1, counter: 2, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 3, calibration: 1.000000",
        msg);
}

TEST(ConfigurationTest, LoadUnsavedConfiguration) {
    Configuration config;
    uint32_t BadNumber = 0xDEADDEAD;
    ESP.rtcUserMemoryWrite(32, &BadNumber, 4);

    ASSERT_FALSE(config.checkMemory());
    ASSERT_FALSE(config.fromMemory());
}

TEST(ConfigurationTest, LoadFromJSON) {
    Configuration config;
    char msg[250];
    Parameters params;

    config.setParameters(3600000, 1000, 5, 3);
    config.setVersion(4);

    config.fromJson("{ measurementInterval: 21600000, sampleInterval: 5000, nSamples: 3, transmitFrequency:2, version: 16 }");
    config.populateStatusMsg(msg, 250);
    config.populateParameters(&params);
    
    ASSERT_STRCASEEQ(
        "Version: 16, counter: 1, measurementInterval: 21600000, sampleInterval: 5000, nSamples: 3, transmitFrequency: 2, calibration: 1.000000",
        msg);
    ASSERT_EQ(16, params.currentVersion);
    ASSERT_EQ(1, params.counter);
    ASSERT_EQ(21600000, params.measurementInterval);
    ASSERT_EQ(5000, params.sampleInterval);
    ASSERT_EQ(3, params.nSamples);
    ASSERT_EQ(2, params.transmitFrequency);
}

TEST(ConfigurationTest, LoadOneFromJSON) {
    Configuration config;
    char msg[250];

    config.setParameters(3600000, 1000, 5, 3);
    config.setVersion(4);
    config.incrementCounter();
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 4, counter: 2, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 3, calibration: 1.000000",
        msg);

    config.fromJson("version: 7");
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 3, calibration: 1.000000",
        msg);

    config.fromJson("transmitFrequency: 7");
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 7, calibration: 1.000000",
        msg);

    config.fromJson("measurementInterval: 7200000");
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 7200000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 7, calibration: 1.000000",
        msg);

    config.fromJson("nSamples: 10");
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 7200000, sampleInterval: 1000, nSamples: 10, transmitFrequency: 7, calibration: 1.000000",
        msg);

    config.fromJson("sampleInterval: 2000");
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 7200000, sampleInterval: 2000, nSamples: 10, transmitFrequency: 7, calibration: 1.000000",
        msg);
}

TEST(ConfigurationTest, DontLoadCounterFromJSON) {
    Configuration config;
    char msg[250];

    config.setParameters(3600000, 1000, 5, 3);
    config.setVersion(7);
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 3, calibration: 1.000000",
        msg);

    config.fromJson("counter: 2");
    config.populateStatusMsg(msg, 250);
    ASSERT_STRCASEEQ(
        "Version: 7, counter: 1, measurementInterval: 3600000, sampleInterval: 1000, nSamples: 5, transmitFrequency: 3, calibration: 1.000000",
        msg);
    ASSERT_EQ(1, config.getCounter());
}

TEST(ConfigurationTest, GetPointerToData) {
    Configuration config;
    Configuration restoredConfig;

    config.setParameters(3600000, 1000, 5, 3);
    uint16_t* values = config.getData();
    for (uint16_t i=0; i < MAX_DATA_ELEMENTS; i++) {
        values[i] = i;
    }

    config.save();

    restoredConfig.fromMemory();
    values = restoredConfig.getData();
    for (uint16_t i=0; i < MAX_DATA_ELEMENTS; i++) {
        ASSERT_EQ(i, values[i]);
    }

}

TEST(ConfigurationTest, LoadConfigurationLoadsDataFromMemory) {
    char msg[250];
    Configuration config;
    Configuration loadedConfiguration;
    loadedConfiguration.setParameters(0,0,0,0);
    loadedConfiguration.setVersion(0);
    uint16_t* loadedData = loadedConfiguration.getData();
    for (int i=0; i< MAX_DATA_ELEMENTS; i++) loadedData[i] = 0;

    uint16_t* data = config.getData();
    for (int i=0; i < MAX_DATA_ELEMENTS; i++) data[i] = i+1;

    config.save();
    ASSERT_TRUE(config.checkMemory());
    ASSERT_EQ(loadedData[42], 0);

    bool loaded = loadedConfiguration.fromMemory();
    ASSERT_TRUE(loaded);
    for(int i=0; i < MAX_DATA_ELEMENTS; i++) {
        ASSERT_EQ(loadedData[i], i+1);
    }
}


TEST(ConfigurationTest, ResetSynchronizationSetsMemberData) {
    Configuration config;
    Synchronisation sync;
    
    config.resetSynchronisation(121343565, 1.0001);
    config.populateSynchronisation(&sync);

    ASSERT_EQ(sync.syncTime, 121343565);
    ASSERT_EQ(sync.nominalElapsed, 0);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0001);
}

TEST(ConfigurationTest, LoadConfigurationLoadsSynchronizationDataFromMemory) {
    char msg[250];
    Configuration config;
    Configuration loadedConfiguration;
    Synchronisation sync;
    loadedConfiguration.resetSynchronisation(0, 0.0);
    config.resetSynchronisation(121343565, 1.0005);
    config.incrementElapsed(1200000);

    loadedConfiguration.populateSynchronisation(&sync);
    ASSERT_EQ(sync.syncTime, 0);
    ASSERT_EQ(sync.nominalElapsed, 0);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 0.0);

    config.save();

    ASSERT_TRUE(config.checkMemory());
 
    bool loaded = loadedConfiguration.fromMemory();
    ASSERT_TRUE(loaded);

    loadedConfiguration.populateSynchronisation(&sync);
    ASSERT_EQ(sync.syncTime, 121343565);
    ASSERT_EQ(sync.nominalElapsed, 1200);
    ASSERT_FLOAT_EQ(sync.calibrationFactor, 1.0005);
}




int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}