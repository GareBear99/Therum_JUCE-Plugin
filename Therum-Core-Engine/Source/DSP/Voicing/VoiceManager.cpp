#include "VoiceManager.h"

void VoiceManager::prepare(double sampleRate, int voices)
{
    bank.buildDefaultSineTable(2048);
    pool.resize((size_t)voices);
    for (auto& v : pool)
        v.prepare(sampleRate, &bank);
}

TherumVoice* VoiceManager::findFreeVoice()
{
    for (auto& v : pool)
        if (!v.isActive())
            return &v;
    return pool.empty() ? nullptr : &pool.front();
}

TherumVoice* VoiceManager::findVoiceByNote(int midiNote)
{
    for (auto& v : pool)
        if (v.isActive() && v.getMidiNote() == midiNote)
            return &v;
    return nullptr;
}

void VoiceManager::noteOn(int midiNote, float velocity)
{
    if (auto* v = findFreeVoice())
        v->start(midiNote, velocity);
}

void VoiceManager::noteOff(int midiNote)
{
    if (auto* v = findVoiceByNote(midiNote))
        v->stop();
}

void VoiceManager::allNotesOff()
{
    for (auto& v : pool)
        if (v.isActive())
            v.stop();
}

float VoiceManager::renderMixed(float filterCutoff, float macroAmount, float delayMix, float reverbMix, float distMix)
{
    float mix = 0.0f;
    for (auto& v : pool)
        mix += v.render(filterCutoff, macroAmount, delayMix, reverbMix, distMix);
    return mix;
}

int VoiceManager::getActiveVoiceCount() const
{
    int n = 0;
    for (const auto& v : pool)
        if (v.isActive())
            ++n;
    return n;
}
