// VoiceFilter — the voice's filter slot: one Simper SVF (LP/HP/BP/Notch)
// or the Huovilainen Moog ladder, selected by TherumFilterType.
//
// The SVF path is the shipped voice filter, unchanged: the same
// object, the same calls, the same Q (0.7071, Butterworth) and drive
// (0) — selecting an SVF type is bit-identical to the pre-ladder
// voice (proved by the test rig, section (f)). The ladder is the new
// engine, selected by type == Ladder, and runs at 1x in production
// (Gary's call, 2026-11-05): the 2x build stays built and measured,
// it is measurement mode only for the voice path.
//
// 1x production bound: the ladder's cutoff is clamped to
// 20 Hz .. sr/(2*pi) (alpha <= 1), so the ladder stays a lowpass
// with the stopband below 0 dB. Above alpha = 1 the stopband rises
// above 0 dB (measured, test_moogladder section (c)); above
// alpha = 2 the Euler update is unstable (measured, section (j)).
// At 44.1 kHz the production ladder tops out at ~7.0 kHz — the
// honest consequence of 1x. The 2x build keeps the full range;
// the full-range question is the oversampling decision, already
// made, not something this file re-decides.
//
// Type switching crossfades the outputs over 128 samples (~3 ms):
// the outgoing engine keeps processing the input (its state decays
// naturally) while the incoming engine starts from reset and fades
// in, so there is no step and no click. Two SVF objects ping-pong
// so an SVF-to-SVF type change crossfades the same way (the
// incoming SVF is freshly reset and fades in). A second switch
// inside a crossfade restarts the fade from the new engine —
// bounded, but a rapid double-switch can step by the engine
// difference; in practice a type change cannot happen twice
// within 3 ms from a knob, and automation doing so is pathological.
//
// The first type set after prepare()/reset() snaps (the state is
// zero, there is nothing to crossfade from). reset() clears the
// state and any crossfade in progress, but not the type.
//
// Resonance: the ladder's resonance (0..1, k = 4*reso) comes from
// setResonance(). The SVF keeps its Q = 0.7071 — setResonance()
// is a no-op for SVF types, so the shipped SVF sound is unchanged.
// The processor's filter1_reso parameter therefore drives the
// ladder only; in SVF mode it is inert, by design. (Comb and
// Formant are in the enum but not implemented: setType() keeps
// the current type for them.)
//
// Lock-free rules (paper section 3): every member is a plain
// value or a filter object constructed once in prepare() — no
// allocation on the audio thread, no locks anywhere. setType /
// setCutoff / setResonance are cheap change-detects, safe to call
// per sample from the voice render.
#pragma once

#include <algorithm>
#include <cmath>

#include "FilterModels.h"
#include "MoogLadderFilter.h"
#include "SvfFilter.h"

class VoiceFilter
{
public:
    static constexpr int kCrossfadeSamples = 128;

    void prepare(double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        svf[0].prepare(sr);
        svf[1].prepare(sr);
        for (int i = 0; i < 2; ++i)
        {
            svf[i].setType(SvfFilter::Type::LowPass);
            svf[i].setQ(0.7071f); // Butterworth: -3 dB at the cutoff, no peak
            svf[i].setDrive(0.0f); // no drive in the voice path
        }
        ladder.prepare(sr);
        ladder.setOversampling(1); // production: 1x (Gary's call, 2026-11-05)
        type = TherumFilterType::LowPass;
        active = 0; // svf[0], lowpass — the shipped voice filter
        outgoing = -1;
        xfadePos = 0;
        cutoff = -1.0f;
        reso = -1.0f;
        hasProcessed = false;
    }

    void setType(TherumFilterType t)
    {
        if (t == type)
            return;
        if (!isSvfType(t) && t != TherumFilterType::Ladder)
            return; // Comb / Formant: not implemented, keep the current type

        const int target = (t == TherumFilterType::Ladder) ? 2 : targetSvf();
        type = t;
        configureEngine(target);

        if (!hasProcessed)
        {
            // Nothing processed yet: snap. The state is zero, so
            // there is no discontinuity to crossfade away.
            active = target;
            outgoing = -1;
            xfadePos = 0;
        }
        else
        {
            outgoing = active;
            active = target;
            xfadePos = 0;
        }
    }

    void setCutoff(float hz)
    {
        if (!std::isfinite(hz))
            return;
        if (hz == cutoff)
            return;
        cutoff = hz;
        applyCutoff(active);
        if (outgoing >= 0)
            applyCutoff(outgoing);
    }

