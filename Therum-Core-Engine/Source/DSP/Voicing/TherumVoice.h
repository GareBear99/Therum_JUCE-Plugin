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
    float render(float filterCutoff, float filterReso, float macroAmount, float delayMix, float reverbMix, float distMix);

    // The voice filter type (SVF LowPass by default; Ladder selects
    // the Moog ladder). Applied to the filter on the next render;
    // a type change crossfades over ~3 ms, so it never clicks.
    void setFilterType(TherumFilterType t) { filter.setType(t); }

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
