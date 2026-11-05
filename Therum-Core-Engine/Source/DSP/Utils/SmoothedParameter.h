#pragma once
#include <cmath>

// One-pole parameter smoother (framework-free, plain C++).
//
// A host or UI can move a parameter in a single block; without smoothing the
// audio jumps to the new value and zippers. This smoother advances one sample
// at a time toward its target with a fixed time constant, so a knob or
// automation move becomes a short exponential glide instead of a click.
//
// Usage: prepare() once, setTarget() whenever the parameter changes,
// process() once per sample. The first setTarget() snaps the value so there
// is no ramp from zero on the first block.
class SmoothedParameter
{
public:
    // timeConstantMs: the one-pole time constant. 10 ms is fast enough to
    // feel immediate and slow enough to kill zipper noise on cutoff/gain.
    void prepare(double sampleRate, float timeConstantMs = 10.0f)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        const float tauSeconds = timeConstantMs * 0.001f;
        coeff = 1.0f - std::exp(-1.0f / (float)(tauSeconds * sr));
        initialised = false;
    }

    void setTarget(float newTarget)
    {
        target = newTarget;
        if (!initialised)
        {
            value = newTarget;
            initialised = true;
        }
    }

    // Snap the current value (use on prepare, or when a jump is wanted).
    void setValueImmediate(float newValue)
    {
        value = newValue;
        target = newValue;
        initialised = true;
    }

    float process()
    {
        value += coeff * (target - value);
        return value;
    }

    float getValue() const { return value; }
    float getTarget() const { return target; }

private:
    double sr = 44100.0;
    float coeff = 1.0f;
    float target = 0.0f;
    float value = 0.0f;
    bool initialised = false;
};
