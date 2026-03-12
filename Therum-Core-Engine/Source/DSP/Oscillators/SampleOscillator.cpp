#include "SampleOscillator.h"

float SampleOscillator::process()
{
    if (data == nullptr || data->empty())
        return 0.0f;

    const int size = (int)data->size();
    int i0 = (int)pos;
    if (i0 < 0) i0 = 0;
    if (i0 >= size) i0 = size - 1;

    float out = (*data)[(size_t)i0];
    pos += rate;
    if (pos >= (float)size) pos = 0.0f;
    return out;
}
