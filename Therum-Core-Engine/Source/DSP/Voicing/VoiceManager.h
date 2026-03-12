#pragma once
#include <vector>
#include "../Oscillators/WavetableBank.h"
#include "TherumVoice.h"

class VoiceManager
{
public:
    void prepare(double sampleRate, int voices);

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();

    float renderMixed(float filterCutoff, float macroAmount, float delayMix, float reverbMix, float distMix);
    int getActiveVoiceCount() const;

private:
    TherumVoice* findFreeVoice();
    TherumVoice* findVoiceByNote(int midiNote);

    WavetableBank bank;
    std::vector<TherumVoice> pool;
};
