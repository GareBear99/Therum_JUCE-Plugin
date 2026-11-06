#pragma once
#include <vector>
#include "../Filters/FilterModels.h"
#include "../Oscillators/WavetableBank.h"
#include "../Utils/SmoothedParameter.h"
#include "../Utils/Limiter.h"
#include "TherumVoice.h"

// Voice pool + global parameter smoothing + output limiter.
//
// The manager owns the voice pool and the parameters that reach audio. It
// smooths every parameter it receives (one-pole, per sample) so host
// automation and knob moves never zipper, and it limits the summed output
// so the mix can never clip the host.
//
// Voice stealing is round-robin: when every voice is busy, the next note
// takes the voice at the cursor and the cursor advances, instead of always
// taking voice 0. A note-on for a note that is already sounding retriggers
// that voice instead of starting a duplicate.
class VoiceManager
{
public:
    void prepare(double sampleRate, int voices);

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();

    // The voice filter type (SVF LowPass by default; Ladder selects
    // the Moog ladder). Applied to every voice in the pool, and to
    // voices prepared afterwards. A change crossfades per voice, so
    // it never clicks. Safe to call per block (cheap change-detect).
    void setFilterType(TherumFilterType type);

    // Renders one sample of the summed mix. masterGainDb is the host's
    // master gain in dB; the manager converts it to linear, smooths it,
    // applies it, and limits the result. The returned sample never exceeds
    // the limiter threshold in absolute value.
    // filterReso is the ladder resonance (0..1); it is smoothed per
    // sample like the cutoff, and is inert in SVF mode (the SVF keeps
    // Q = 0.7071).
    float renderMixed(float filterCutoff, float filterReso, float macroAmount, float delayMix, float reverbMix, float distMix, float masterGainDb);
    int getActiveVoiceCount() const;
    bool isNoteActive(int midiNote) const;

    // Longest release tail in seconds, for the host's tail-length reporting.
    // (The release is the envelope's release time; the host needs it so it
    // does not cut the tail off when the transport stops.)
    float getTailLengthSeconds() const;

private:
    TherumVoice* findFreeVoice();
    TherumVoice* findVoiceByNote(int midiNote);

    WavetableBank bank;
    std::vector<TherumVoice> pool;

    // Round-robin cursor: the next voice to steal when the pool is full,
    // and the starting point when scanning for a free voice.
    size_t nextVoice = 0;

    // One-pole smoothers for every parameter that reaches audio. They live
    // here (once, globally) rather than per voice, because all voices share
    // the same parameter values.
    SmoothedParameter cutoffSmooth;
    SmoothedParameter resoSmooth; // ladder resonance (inert in SVF mode)
    SmoothedParameter macroSmooth;
    SmoothedParameter delaySmooth;
    SmoothedParameter reverbSmooth;
    SmoothedParameter distSmooth;
    SmoothedParameter masterSmooth; // linear gain

    // The voice filter type, shared by all voices (SVF LowPass by
    // default). Stored so prepare() applies it to voices it creates.
    TherumFilterType currentFilterType = TherumFilterType::LowPass;

    Limiter limiter;
};
