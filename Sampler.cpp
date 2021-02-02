#include "Sampler.h"
#include <limits.h>
#include <Esp.h>
#include <math.h>

#define MAX_SLEEP_TIME_MS 3600000

unsigned long millis();

Sampler::Sampler(Configuration& config) {
    this->configuration = &config;
    config.resetSynchronisation(0,1.0);
}

void Sampler::setup() {
    Parameters params;
    Synchronisation sync;
    this->initialTime = millis();
    if (this->configuration->checkMemory()) {
        this->configuration->fromMemory();
    } else {
        uint16_t* data = this->configuration->getData();
        for (int i=0; i < MAX_DATA_ELEMENTS; i++) data[i]=0;
    }
    this->configuration->populateParameters(&params);
    this->configuration->populateSynchronisation(&sync);
    this->n = params.nSamples;
    this->t = params.transmitFrequency;
    this->d = ((params.measurementInterval - 1) / MAX_SLEEP_TIME_MS) + 1;
    this->y = params.nSamples + this->d - 1;
    this->x = params.transmitFrequency * this->y;
    this->m = params.measurementInterval;
    this->s = params.sampleInterval;
    this->l = sync.syncTime;
    this->e = sync.nominalElapsed;
    this->f = sync.calibrationFactor;
    this->o = 0.0;
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
        uint32_t index = this->n + ((counter + (this->d -1) + (this->x - this->y)) / this->y) % this->t; 
        data[index] = this->cbTakeMeasurement(data,this->n);
    }
    if (this->cbTransmit && isTransmitDue(counter)) {
        this->cbTransmit(data + this->n, this->t);
    }
    uint32_t nominalSleepTime = calculateSleepTime(counter);
    this->configuration->incrementElapsed(nominalSleepTime);
    this->configuration->save();

    unsigned long processingTime = millis() - initialTime;
    unsigned long sleepTime = nominalSleepTime - ((unsigned long)(this->o*1000UL) + processingTime);
    sleepTime = sleepTime < 0 ? 0: sleepTime;
    ESP.deepSleep((unsigned long)round(sleepTime*this->f)*1000UL, isTransmitDue(counter+1)?RF_DEFAULT:RF_DISABLED);
    this->setup();
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
    if (cyclePos < (this->n -1)) {
        sleepTime = this->s;
    } else if (cyclePos == (this->n -1)) {
        if (this->d > 1) {
            sleepTime = MAX_SLEEP_TIME_MS - (this->n-1)*this->s;
        } else {
            sleepTime = this->m - (this->n-1)*this->s;
        }
    } else if (c % this->y == 0) {
        sleepTime = this->m % MAX_SLEEP_TIME_MS;
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
    if (this->l != 0) {
        float actualElapsed = timeInSeconds - this->l;
        this->o = actualElapsed - this->e;
        this->f *= (1.0 - this->o/actualElapsed);
        this->configuration->resetSynchronisation(timeInSeconds, this->f);
    } else {
        this->configuration->resetSynchronisation(timeInSeconds, 1.0);
    }
}