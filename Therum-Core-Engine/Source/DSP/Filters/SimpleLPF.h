#pragma once
#include <cmath>

class SimpleLPF
{
public:
    void prepare(double sampleRate) { sr = sampleRate; }
    void setCutoff(float hz);
    float process(float x);

private:
    double sr = 44100.0;
    float cutoff = 18000.0f;
    float a = 0.99f;
    float z1 = 0.0f;
};
