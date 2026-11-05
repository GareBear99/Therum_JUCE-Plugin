#include "VoiceManager.h"
#include <cmath>

void VoiceManager::prepare(double sampleRate, int voices)
{
    bank.buildDefaultSineTable(2048);
    pool.resize((size_t)voices);
    for (auto& v : pool)
        v.prepare(sampleRate, &bank);

    nextVoice = 0;

    cutoffSmooth.prepare(sampleRate, 10.0f);
    macroSmooth.prepare(sampleRate, 10.0f);
    delaySmooth.prepare(sampleRate, 10.0f);
    reverbSmooth.prepare(sampleRate, 10.0f);
    distSmooth.prepare(sampleRate, 10.0f);
    masterSmooth.prepare(sampleRate, 10.0f);

    limiter.prepare(sampleRate, 100.0f);
    limiter.setThreshold(0.99f);
}

TherumVoice* VoiceManager::findFreeVoice()
{
    if (pool.empty())
        return nullptr;

    // Prefer a free voice, scanning from the round-robin cursor so voices
    // are used evenly instead of always taking the first free slot.
    for (size_t i = 0; i < pool.size(); ++i)
    {
        const size_t idx = (nextVoice + i) % pool.size();
        if (!pool[idx].isActive())
        {
            nextVoice = (idx + 1) % pool.size();
            return &pool[idx];
        }
    }

    // All busy: steal round-robin. The cursor advances on every steal, so
    // consecutive steals take consecutive voices instead of always voice 0.
    TherumVoice* stolen = &pool[nextVoice];
    nextVoice = (nextVoice + 1) % pool.size();
    return stolen;
}

TherumVoice* VoiceManager::findVoiceByNote(int midiNote)
{
    for (auto& v : pool)
        if (v.isActive() && v.getMidiNote() == midiNote)
            return &v;
    return nullptr;
}

bool VoiceManager::isNoteActive(int midiNote) const
{
    for (const auto& v : pool)
        if (v.isActive() && v.getMidiNote() == midiNote)
            return true;
    return false;
}

void VoiceManager::noteOn(int midiNote, float velocity)
{
    // A note that is already sounding (including one in its release tail)
    // is retriggered on the same voice instead of starting a duplicate.
    // This is what stops the same note stacking up as two voices.
    if (auto* existing = findVoiceByNote(midiNote))
    {
        existing->start(midiNote, velocity);
        return;
    }

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

float VoiceManager::renderMixed(float filterCutoff, float macroAmount, float delayMix, float reverbMix, float distMix, float masterGainDb)
{
    // Smooth every parameter that reaches audio, once per sample. The
    // smoothers snap to their first target, so the first block has no ramp
    // from zero. Smoothing lives here (globally) because all voices share
    // the same parameter values.
    cutoffSmooth.setTarget(filterCutoff);
    macroSmooth.setTarget(macroAmount);
    delaySmooth.setTarget(delayMix);
    reverbSmooth.setTarget(reverbMix);
    distSmooth.setTarget(distMix);
    masterSmooth.setTarget(std::pow(10.0f, masterGainDb * 0.05f));

    const float cutoff = cutoffSmooth.process();
    const float macro = macroSmooth.process();
    const float delay = delaySmooth.process();
    const float reverb = reverbSmooth.process();
    const float dist = distSmooth.process();
    const float masterLin = masterSmooth.process();

    float mix = 0.0f;
    for (auto& v : pool)
        mix += v.render(cutoff, macro, delay, reverb, dist);

    // Master gain, then the limiter. The limiter guarantees the output
    // never exceeds its threshold, however many voices are sounding.
    return limiter.process(mix * masterLin);
}

int VoiceManager::getActiveVoiceCount() const
{
    int n = 0;
    for (const auto& v : pool)
        if (v.isActive())
            ++n;
    return n;
}

float VoiceManager::getTailLengthSeconds() const
{
    // The tail is the envelope's release time. All voices share the same
    // ADSR, so any voice's release time is the tail length.
    if (pool.empty())
        return 0.0f;
    return pool.front().getReleaseSeconds();
}
