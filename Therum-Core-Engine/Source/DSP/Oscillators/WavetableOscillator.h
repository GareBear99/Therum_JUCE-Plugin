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
    // How the frame position is rendered (paper section 4.1). Crossfade
    // is the cheap default (per-sample time-domain blend). Spectral is
    // the quality mode: the position is rendered by interpolating the
    // frames' spectra and IFFT'ing, block-rate (see recomputeMorph).
    enum class MorphMode
    {
        Crossfade,
        Spectral
    };
    enum class MorphPhase
    {
        None,            // blend the complex bins directly
        ZeroFundamental, // rotate frame B so its fundamental phase
                         // matches A's, then blend (aligns the frames'
                         // overall timing before the blend)
        ZeroAll          // blend magnitudes only; the morphed frame is
                         // zero-phase (all harmonics are cosines)
    };
    void setMorphMode(MorphMode mode);
    void setMorphPhase(MorphPhase phaseMode);
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
    // How many times the morphed frame was rebuilt (spectral morph).
    int getMorphRecomputeCount() const { return morphRecomputes; }
    // Samples remaining in the morph-switch crossfade (0 = no crossfade
    // in progress). A mode/phase change crossfades the outputs over
    // kMorphXfadeSamples so the switch never clicks, like the voice
    // filter's type crossfade (VoiceFilter::kCrossfadeSamples = 128).
    int getMorphXfadeRemaining() const { return morphXfade; }

private:
    // Picks the two mip levels for the current frequency and caches the
    // crossfade. Called from setFrequency/prepare/setBank (the selection
    // depends only on f0 and the sample rate, so it is never recomputed
    // per sample; call setFrequency whenever the pitch changes).
    void selectMip();
    // Rebuilds the morphed frame for the current position (spectral
    // morph): interpolate the adjacent frames' spectra, IFFT, then
    // rebuild only the two mip levels currently selected.
    void recomputeMorph();
    // Rebuilds only the morphed frame's two mip levels (the frame itself
    // is unchanged — used when the frequency moved the mip selection).
    // The overload takes the morphed frame's spectrum directly, so
    // recomputeMorph does not re-FFT the frame it just built.
    void rebuildMorphMips();
    void rebuildMorphMips(const std::vector<std::complex<float>>& morphed);
    // One read from the morphed frame (its mip levels when they match
    // the current selection, else the base morphed frame).
    float readMorphFrame() const;
    // One read from the frame before a morph mode/phase switch (the
    // crossfade source). Mirrors readMorphFrame on the preserved
    // previous frame and its mip levels; falls back to the base
    // previous frame when the mip selection has moved since.
    float readPrevMorphFrame() const;
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
    // Spectral morph (the quality mode). The morphed frame is rebuilt
    // lazily, at block rate: setPosition only marks it dirty, and
    // process() rebuilds it once at least kMorphMinSamples have passed
    // since the last rebuild. Rebuilding costs ~4 FFTs of 2048 samples,
    // so it is never done per sample.
    MorphMode morphMode = MorphMode::Crossfade;
    MorphPhase morphPhase = MorphPhase::None;
    static constexpr int kMorphMinSamples = 32;
    bool morphDirty = true;
    int samplesSinceMorph = kMorphMinSamples;
    float morphPos = -1.0f; // position the morphed frame was built at
    int morphMipLo = -1;    // mip levels the morphed frame was built for
    int morphMipHi = -1;
    std::vector<float> morphFrame; // 2048 samples, the morphed frame
    WavetableBank::MipLevel morphLo; // its two mip levels
    WavetableBank::MipLevel morphHi;
    int morphRecomputes = 0;
    // Morph-switch crossfade (same standard as the voice filter's
    // type crossfade). A mode/phase change rebuilds the morphed frame
    // from different spectra, so the read would jump from one
    // waveshape to another in a single sample — a click. The rebuild
    // preserves the previous frame (and its mip levels) and the
    // output crossfades from it over kMorphXfadeSamples. Position
    // moves rebuild the frame too, but they do NOT crossfade: the
    // position is a continuous glide, and crossfading every rebuild
    // would smear it. Only a mode/phase change arms the crossfade
    // (morphSwitchPending, set by setMorphMode/setMorphPhase).
    static constexpr int kMorphXfadeSamples = 128;
    bool morphSwitchPending = false;
    int morphXfade = 0; // samples remaining in the crossfade
    std::vector<float> prevMorphFrame; // the frame before the switch
    WavetableBank::MipLevel prevMorphLo; // its two mip levels
    WavetableBank::MipLevel prevMorphHi;
    int prevMorphMipLo = -1; // mip levels the previous frame was built for
    int prevMorphMipHi = -1;
};
