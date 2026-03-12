#include "WavetableOscillator.h"
#include <cmath>

void WavetableOscillator::prepare(double sampleRate) { sr = sampleRate; }
void WavetableOscillator::setFrequency(float hz) { freq = hz; }
void WavetableOscillator::setPosition(float normalized) { pos = normalized; (void)pos; }
void WavetableOscillator::setBank(const WavetableBank* wtBank) { bank = wtBank; }

float WavetableOscillator::process()
{
    if (bank == nullptr || !bank->isReady())
    {
        auto out = std::sin(phase * 6.28318530718f);
        phase += freq / (float)sr;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

    const auto& table = bank->getTable();
    const auto size = (int)table.size();

    float idx = phase * (float)size;
    int i0 = (int)idx % size;
    int i1 = (i0 + 1) % size;
    float frac = idx - (float)i0;

    float out = table[(size_t)i0] + (table[(size_t)i1] - table[(size_t)i0]) * frac;

    phase += freq / (float)sr;
    if (phase >= 1.0f) phase -= 1.0f;
    return out;
}
