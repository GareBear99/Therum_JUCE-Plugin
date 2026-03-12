#include "LfoGenerator.h"
#include <cmath>
float LfoGenerator::process()
{
    auto x = 0.5f + 0.5f * std::sin(phase * 6.28318530718f);
    phase += rate / (float)sr;
    if (phase >= 1.0f) phase -= 1.0f;
    return x;
}
