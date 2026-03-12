#include "WavetableBank.h"
#include <cmath>

void WavetableBank::buildDefaultSineTable(int size)
{
    table.resize((size_t)size);
    for (int i = 0; i < size; ++i)
    {
        const float phase = (float)i / (float)size;
        table[(size_t)i] = std::sin(phase * 6.28318530718f);
    }
}
