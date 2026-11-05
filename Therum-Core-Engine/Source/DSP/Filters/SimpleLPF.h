#pragma once
#include <cmath>

class SimpleLPF
{
public:
    void prepare(double sampleRate);
    void setCutoff(float hz);
    void reset() { z1 = 0.0f; }
    float process(float x);

private:
    double sr = 44100.0;
    // -1 means "never set": the first setCutoff() always recomputes the
    // coefficient, so the filter never runs on the stale initial value.
    float cutoff = -1.0f;
    float a = 0.0f;
    float z1 = 0.0f;
};
