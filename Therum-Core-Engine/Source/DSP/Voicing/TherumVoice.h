#pragma once
#include "../Oscillators/WavetableOscillator.h"
#include "../Oscillators/WavetableBank.h"
#include "../Modulation/EnvelopeGenerator.h"
#include "../Filters/VoiceFilter.h"
#include "../FX/FxRack.h"

class TherumVoice
{
public:
    void prepare(double sampleRate, const WavetableBank* sharedBank);
    void start(int midiNote, float velocity);
    void stop();
    // wtPosA / wtPosB are the smoothed frame positions (0..1 across
    // the bank) for osc A / B. Applied per sample; the oscillator
    // clamps and no-ops on an unchanged value, so a settled position
    // costs nothing.
    float render(float filterCutoff, float filterReso, float macroAmount, float delayMix, float reverbMix, float distMix, float wtPosA, float wtPosB);

    // The voice filter type (SVF LowPass by default; Ladder selects
    // the Moog ladder). Applied to the filter on the next render;
    // a type change crossfades over ~3 ms, so it never clicks.
    void setFilterType(TherumFilterType t) { filter.setType(t); }

    // The morph mode/phase for each oscillator (spectral morph is
    // the quality mode; crossfade is the cheap default). Applied on
    // the next render; the oscillator change-detects, so a repeated
    // set is free, and a mode/phase change crossfades the outputs
    // over 128 samples, so it never clicks.
    void setMorphA(WavetableOscillator::MorphMode mode, WavetableOscillator::MorphPhase phase)
    { oscA.setMorphMode(mode); oscA.setMorphPhase(phase); }
    void setMorphB(WavetableOscillator::MorphMode mode, WavetableOscillator::MorphPhase phase)
    { oscB.setMorphMode(mode); oscB.setMorphPhase(phase); }

    // The amp-envelope ADSR (ms, ms, 0..1, ms). Applied to the
    // envelope on the next render; the envelope value is continuous,
    // so a mid-note change just changes the slope from here — it
    // never clicks. The default (10/120/0.75/220) is set in prepare().
    void setADSR(float aMs, float dMs, float s, float rMs) { ampEnv.setADSR(aMs, dMs, s, rMs); }

    bool isActive() const { return active; }
    int getMidiNote() const { return midiNote; }
    // Release time in seconds (the envelope's release), for tail reporting.
    float getReleaseSeconds() const { return ampEnv.getReleaseSeconds(); }

private:
    WavetableOscillator oscA;
    WavetableOscillator oscB;
    EnvelopeGenerator ampEnv;
    VoiceFilter filter; // the voice filter: SVF by default, Moog ladder on type
    FxRack fx;

    bool active = false;
    int midiNote = -1;
    float velocity = 0.0f;
};
