#include "Sampler.h"
#include <limits.h>
#include <math.h>
#include "Espx.h"
#include <Arduino.h>

#define MAX_SLEEP_TIME_MS 3600000

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
    this->configuration->populateSynchronisation(&sync);
    this->d = ((params.measurementInterval - 1) / MAX_SLEEP_TIME_MS) + 1;
    this->y = params.nSamples + this->d - 1;
    this->x = params.transmitFrequency * this->y;
    this->offset = 0.0;
}

void Sampler::loop() {
    uint16_t counter = this->configuration->getCounter();
    uint16_t* data = this->configuration->getData();
    if (counter % this->x == 0 && counter > (USHRT_MAX - this->x)) {
        this->configuration->resetCounter();
    } else { 
        this->configuration->incrementCounter();
    }

    if (this->cbTakeSample && isSampleDue(counter)) {
        data[(counter - 1) % this-> y ] = this->cbTakeSample();
    }
    if (this->cbTakeMeasurement && isMeasurementDue(counter)) {
        uint32_t index = params.nSamples + ((counter + (this->d -1) + (this->x - this->y)) / this->y) % params.transmitFrequency; 
        data[index] = this->cbTakeMeasurement(data,params.nSamples);
    }
    if (this->cbTransmit && isTransmitDue(counter)) {
        this->cbTransmit(data + params.nSamples, params.transmitFrequency);
    }
    uint32_t nominalSleepTime = calculateSleepTime(counter);
    long correctionTime = (long)(this->offset*1000UL);
    this->configuration->incrementElapsed(correctionTime >  (long) nominalSleepTime ? 0 : (nominalSleepTime - correctionTime));
    this->configuration->save();

    correctionTime += millis() - this->initialTime;
    unsigned long sleepTime = (correctionTime > (long) nominalSleepTime) ? 0 : (nominalSleepTime - correctionTime);
    Espx::deepSleep((unsigned long)round(sleepTime*sync.calibrationFactor)*1000UL, isTransmitDue(counter+1));
    this->setup();
}

void Sampler::synchronise(uint32_t timeInSeconds) {
    uint32_t processingSeconds = (millis() - this->initialTime)/1000;
    if (sync.syncTime != 0) {
        float actualElapsed = timeInSeconds - (sync.syncTime + processingSeconds);
        if (sync.nominalElapsed > 0) {
            this->offset = actualElapsed - sync.nominalElapsed;
            sync.calibrationFactor *= (1.0 - this->offset/actualElapsed);
        }
        this->configuration->resetSynchronisation(timeInSeconds - processingSeconds, sync.calibrationFactor);
    } else {
        this->configuration->resetSynchronisation(timeInSeconds - processingSeconds, 1.0);
    }
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
    return ((c - 1) % (int32_t)this->y ) < (int32_t)params.nSamples;
}

bool Sampler::isMeasurementDue(int32_t c) {
    return ((c - (int32_t)params.nSamples) % (int32_t)this->y) == 0;
}
