#include "TherumVoice.h"
#include <cmath>

static float midiToHz(int note)
{
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

void TherumVoice::prepare(double sampleRate, const WavetableBank* sharedBank)
{
    oscA.prepare(sampleRate);
    oscA.setBank(sharedBank);
    oscB.prepare(sampleRate);
    oscB.setBank(sharedBank);

    ampEnv.prepare(sampleRate);
    ampEnv.setADSR(10.0f, 120.0f, 0.75f, 220.0f);
    lpf.prepare(sampleRate);
}

void TherumVoice::start(int note, float vel)
{
    midiNote = note;
    velocity = vel;
    oscA.setFrequency(midiToHz(note));
    oscB.setFrequency(midiToHz(note) * 1.005f);
    // Fresh start: reset the oscillator phases and the filter state so a
    // retrigger begins at the same point as a first trigger, with no click
    // from a stale phase or leftover filter memory.
    oscA.resetPhase();
    oscB.resetPhase();
    lpf.reset();
    ampEnv.noteOn();
    active = true;
}

void TherumVoice::stop()
{
    // Release only. Do NOT deactivate here: the voice keeps rendering its
    // release tail and deactivates itself in render() once the envelope has
    // run to zero. Deactivating here is what made the release never sound.
    ampEnv.noteOff();
}

float TherumVoice::render(float filterCutoff, float macroAmount, float delayMix, float reverbMix, float distMix)
{
    if (!active) return 0.0f;

    // The release tail is done: the envelope has run to zero, so this voice
    // is finished. Deactivate it now (and free the note) instead of
    // rendering silence forever.
    if (ampEnv.isFinished())
    {
        active = false;
        midiNote = -1;
        return 0.0f;
    }

    const float modulatedCutoff = filterCutoff * (0.5f + macroAmount * 0.5f);
    lpf.setCutoff(modulatedCutoff);

    const float raw = (oscA.process() * 0.62f) + (oscB.process() * 0.38f);
    const float filtered = lpf.process(raw);

    fx.setDelayMix(delayMix);
    fx.setReverbMix(reverbMix);
    fx.setDistMix(distMix);

    return fx.process(filtered) * ampEnv.process() * velocity * 0.2f;
}
