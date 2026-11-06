// MorphChoice — the single source of truth for the osc morph choices:
// the labels the UI shows, the index the parameter stores, and the
// MorphMode / MorphPhase the oscillator renders.
//
// The AudioParameterChoice "Osc A/B Morph Mode" and "Osc A/B Morph
// Phase" in the processor layout, the ComboBoxes in OscPanel, and
// this header all read the same lists, so a label can never drift
// between the parameter and the panel. The UI selector writes a
// choice index; the processor maps it once per block with
// modeFromIndex() / phaseFromIndex() and the manager change-detects,
// so a repeated set is free — the selector drives the same path the
// voice wiring built, no second path.
//
// Persistence: the parameter store keeps each choice by its index,
// so a saved index restores the same mode/phase on load —
// modeFromIndex / phaseFromIndex are pure functions of the index,
// and indexFromMode / indexFromPhase are their exact inverses.
// The host-side save/restore round-trip itself is verified on the
// Mac build check, not here: the headless rig cannot instantiate
// the processor.
//
// Morphing (paper section 4.1). Crossfade is the cheap default
// (per-sample time-domain blend of the adjacent frames). Spectral
// is the quality mode (the position is rendered by interpolating
// the frames' spectra and IFFT'ing, block-rate). The phase setting
// is what kills the phasing: None blends the complex bins
// directly; Zero Fundamental rotates frame B so its fundamental
// phase matches A's, then blends; Zero All blends magnitudes
// only, so the morphed frame is zero-phase (all harmonics are
// cosines). The phase choice is inert in Crossfade mode — it
// only affects the spectral render.
//
// Framework-free (paper section 3): plain values only, no host
// includes — the headless test rig covers this header.
#pragma once

#include <array>

#include "WavetableOscillator.h"

namespace TherumMorphChoice
{
    // The two morph modes, in choice order.
    inline constexpr std::array<const char*, 2> modeLabels =
        { "Crossfade", "Spectral" };
    inline constexpr int modeCount = 2;
    inline constexpr int modeDefaultIndex = 0; // Crossfade: the cheap default

    // Maps a choice index to the oscillator morph mode. Any
    // out-of-range index falls back to the cheap Crossfade.
    inline WavetableOscillator::MorphMode modeFromIndex(int index)
    {
        switch (index)
        {
            case 1: return WavetableOscillator::MorphMode::Spectral;
            case 0:
            default: return WavetableOscillator::MorphMode::Crossfade;
        }
    }

    // Inverse of modeFromIndex.
    inline int indexFromMode(WavetableOscillator::MorphMode m)
    {
        switch (m)
        {
            case WavetableOscillator::MorphMode::Spectral: return 1;
            case WavetableOscillator::MorphMode::Crossfade:
            default: return 0;
        }
    }

    // The three spectral phase modes, in choice order.
    inline constexpr std::array<const char*, 3> phaseLabels =
        { "None", "Zero Fundamental", "Zero All" };
    inline constexpr int phaseCount = 3;
    inline constexpr int phaseDefaultIndex = 0; // None: blend the bins directly

    // Maps a choice index to the oscillator morph phase. Any
    // out-of-range index falls back to None.
    inline WavetableOscillator::MorphPhase phaseFromIndex(int index)
    {
        switch (index)
        {
            case 1: return WavetableOscillator::MorphPhase::ZeroFundamental;
            case 2: return WavetableOscillator::MorphPhase::ZeroAll;
            case 0:
            default: return WavetableOscillator::MorphPhase::None;
        }
    }

    // Inverse of phaseFromIndex.
    inline int indexFromPhase(WavetableOscillator::MorphPhase p)
    {
        switch (p)
        {
            case WavetableOscillator::MorphPhase::ZeroFundamental: return 1;
            case WavetableOscillator::MorphPhase::ZeroAll: return 2;
            case WavetableOscillator::MorphPhase::None:
            default: return 0;
        }
    }
}
