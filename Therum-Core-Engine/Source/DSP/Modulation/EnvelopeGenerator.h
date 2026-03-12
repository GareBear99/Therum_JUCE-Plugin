#pragma once
class EnvelopeGenerator
{
public:
    void prepare(double sampleRate) { sr = sampleRate; }
    void setADSR(float aMs, float dMs, float s, float rMs);
    void noteOn();
    void noteOff();
    float process();

private:
    double sr = 44100.0;
    float attackMs = 10.0f;
    float decayMs = 120.0f;
    float sustain = 0.75f;
    float releaseMs = 220.0f;

    int stage = 0; // 0 idle, 1 attack, 2 decay, 3 sustain, 4 release
    float value = 0.0f;
};
