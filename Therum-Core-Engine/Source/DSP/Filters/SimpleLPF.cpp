#include "SimpleLPF.h"

void SimpleLPF::setCutoff(float hz)
{
    cutoff = hz;
    const float x = std::exp(-6.28318530718f * cutoff / (float)sr);
    a = x;
}

float SimpleLPF::process(float in)
{
    z1 = (1.0f - a) * in + a * z1;
    return z1;
}
