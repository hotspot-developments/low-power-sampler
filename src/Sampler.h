// MIT License

// Low Power Sampler - Use to take sensor readings at configured intervals.
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
    Synchronisation sync;
    unsigned long initialTime;
    uint32_t d, y, x;
    float offset;
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