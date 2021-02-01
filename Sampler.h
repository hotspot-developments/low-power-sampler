#ifndef SAMPLER_H
#define SAMPLER_H

#include <functional>
#include "Configuration.h"

using SampleCallBack = std::function<uint16_t()>;
using MeasurementCallBack = std::function<uint16_t(uint16_t*, uint32_t)>;
using TransmitCallBack = std::function<void(uint16_t*, uint32_t)>;

class Sampler {

    private:
    Configuration* configuration;
    Parameters params;
    uint32_t n, d, y, x, t;
    unsigned long initialTime;
    bool isTransmitDue(int32_t c);
    bool isSampleDue(int32_t c);
    bool isMeasurementDue(int32_t c);
    uint32_t calculateSleepTime(uint16_t counter);

    // Callbacks
    SampleCallBack cbTakeSample;
    MeasurementCallBack cbTakeMeasurement;
    TransmitCallBack cbTransmit;

    public:
    Sampler(Configuration& config);
    void setup();
    void loop();
    void onTakeSample(SampleCallBack fnSample);
    void onTakeMeasurement(MeasurementCallBack fnMeasurement);
    void onTransmit(TransmitCallBack fnTransmit);
    void synchronise(uint32_t timeInSeconds);
};

#endif // SAMPLER_H