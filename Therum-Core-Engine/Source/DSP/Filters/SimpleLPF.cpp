#include "SimpleLPF.h"
#include <algorithm>

void SimpleLPF::prepare(double sampleRate)
{
    sr = sampleRate;
    z1 = 0.0f;
    cutoff = -1.0f; // force the coefficient to be recomputed on first use
    a = 0.0f;
}

void SimpleLPF::setCutoff(float hz)
{
    // Ignore non-finite values: keep the previous cutoff rather than
    // letting a NaN or infinity poison the coefficient.
    if (!std::isfinite(hz))
        return;

    // Clamp to a sane range. An unclamped cutoff above Nyquist (or below
    // zero) makes the coefficient wrong and can push the filter unstable.
    const float nyquist = (float)sr * 0.5f;
    const float clamped = std::clamp(hz, 20.0f, std::min(20000.0f, nyquist));

    // Only recompute the coefficient when the cutoff actually changed.
    // This is called once per sample per voice; the exp() is only needed
    // when the value moves, not on every sample of a static cutoff.
    if (clamped == cutoff)
        return;

    cutoff = clamped;
    a = std::exp(-6.28318530718f * cutoff / (float)sr);
}

float SimpleLPF::process(float in)
{
    z1 = (1.0f - a) * in + a * z1;
    return z1;
}
