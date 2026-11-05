#pragma once
#include <cmath>
#include <algorithm>

// Simple peak limiter (framework-free, plain C++).
//
// Sits on the summed voice output so the mix can never clip the host, even
// with all voices sounding at full velocity. Instant attack (the gain drops
// the moment the signal exceeds the threshold) with a slow release back to
// unity, so a loud transient is caught and the gain recovers smoothly
// instead of pumping.
//
// This is a safety limiter, not a mastering effect: it only acts when the
// signal would otherwise exceed the threshold. A mix that stays below the
// threshold passes through untouched (gain stays at 1.0).
class Limiter
{
public:
    void prepare(double sampleRate, float releaseMs = 100.0f)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        const float tauSeconds = releaseMs * 0.001f;
        releaseCoeff = 1.0f - std::exp(-1.0f / (float)(tauSeconds * sr));
        gain = 1.0f;
    }

    // Linear threshold, e.g. 0.99 keeps the output just below full scale.
    void setThreshold(float linearThreshold)
    {
        threshold = std::clamp(linearThreshold, 0.01f, 1.0f);
    }

    float getThreshold() const { return threshold; }

    float process(float x)
    {
        const float absx = std::fabs(x);
        if (absx > threshold && absx > 0.0f)
        {
            // Above threshold: only ever reduce the gain (instant attack).
            // The gain is already low enough, hold it — do NOT release here.
            // Releasing while the signal is above threshold would raise the
            // gain under a loud sample and let the output exceed the
            // threshold (the release step is a relative increase, and it is
            // largest exactly when the gain is smallest).
            const float needed = threshold / absx;
            if (needed < gain)
                gain = needed;
        }
        else
        {
            // Below threshold: recover toward unity, slowly.
            gain += releaseCoeff * (1.0f - gain);
        }

        // Final safety: the gain maths above keeps the output at or below
        // the threshold, but float rounding can put it a hair over. Clamp
        // so the "never exceeds the threshold" guarantee is absolute.
        return std::clamp(x * gain, -threshold, threshold);
    }

    void reset() { gain = 1.0f; }

private:
    double sr = 44100.0;
    float threshold = 0.99f;
    float gain = 1.0f;
    float releaseCoeff = 0.0f;
};
