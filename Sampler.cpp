#include "Sampler.h"
#include <limits.h>
#include <Esp.h>

#define MAX_SLEEP_TIME_MS 3600000

unsigned long millis();

Sampler::Sampler(Configuration& config) {
    this->configuration = &config;
    config.resetSynchronisation(0,1.0);
}

void Sampler::setup() {
    this->initialTime = millis();
    if (this->configuration->checkMemory()) {
        this->configuration->fromMemory();
    } else {
        uint16_t* data = this->configuration->getData();
        for (int i=0; i < MAX_DATA_ELEMENTS; i++) data[i]=0;
    }
    this->configuration->populateParameters(&params);
    this->n = params.nSamples;
    this->t = params.transmitFrequency;
    this->d = ((params.measurementInterval - 1) / MAX_SLEEP_TIME_MS) + 1;
    this->y = params.nSamples + this->d - 1;
    this->x = params.transmitFrequency * this->y;
}

void Sampler::loop() {
    uint16_t counter = this->configuration->getCounter();
    uint16_t* data = this->configuration->getData();
    if (counter % this->x == 0 && counter > (USHRT_MAX - this->x)) {
        this->configuration->resetCounter();
    } else { 
        this->configuration->incrementCounter();
    }
    this->configuration->save();

    if (this->cbTakeSample && isSampleDue(counter)) {
        data[(counter - 1) % this-> y ] = this->cbTakeSample();
    }
    if (this->cbTakeMeasurement && isMeasurementDue(counter)) {
        uint32_t index = this->n + ((counter + (this->d -1) + (this->x - this->y)) / this->y) % this->t; 
        data[index] = this->cbTakeMeasurement(data,this->n);
    }
    if (this->cbTransmit && isTransmitDue(counter)) {
        this->cbTransmit(data + this->n, this->t);
    }

    unsigned long sleepTime = calculateSleepTime(counter);
    unsigned long processingTime = millis() - initialTime;
    sleepTime = processingTime >= sleepTime? 0: sleepTime - processingTime;
    ESP.deepSleep(sleepTime*1000, isTransmitDue(counter+1)?RF_DEFAULT:RF_DISABLED);
    initialTime = millis();
}

void Sampler::onTakeSample(SampleCallBack fnSample) {
    this->cbTakeSample = fnSample;
}

void Sampler::onTakeMeasurement(MeasurementCallBack fnMeasurement) {
    this->cbTakeMeasurement = fnMeasurement;
}

void Sampler::onTransmit(TransmitCallBack fnTransmit) {
    this->cbTransmit = fnTransmit;
}

uint32_t Sampler::calculateSleepTime(uint16_t c) {
    uint32_t sleepTime = 0;
    uint32_t cyclePos = (c -1) % this->y;
    if (cyclePos < (params.nSamples -1)) {
        sleepTime = params.sampleInterval;
    } else if (cyclePos == (params.nSamples -1)) {
        if (this->d > 1) {
            sleepTime = MAX_SLEEP_TIME_MS - (params.nSamples-1)*params.sampleInterval;
        } else {
            sleepTime = params.measurementInterval - (params.nSamples-1)*params.sampleInterval;
        }
    } else if (c % this->y == 0) {
        sleepTime = params.measurementInterval % MAX_SLEEP_TIME_MS;
        sleepTime = (sleepTime == 0)?MAX_SLEEP_TIME_MS:sleepTime;
    } else {
        sleepTime = MAX_SLEEP_TIME_MS;
    }
    return sleepTime;
}

bool Sampler::isTransmitDue(int32_t c) {
    return (c - (int32_t)(this->x - (this->d - 1))) % (int32_t)this->x == 0;
}

bool Sampler::isSampleDue(int32_t c) {
    return ((c - 1) % (int32_t)this->y ) < this->n;
}

bool Sampler::isMeasurementDue(int32_t c) {
    return ((c - (int32_t)this->n) % (int32_t)this->y) == 0;
}

void Sampler::synchronise(uint32_t timeInSeconds) {
    
}