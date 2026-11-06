#include "EnvelopeGenerator.h"
#include <algorithm>

void EnvelopeGenerator::setADSR(float aMs, float dMs, float s, float rMs)
{
    attackMs = std::max(1.0f, aMs);
    decayMs = std::max(1.0f, dMs);
    sustain = std::clamp(s, 0.0f, 1.0f);
    releaseMs = std::max(1.0f, rMs);
}

void EnvelopeGenerator::noteOn() { stage = 1; }
void EnvelopeGenerator::noteOff() { stage = 4; }

float EnvelopeGenerator::process()
{
    const float aStep = 1.0f / (float)(attackMs * 0.001 * sr);
    const float dStep = (1.0f - sustain) / (float)(decayMs * 0.001 * sr);
    const float rStep = 1.0f / (float)(releaseMs * 0.001 * sr);

    switch (stage)
    {
        case 1: value += aStep; if (value >= 1.0f) { value = 1.0f; stage = 2; } break;
        case 2: value -= dStep; if (value <= sustain) { value = sustain; stage = 3; } break;
        case 3:
            // Sustain: hold the level — gliding to it over ~10 ms if
            // the sustain parameter changed mid-note. A plain
            // `value = sustain` would jump the output in one sample
            // on a sustain change (a click on an automatable
            // parameter); the glide keeps it continuous. On entry
            // from decay the value already equals sustain, so the
            // glide is a no-op and the settled level is exact.
            {
                const float step = 1.0f / (float)(0.010 * sr);
                if (value < sustain) value = std::min(value + step, sustain);
                else if (value > sustain) value = std::max(value - step, sustain);
            }
            break;
        case 4: value -= rStep; if (value <= 0.0f) { value = 0.0f; stage = 0; } break;
        default: value = 0.0f; break;
    }
    return value;
}