    void setResonance(float r)
    {
        if (!std::isfinite(r))
            return;
        if (r == reso)
            return;
        reso = r;
        applyResonance(active);
        if (outgoing >= 0)
            applyResonance(outgoing);
    }

    void reset()
    {
        svf[0].reset();
        svf[1].reset();
        ladder.reset();
        outgoing = -1;
        xfadePos = 0;
        hasProcessed = false;
    }

    float process(float x)
    {
        hasProcessed = true;
        if (outgoing < 0)
            return processEngine(active, x);

        const float yOld = processEngine(outgoing, x);
        const float yNew = processEngine(active, x);
        const float alpha = (float)(xfadePos + 1) / (float)kCrossfadeSamples;
        ++xfadePos;
        if (xfadePos >= kCrossfadeSamples)
            outgoing = -1;
        return yOld + alpha * (yNew - yOld);
    }

    // Accessors, for tests.
    TherumFilterType getType() const { return type; }
    // The active engine's cutoff (the ladder's is the 1x-clamped value).
    float getCutoff() const
    {
        return (active == 2) ? ladder.getCutoff() : svf[active].getCutoff();
    }
    // The last setResonance() value (the parameter, not the SVF's Q —
    // the SVF has no resonance parameter; its Q is fixed at 0.7071).
    float getResonance() const { return reso; }
    bool isCrossfading() const { return outgoing >= 0; }

private:
    static constexpr double kPi = 3.14159265358979323846;

    static bool isSvfType(TherumFilterType t)
    {
        return t == TherumFilterType::LowPass || t == TherumFilterType::HighPass ||
               t == TherumFilterType::BandPass || t == TherumFilterType::Notch;
    }

    static SvfFilter::Type toSvfType(TherumFilterType t)
    {
        switch (t)
        {
            case TherumFilterType::HighPass: return SvfFilter::Type::HighPass;
            case TherumFilterType::BandPass: return SvfFilter::Type::BandPass;
            case TherumFilterType::Notch: return SvfFilter::Type::Notch;
            case TherumFilterType::LowPass:
            default: return SvfFilter::Type::LowPass;
        }
    }

    // The SVF that is not currently active (the ping-pong target).
    // If the ladder is active, svf[0] is the target.
    int targetSvf() const { return (active == 0) ? 1 : 0; }

    // The 1x production bound: alpha = 2*pi*fc/sr <= 1, so the
    // ladder stays a lowpass. Returns < 0 when hz is not finite
    // (the caller skips the call, keeping the previous cutoff).
    float ladderCutoff(float hz) const
    {
        if (!std::isfinite(hz))
            return -1.0f;
        const float hi = (float)(sr / (2.0 * kPi));
        return std::clamp(hz, 20.0f, hi);
    }

    void applyCutoff(int engine)
    {
        if (cutoff < 0.0f)
            return; // never set: the engine keeps its own "never set" state
        if (engine == 2)
        {
            const float c = ladderCutoff(cutoff);
            if (c >= 0.0f)
                ladder.setCutoff(c);
        }
        else
        {
            svf[engine].setCutoff(cutoff);
        }
    }

    void applyResonance(int engine)
    {
        if (reso < 0.0f)
            return; // never set
        if (engine == 2)
            ladder.setResonance(reso); // clamps 0..1 internally
        // SVF: no resonance parameter; Q stays 0.7071.
    }

    // Configure the target engine for the new type (type, cutoff,
    // resonance) and reset its state, so the crossfade fades in
    // from a clean start.
    void configureEngine(int engine)
    {
        if (engine == 2)
        {
            applyCutoff(2);
            applyResonance(2);
            ladder.reset();
        }
        else
        {
            svf[engine].setType(toSvfType(type));
            applyCutoff(engine);
            svf[engine].reset();
        }
    }

    float processEngine(int engine, float x)
    {
        if (engine == 2)
            return ladder.process(x);
        return svf[engine].process(x);
    }

    double sr = 44100.0;
    SvfFilter svf[2];          // ping-pong pair for SVF types
    MoogLadderFilter ladder;   // the Moog ladder, 1x in production
    TherumFilterType type = TherumFilterType::LowPass;
    int active = 0;            // 0 = svf[0], 1 = svf[1], 2 = ladder
    int outgoing = -1;         // engine fading out, or -1 (no crossfade)
    int xfadePos = 0;          // samples into the crossfade
    float cutoff = -1.0f;      // last requested cutoff (-1 = never set)
    float reso = -1.0f;        // last requested resonance (-1 = never set)
    bool hasProcessed = false;
};
