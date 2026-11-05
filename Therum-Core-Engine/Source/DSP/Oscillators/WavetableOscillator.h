#pragma once
#include "WavetableBank.h"

class WavetableOscillator
{
public:
    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setPosition(float normalized);
    void setBank(const WavetableBank* wtBank);
    // Reset the phase to zero. Called on note start so every retrigger
    // begins at the same point in the table (no click from a stale phase).
    void resetPhase() { phase = 0.0f; }
    float process();

private:
    double sr = 44100.0;
    float freq = 220.0f;
    float pos = 0.0f;
    float phase = 0.0f;
    const WavetableBank* bank = nullptr;
};
