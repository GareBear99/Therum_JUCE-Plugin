#pragma once
#include "WavetableBank.h"

class WavetableOscillator
{
public:
    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setPosition(float normalized);
    void setBank(const WavetableBank* wtBank);
    float process();

private:
    double sr = 44100.0;
    float freq = 220.0f;
    float pos = 0.0f;
    float phase = 0.0f;
    const WavetableBank* bank = nullptr;
};
