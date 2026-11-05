#pragma once
// Wavetable oscillator (paper section 6, rank 2). Reads the shared
// WavetableBank: up to 256 frames of 2048 samples, frame position
// interpolated by crossfade, cubic/Hermite within-frame interpolation
// (linear kept as the Eco quality mode), mipmaps selected per frequency
// and crossfaded (paper section 4.1).
//
// The oscillator never owns table data: the bank is shared (the voice
// manager holds it) and the oscillator only reads it. The bank must
// outlive the oscillator.
#include "WavetableBank.h"

class WavetableOscillator
{
public:
    enum class Interpolation
    {
        Hermite, // cubic Catmull-Rom: the default (paper section 4.1)
        Linear   // Eco quality mode: cheaper, measurably less accurate
    };

    void prepare(double sampleRate);
    void setFrequency(float hz);
    // Frame position, 0..1 across the bank (0 = first frame, 1 = last).
    // Crossfades between adjacent frames; was a no-op in the scaffold.
    void setPosition(float normalized);
    void setBank(const WavetableBank* wtBank);
    void setInterpolation(Interpolation mode) { interp = mode; }
    // Reset the phase to zero. Called on note start so every retrigger
    // begins at the same point in the table (no click from a stale phase).
    void resetPhase() { phase = 0.0; }
    float process();

    // Test/verification hooks.
    float getPosition() const { return pos; }
    float getFrequency() const { return freq; }
    // Cached mip selection for the current frequency: the two level
    // indices being crossfaded and the blend (0 = all lo, 1 = all hi).
    // -1 means the bank has no mipmaps.
    int getMipLo() const { return mipLo; }
    int getMipHi() const { return mipHi; }
    float getMipAlpha() const { return mipAlpha; }

private:
    // Picks the two mip levels for the current frequency and caches the
    // crossfade. Called from setFrequency/prepare/setBank (the selection
    // depends only on f0 and the sample rate, so it is never recomputed
    // per sample; call setFrequency whenever the pitch changes).
    void selectMip();
    // One table read at a fractional index (wraps). Hermite is 4-point
    // Catmull-Rom; Linear is the 2-point Eco read.
    static float readHermite(const float* table, int size, float index);
    static float readLinear(const float* table, int size, float index);
    float readTable(const float* table, int size, float index) const
    {
        return interp == Interpolation::Hermite
                   ? readHermite(table, size, index)
                   : readLinear(table, size, index);
    }

    double sr = 44100.0;
    float freq = 220.0f;
    float pos = 0.0f;
    // Double phase accumulator. A float accumulator drifts ~2e-4 cycles
    // over one second of 44.1 kHz rendering (systematic rounding over
    // 44,100 additions) — inaudible as pitch, but it swamps the
    // interpolation error the tests measure, and it costs one double add
    // per sample. The table index is still float.
    double phase = 0.0;
    Interpolation interp = Interpolation::Hermite;
    const WavetableBank* bank = nullptr;
    // Cached mip selection (see selectMip). -1 = bank has no mipmaps.
    int mipLo = -1;
    int mipHi = -1;
    float mipAlpha = 0.0f;
};
