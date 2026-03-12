#include "NoiseOscillator.h"
float NoiseOscillator::process()
{
    state = state * 1664525u + 1013904223u;
    return ((state >> 8) & 0xFFFF) / 32768.0f - 1.0f;
}
