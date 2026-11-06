// FilterTypeChoice — the single source of truth for the filter1_type
// choice: the labels the UI shows, the index the parameter stores,
// and the TherumFilterType the voice renders.
//
// The AudioParameterChoice "Filter 1 Type" in the processor layout,
// the ComboBox in FilterPanel, and this header all read the same
// list, so a label can never drift between the parameter and the
// panel. The UI selector writes a choice index; the processor maps
// it once per block with typeFromIndex() and the manager crossfades
// (VoiceFilter::kCrossfadeSamples = 128) — the selector drives the
// same path the voice wiring built, no second path.
//
// Persistence: the parameter store keeps the choice by its index,
// so a saved index restores the same type on load — typeFromIndex
// is a pure function of the index, and indexFromType is its exact
// inverse for the five selectable types. The host-side save/restore
// round-trip itself is verified on the Mac build check, not here:
// the headless rig cannot instantiate the processor.
//
// Framework-free (paper section 3): plain values only, no host
// includes — the headless test rig covers this header.
#pragma once

#include <array>

#include "FilterModels.h"

namespace TherumFilterChoice
{
    // The five selectable types, in choice order. Comb / Formant
    // are in the enum but not implemented (VoiceFilter keeps the
    // current type for them), so they have no label and no index.
    inline constexpr std::array<const char*, 5> labels =
        { "Low Pass", "High Pass", "Band Pass", "Notch", "Ladder" };
    inline constexpr int count = 5;
    inline constexpr int defaultIndex = 0; // Low Pass: the shipped voice filter

    // Maps a choice index to the voice filter type. Any out-of-range
    // index falls back to the shipped Low Pass.
    inline TherumFilterType typeFromIndex(int index)
    {
        switch (index)
        {
            case 1: return TherumFilterType::HighPass;
            case 2: return TherumFilterType::BandPass;
            case 3: return TherumFilterType::Notch;
            case 4: return TherumFilterType::Ladder;
            case 0:
            default: return TherumFilterType::LowPass;
        }
    }

    // Inverse of typeFromIndex for the five selectable types.
    // Comb / Formant have no choice index: -1.
    inline int indexFromType(TherumFilterType t)
    {
        switch (t)
        {
            case TherumFilterType::LowPass:  return 0;
            case TherumFilterType::HighPass: return 1;
            case TherumFilterType::BandPass: return 2;
            case TherumFilterType::Notch:    return 3;
            case TherumFilterType::Ladder:   return 4;
            default: return -1;
        }
    }
}
