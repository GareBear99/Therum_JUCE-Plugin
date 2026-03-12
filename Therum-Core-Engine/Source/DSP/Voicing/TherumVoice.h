#pragma once
#include "../Oscillators/WavetableOscillator.h"
#include "../Oscillators/WavetableBank.h"
#include "../Modulation/EnvelopeGenerator.h"
#include "../Filters/SimpleLPF.h"
#include "../FX/FxRack.h"

class TherumVoice
{
public:
    void prepare(double sampleRate, const WavetableBank* sharedBank);
    void start(int midiNote, float velocity);
    void stop();
    float render(float filterCutoff, float macroAmount, float delayMix, float reverbMix, float distMix);

    bool isActive() const { return active; }
    int getMidiNote() const { return midiNote; }

private:
    WavetableOscillator oscA;
    WavetableOscillator oscB;
    EnvelopeGenerator ampEnv;
    SimpleLPF lpf;
    FxRack fx;

    bool active = false;
    int midiNote = -1;
    float velocity = 0.0f;
};
