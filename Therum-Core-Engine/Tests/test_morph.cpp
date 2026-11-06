// test_morph — wavetable morph wired into the Therum voice.
//
// Paper section 4.1, rank 2 (wavetable engine): the morph mode and
// phase are selectable per oscillator (Crossfade / Spectral, None /
// Zero Fundamental / Zero All), driven by parameters, smoothed
// frame positions, and a 128-sample output crossfade on a mode/phase
// switch — the same standard as the voice filter's type crossfade.
// Before this work the morph DSP existed (Step 2 piece 4) but nothing
// called it: no parameters, no voice wiring, and the frame position
// itself never reached the oscillators (both sat at position 0, the
// sine frame), so morph could do nothing audible.
//
// Expected behaviour (defined BEFORE the tests were run):
//   (E1) Position wiring: a voice at wtPosA 0.5 renders differently
//        from wtPosA 0.0 (different frames); the same settings twice
//        render bit-identical (deterministic); wtPosA 2.0 clamps to
//        1.0 and -1.0 clamps to 0.0 (identical renders).
//   (E2) Spectral/None == Crossfade: at pos 0.5, Spectral with phase
//        None renders the same frame as Crossfade — interpolating
//        complex spectra then IFFT'ing is the same linear blend as
//        the time-domain crossfade, up to FFT round-off. Voice-level
//        max|diff| < 1e-4. This proves the spectral engine is
//        correctly wired, not just reachable.
//   (E3) ZeroAll differs: Spectral + ZeroAll at pos 0.5 differs from
//        Crossfade by > 1% RMS — the phase setting reaches the
//        oscillator and changes the render (zero-phase magnitudes
//        are a different waveshape from the blended bins).
//   (E4) ZeroFundamental rotates: on a custom bank (frame A sine,
//        frame B cosine, 90 degrees apart), at pos 0.5 Spectral +
//        ZeroFundamental renders the pure sine (frame A, max|diff|
//        < 1e-4 vs the analytic sine), Spectral + None renders
//        0.5*sin + 0.5*cos, and Spectral + ZeroAll renders the pure
//        cosine. This proves the rotation does what the header
//        claims: it aligns B's fundamental to A's before blending.
//   (E5) Mid-note switch is click-free: render Crossfade 0.5 s,
//        switch to Spectral/ZeroAll, render 0.5 s more. Finite
//        everywhere; the largest per-sample step inside the 128
//        samples after the switch is at most 2x the largest step
//        in steady state (the E4 criterion from test_voicefilter);
//        after settling, the output matches a fresh Spectral/ZeroAll
//        voice (last-0.5 s RMS within 10% — the section-(i)
//        criterion, which averages out the detuned osc B beat).
//   (E6) Single-frame bank: Spectral on a 1-frame bank renders the
//        frame (matches Crossfade, max|diff| < 1e-4), finite,
//        exactly 1 morphed-frame rebuild.
//   (E7) No-spectra bank: a bank that never built mipmaps has no
//        spectra, so Spectral falls through to the per-sample
//        crossfade — the output is bit-identical to Crossfade
//        (max|diff| == 0). Morph is never silently wrong.
//   (E8) MorphChoice mapping: the labels are the choice strings in
//        order, every index maps to the right mode/phase,
//        out-of-range indices fall back to Crossfade / None, and
//        the map inverts exactly — a saved index restores the same
//        mode/phase.
//   (E9) UI-driven path (MorphChoice): setMorphA(modeFromIndex(1),
//        phaseFromIndex(2)) — the exact path the processor runs
//        when the user picks Spectral + Zero All in the OscPanel —
//        engages Spectral/ZeroAll in the voice (differs from
//        Crossfade by > 1% RMS; bit-identical to the same settings
//        passed directly). The host-side save/restore round-trip of
//        the parameter value itself is verified on the Mac build
//        check, not here: the headless rig cannot instantiate the
//        processor.
//   (E10) Rebuild rate: Spectral, fixed position, setPosition(same
//        value) every sample for 1 s -> exactly 1 rebuild (the
//        no-op guard: an unchanged position marks nothing dirty, so
//        the voice calling setPosition per sample from its smoothed
//        parameter costs nothing once settled). A moving position
//        for 0.1 s rebuilds, but stays bounded (< 150 rebuilds:
//        0.1 s / 32 samples + the initial one).
//   (E11) Position smoothing: step wtPosA 0.0 -> 1.0 mid-note.
//        Finite and bounded; 0.5 s after the step the output
//        matches a fresh voice at wtPosA 1.0 (last-0.5 s RMS within
//        10% — the smoother has settled: 10 ms tau, 0.5 s is 50
//        tau, and the comparison window averages the detuned beat).
// Build and run: `make morph` (see Makefile), or:
//   g++ -std=c++17 -O2 -Wall -Wextra -o test_morph \
//       test_morph.cpp \
//       ../Source/DSP/Oscillators/WavetableOscillator.cpp \
//       ../Source/DSP/Oscillators/WavetableBank.cpp \
//       ../Source/DSP/Oscillators/WavetableBankMips.cpp \
//       ../Source/DSP/Utils/Fft.cpp \
//       ../Source/DSP/Utils/WavImport.cpp \
//       ../Source/DSP/Filters/SvfFilter.cpp \
//       ../Source/DSP/Filters/MoogLadderFilter.cpp \
//       ../Source/DSP/Filters/DualFilter.cpp \
//       ../Source/DSP/Modulation/EnvelopeGenerator.cpp \
//       ../Source/DSP/Voicing/TherumVoice.cpp \
//       ../Source/DSP/Voicing/VoiceManager.cpp \
//       ../Source/DSP/FX/FxRack.cpp
//   ./test_morph
// Exit code 0 means every check passed; non-zero means a check
// failed (failures are printed with file and line).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "../Source/DSP/Oscillators/MorphChoice.h"
#include "../Source/DSP/Oscillators/WavetableBank.h"
#include "../Source/DSP/Oscillators/WavetableOscillator.h"
#include "../Source/DSP/Voicing/VoiceManager.h"

static constexpr double kSr = 44100.0;
static constexpr double kPi = 3.14159265358979323846;
static constexpr double kPi2 = 2.0 * kPi;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond) do { \
    ++g_checks; \
    if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_NEAR(a, b, eps) do { \
    ++g_checks; \
    const double _d = std::fabs((double)(a) - (double)(b)); \
    if (!(_d <= (double)(eps))) { ++g_failures; \
        std::printf("FAIL %s:%d: |(%s) - (%s)| = %g > %g\n", __FILE__, __LINE__, #a, #b, _d, (double)(eps)); } \
} while (0)

// Max |a - b| over two equal-length buffers.
static double maxDiff(const std::vector<float>& a, const std::vector<float>& b)
{
    double worst = 0.0;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
        worst = std::max(worst, (double)std::fabs((double)a[i] - (double)b[i]));
    return worst;
}

// RMS of a buffer over a window [first, last).
static double rmsWindow(const std::vector<float>& a, size_t first, size_t last)
{
    double sum = 0.0;
    size_t n = 0;
    for (size_t i = first; i < last && i < a.size(); ++i)
    {
        sum += (double)a[i] * (double)a[i];
        ++n;
    }
    return n > 0 ? std::sqrt(sum / (double)n) : 0.0;
}

// RMS difference over a window [first, last).
static double rmsDiff(const std::vector<float>& a, const std::vector<float>& b,
                      size_t first, size_t last)
{
    double sum = 0.0;
    size_t n = 0;
    for (size_t i = first; i < last && i < a.size() && i < b.size(); ++i)
    {
        const double d = (double)a[i] - (double)b[i];
        sum += d * d;
        ++n;
    }
    return n > 0 ? std::sqrt(sum / (double)n) : 0.0;
}

using MorphMode = WavetableOscillator::MorphMode;
using MorphPhase = WavetableOscillator::MorphPhase;

// Render a note through a fresh VoiceManager with the given morph
// settings and frame positions. Cutoff 18000 Hz (transparent),
// reso 0.15, macro 1.0 (no cutoff modulation), no FX, master 0 dB,
// note 69, velocity 1.0. Both oscillators share the mode/phase
// unless the B settings are given.
static std::vector<float> renderVoice(float wtPosA, float wtPosB,
                                      MorphMode modeA, MorphPhase phaseA,
                                      MorphMode modeB, MorphPhase phaseB,
                                      double seconds)
{
    VoiceManager vm;
    vm.prepare(kSr, 16);
    vm.setMorphA(modeA, phaseA);
    vm.setMorphB(modeB, phaseB);
    vm.noteOn(69, 1.0f);
    const int n = (int)(seconds * kSr);
    std::vector<float> out((size_t)n);
    for (int i = 0; i < n; ++i)
        out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, wtPosA, wtPosB);
    return out;
}

// Render N samples from a standalone oscillator.
static std::vector<float> renderOsc(WavetableOscillator& o, int n)
{
    std::vector<float> out((size_t)n);
    for (int i = 0; i < n; ++i)
        out[(size_t)i] = o.process();
    return out;
}

// The default bank (sine, triangle, saw, square) with mipmaps and
// spectra, the same bank the VoiceManager builds.
static WavetableBank defaultBank()
{
    WavetableBank bank;
    bank.buildDefaultBank();
    bank.buildMipmaps();
    return bank;
}

int main()
{
    std::printf("Therum morph test (paper section 4.1, morph wired into the voice)\n");
    std::printf("=================================================================\n");

    // --- (a) Position wiring (voice level). ---
    std::printf("\n-- (a) Position wiring (voice level) --\n");
    {
        const std::vector<float> at0 = renderVoice(0.0f, 0.0f, MorphMode::Crossfade, MorphPhase::None,
                                                    MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> atHalf = renderVoice(0.5f, 0.0f, MorphMode::Crossfade, MorphPhase::None,
                                                      MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> at0Again = renderVoice(0.0f, 0.0f, MorphMode::Crossfade, MorphPhase::None,
                                                        MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> at2 = renderVoice(2.0f, 0.0f, MorphMode::Crossfade, MorphPhase::None,
                                                    MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> at1 = renderVoice(1.0f, 0.0f, MorphMode::Crossfade, MorphPhase::None,
                                                    MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> atNeg = renderVoice(-1.0f, 0.0f, MorphMode::Crossfade, MorphPhase::None,
                                                     MorphMode::Crossfade, MorphPhase::None, 1.0);
        const double dHalf = maxDiff(at0, atHalf);
        const double dSame = maxDiff(at0, at0Again);
        const double dClampHi = maxDiff(at2, at1);
        const double dClampLo = maxDiff(atNeg, at0);
        std::printf("  pos 0.0 vs 0.5: max|diff| %.3g (RMS 0.0 %.4f, RMS 0.5 %.4f)\n",
                    dHalf, rmsWindow(at0, 0, at0.size()), rmsWindow(atHalf, 0, atHalf.size()));
        std::printf("  same settings twice: max|diff| %.3g\n", dSame);
        std::printf("  clamp: 2.0 vs 1.0 max|diff| %.3g; -1.0 vs 0.0 max|diff| %.3g\n", dClampHi, dClampLo);
        CHECK(dHalf > 0.01); // the position reaches the oscillator
        CHECK(dSame == 0.0); // deterministic
        CHECK(dClampHi == 0.0); // 2.0 clamps to 1.0
        CHECK(dClampLo == 0.0); // -1.0 clamps to 0.0
    }

    // --- (b) Spectral/None == Crossfade (voice level). ---
    std::printf("\n-- (b) Spectral/None == Crossfade (voice level) --\n");
    {
        const std::vector<float> cross = renderVoice(0.5f, 0.5f, MorphMode::Crossfade, MorphPhase::None,
                                                     MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> spectral = renderVoice(0.5f, 0.5f, MorphMode::Spectral, MorphPhase::None,
                                                        MorphMode::Spectral, MorphPhase::None, 1.0);
        const double d = maxDiff(cross, spectral);
        std::printf("  pos 0.5, Spectral/None vs Crossfade: max|diff| %.3g\n", d);
        CHECK(d < 1e-4); // same linear blend, up to FFT round-off
    }

    // --- (c) ZeroAll differs (voice level). ---
    std::printf("\n-- (c) ZeroAll differs (voice level) --\n");
    {
        const std::vector<float> cross = renderVoice(0.5f, 0.5f, MorphMode::Crossfade, MorphPhase::None,
                                                     MorphMode::Crossfade, MorphPhase::None, 1.0);
        const std::vector<float> zeroAll = renderVoice(0.5f, 0.5f, MorphMode::Spectral, MorphPhase::ZeroAll,
                                                       MorphMode::Spectral, MorphPhase::ZeroAll, 1.0);
        const double rmsCross = rmsWindow(cross, 0, cross.size());
        const double dRms = rmsDiff(cross, zeroAll, 0, cross.size());
        std::printf("  pos 0.5, Spectral/ZeroAll vs Crossfade: RMS diff %.4f vs signal RMS %.4f (%.1f%%)\n",
                    dRms, rmsCross, 100.0 * dRms / rmsCross);
        CHECK(dRms > 0.01 * rmsCross); // the phase setting changes the render
    }

    // --- (d) ZeroFundamental rotates (oscillator level, custom bank). ---
    std::printf("\n-- (d) ZeroFundamental rotates (oscillator level) --\n");
    {
        // Frame A sine, frame B cosine (90 degrees apart).
        WavetableBank bank;
        std::vector<float> sine((size_t)WavetableBank::kFrameSize);
        std::vector<float> cosine((size_t)WavetableBank::kFrameSize);
        for (int i = 0; i < WavetableBank::kFrameSize; ++i)
        {
            const double ph = kPi2 * (double)i / (double)WavetableBank::kFrameSize;
            sine[(size_t)i] = (float)std::sin(ph);
            cosine[(size_t)i] = (float)std::cos(ph);
        }
        CHECK(bank.addFrame(sine.data(), WavetableBank::kFrameSize));
        CHECK(bank.addFrame(cosine.data(), WavetableBank::kFrameSize));
        CHECK(bank.buildMipmaps());
        CHECK(bank.hasSpectra());

        const int n = (int)kSr; // 1 s at 440 Hz
        std::vector<float> expectSine((size_t)n), expectCosine((size_t)n), expectMix((size_t)n);
        for (int i = 0; i < n; ++i)
        {
            const double ph = kPi2 * 440.0 * (double)i / kSr;
            expectSine[(size_t)i] = (float)std::sin(ph);
            expectCosine[(size_t)i] = (float)std::cos(ph);
            expectMix[(size_t)i] = (float)(0.5 * std::sin(ph) + 0.5 * std::cos(ph));
        }

        auto renderMode = [&](MorphMode m, MorphPhase p)
        {
            WavetableOscillator o;
            o.prepare(kSr);
            o.setBank(&bank);
            o.setFrequency(440.0f);
            o.setMorphMode(m);
            o.setMorphPhase(p);
            o.setPosition(0.5f);
            return renderOsc(o, n);
        };
        const std::vector<float> outNone = renderMode(MorphMode::Spectral, MorphPhase::None);
        const std::vector<float> outZf = renderMode(MorphMode::Spectral, MorphPhase::ZeroFundamental);
        const std::vector<float> outZa = renderMode(MorphMode::Spectral, MorphPhase::ZeroAll);
        const double dNone = maxDiff(outNone, expectMix);
        const double dZf = maxDiff(outZf, expectSine);
        const double dZa = maxDiff(outZa, expectCosine);
        std::printf("  None vs 0.5*sin+0.5*cos: max|diff| %.3g\n", dNone);
        std::printf("  ZeroFundamental vs pure sine: max|diff| %.3g\n", dZf);
        std::printf("  ZeroAll vs pure cosine: max|diff| %.3g\n", dZa);
        CHECK(dNone < 1e-4); // blends the bins directly
        CHECK(dZf < 1e-4);   // rotates B's fundamental onto A's
        CHECK(dZa < 1e-4);   // magnitudes only, zero phase
    }

    // --- (e) Mid-note switch is click-free (voice level). ---
    std::printf("\n-- (e) Mid-note switch is click-free (voice level) --\n");
    {
        VoiceManager vm;
        vm.prepare(kSr, 16);
        vm.noteOn(69, 1.0f);
        const int half = (int)(kSr * 0.5);
        std::vector<float> out((size_t)(kSr * 1.0));
        double maxStepSteady = 0.0;
        for (int i = 0; i < half; ++i)
        {
            out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            if (i >= (int)(kSr * 0.25) && i > 0)
                maxStepSteady = std::max(maxStepSteady, (double)std::fabs((double)out[(size_t)i] - (double)out[(size_t)(i - 1)]));
        }
        // Switch Crossfade -> Spectral/ZeroAll mid-note.
        vm.setMorphA(MorphMode::Spectral, MorphPhase::ZeroAll);
        vm.setMorphB(MorphMode::Spectral, MorphPhase::ZeroAll);
        double maxStepSwitch = 0.0;
        bool finite = true;
        for (int i = half; i < (int)(kSr * 1.0); ++i)
        {
            out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            if (!std::isfinite(out[(size_t)i])) finite = false;
            if (i < half + 128)
                maxStepSwitch = std::max(maxStepSwitch, (double)std::fabs((double)out[(size_t)i] - (double)out[(size_t)(i - 1)]));
        }
        // Fresh Spectral/ZeroAll voice for the settled comparison.
        const std::vector<float> fresh = renderVoice(0.5f, 0.5f, MorphMode::Spectral, MorphPhase::ZeroAll,
                                                     MorphMode::Spectral, MorphPhase::ZeroAll, 1.0);
        const double rmsFresh = rmsWindow(fresh, (size_t)(kSr * 0.5), fresh.size());
        const double rmsSwitched = rmsWindow(out, (size_t)(kSr * 0.5), out.size());
        std::printf("  steady max step %.4f; switch-window max step %.4f (ratio %.2f)\n",
                    maxStepSteady, maxStepSwitch, maxStepSwitch / maxStepSteady);
        std::printf("  settled RMS: switched %.4f vs fresh %.4f (%.1f%% apart)\n",
                    rmsSwitched, rmsFresh, 100.0 * std::fabs(rmsSwitched - rmsFresh) / rmsFresh);
        CHECK(finite);
        CHECK(maxStepSwitch <= 2.0 * maxStepSteady); // the E4 criterion
        CHECK(std::fabs(rmsSwitched - rmsFresh) <= 0.10 * rmsFresh); // the section-(i) criterion
    }

    // --- (f) Single-frame bank (oscillator level). ---
    std::printf("\n-- (f) Single-frame bank (oscillator level) --\n");
    {
        WavetableBank bank;
        bank.buildDefaultSineTable();
        CHECK(bank.buildMipmaps());
        auto renderMode = [&](MorphMode m)
        {
            WavetableOscillator o;
            o.prepare(kSr);
            o.setBank(&bank);
            o.setFrequency(440.0f);
            o.setMorphMode(m);
            o.setMorphPhase(MorphPhase::None);
            o.setPosition(0.5f);
            std::vector<float> out = renderOsc(o, (int)kSr);
            return std::make_pair(out, o.getMorphRecomputeCount());
        };
        const auto cross = renderMode(MorphMode::Crossfade);
        const auto spectral = renderMode(MorphMode::Spectral);
        const double d = maxDiff(cross.first, spectral.first);
        bool finite = true;
        for (float s : spectral.first) if (!std::isfinite(s)) finite = false;
        std::printf("  1 frame, Spectral vs Crossfade: max|diff| %.3g, recomputes %d, finite = %s\n",
                    d, spectral.second, finite ? "yes" : "NO");
        CHECK(finite);
        CHECK(d < 1e-4); // nothing to morph between: the frame is the frame
        CHECK(spectral.second == 1); // built once, never again
    }

    // --- (g) No-spectra bank falls through to crossfade (oscillator level). ---
    std::printf("\n-- (g) No-spectra bank falls through (oscillator level) --\n");
    {
        WavetableBank bank;
        bank.buildDefaultBank(); // no buildMipmaps(): no spectra, no mips
        CHECK(!bank.hasSpectra());
        auto renderMode = [&](MorphMode m)
        {
            WavetableOscillator o;
            o.prepare(kSr);
            o.setBank(&bank);
            o.setFrequency(440.0f);
            o.setMorphMode(m);
            o.setMorphPhase(MorphPhase::ZeroAll);
            o.setPosition(0.5f);
            return renderOsc(o, (int)kSr);
        };
        const std::vector<float> cross = renderMode(MorphMode::Crossfade);
        const std::vector<float> spectral = renderMode(MorphMode::Spectral);
        const double d = maxDiff(cross, spectral);
        std::printf("  no spectra, Spectral vs Crossfade: max|diff| %.3g\n", d);
        CHECK(d == 0.0); // same code path, bit-identical
    }

    // --- (h) MorphChoice mapping. ---
    std::printf("\n-- (h) MorphChoice mapping --\n");
    {
        CHECK(std::string(TherumMorphChoice::modeLabels[0]) == "Crossfade");
        CHECK(std::string(TherumMorphChoice::modeLabels[1]) == "Spectral");
        CHECK(TherumMorphChoice::modeCount == 2);
        CHECK(TherumMorphChoice::modeDefaultIndex == 0);
        CHECK(TherumMorphChoice::modeFromIndex(0) == MorphMode::Crossfade);
        CHECK(TherumMorphChoice::modeFromIndex(1) == MorphMode::Spectral);
        CHECK(TherumMorphChoice::modeFromIndex(-1) == MorphMode::Crossfade);
        CHECK(TherumMorphChoice::modeFromIndex(2) == MorphMode::Crossfade);
        CHECK(TherumMorphChoice::modeFromIndex(99) == MorphMode::Crossfade);
        CHECK(TherumMorphChoice::indexFromMode(MorphMode::Crossfade) == 0);
        CHECK(TherumMorphChoice::indexFromMode(MorphMode::Spectral) == 1);
        CHECK(TherumMorphChoice::indexFromMode(TherumMorphChoice::modeFromIndex(1)) == 1);
        CHECK(std::string(TherumMorphChoice::phaseLabels[0]) == "None");
        CHECK(std::string(TherumMorphChoice::phaseLabels[1]) == "Zero Fundamental");
        CHECK(std::string(TherumMorphChoice::phaseLabels[2]) == "Zero All");
        CHECK(TherumMorphChoice::phaseCount == 3);
        CHECK(TherumMorphChoice::phaseDefaultIndex == 0);
        CHECK(TherumMorphChoice::phaseFromIndex(0) == MorphPhase::None);
        CHECK(TherumMorphChoice::phaseFromIndex(1) == MorphPhase::ZeroFundamental);
        CHECK(TherumMorphChoice::phaseFromIndex(2) == MorphPhase::ZeroAll);
        CHECK(TherumMorphChoice::phaseFromIndex(-1) == MorphPhase::None);
        CHECK(TherumMorphChoice::phaseFromIndex(3) == MorphPhase::None);
        CHECK(TherumMorphChoice::phaseFromIndex(99) == MorphPhase::None);
        CHECK(TherumMorphChoice::indexFromPhase(MorphPhase::None) == 0);
        CHECK(TherumMorphChoice::indexFromPhase(MorphPhase::ZeroFundamental) == 1);
        CHECK(TherumMorphChoice::indexFromPhase(MorphPhase::ZeroAll) == 2);
        CHECK(TherumMorphChoice::indexFromPhase(TherumMorphChoice::phaseFromIndex(2)) == 2);
        std::printf("  mode labels: %s / %s; phase labels: %s / %s / %s\n",
                    TherumMorphChoice::modeLabels[0], TherumMorphChoice::modeLabels[1],
                    TherumMorphChoice::phaseLabels[0], TherumMorphChoice::phaseLabels[1],
                    TherumMorphChoice::phaseLabels[2]);
    }

    // --- (i) UI-driven path (voice level). ---
    std::printf("\n-- (i) UI-driven path (voice level) --\n");
    {
        VoiceManager vm;
        vm.prepare(kSr, 16);
        // The exact path the processor runs when the user picks
        // Spectral + Zero All in the OscPanel.
        vm.setMorphA(TherumMorphChoice::modeFromIndex(1), TherumMorphChoice::phaseFromIndex(2));
        vm.setMorphB(TherumMorphChoice::modeFromIndex(1), TherumMorphChoice::phaseFromIndex(2));
        vm.noteOn(69, 1.0f);
        std::vector<float> out((size_t)kSr);
        for (int i = 0; i < (int)kSr; ++i)
            out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
        const std::vector<float> direct = renderVoice(0.5f, 0.5f, MorphMode::Spectral, MorphPhase::ZeroAll,
                                                      MorphMode::Spectral, MorphPhase::ZeroAll, 1.0);
        const std::vector<float> cross = renderVoice(0.5f, 0.5f, MorphMode::Crossfade, MorphPhase::None,
                                                     MorphMode::Crossfade, MorphPhase::None, 1.0);
        const double dDirect = maxDiff(out, direct);
        const double rmsCross = rmsWindow(cross, 0, cross.size());
        const double dRms = rmsDiff(out, cross, 0, cross.size());
        std::printf("  mapped indices vs direct settings: max|diff| %.3g\n", dDirect);
        std::printf("  mapped Spectral/ZeroAll vs Crossfade: RMS diff %.1f%% of signal\n",
                    100.0 * dRms / rmsCross);
        CHECK(dDirect == 0.0); // the mapped path is the direct path
        CHECK(dRms > 0.01 * rmsCross); // it engages
    }

    // --- (j) Rebuild rate (oscillator level). ---
    std::printf("\n-- (j) Rebuild rate (oscillator level) --\n");
    {
        WavetableBank bank = defaultBank();
        WavetableOscillator o;
        o.prepare(kSr);
        o.setBank(&bank);
        o.setFrequency(440.0f);
        o.setMorphMode(MorphMode::Spectral);
        o.setMorphPhase(MorphPhase::None);
        o.setPosition(0.5f);
        // Fixed position, setPosition called every sample for 1 s.
        for (int i = 0; i < (int)kSr; ++i)
        {
            o.setPosition(0.5f);
            (void)o.process();
        }
        const int fixedCount = o.getMorphRecomputeCount();
        // Moving position for 0.1 s: the frame must track the glide.
        for (int i = 0; i < (int)(kSr * 0.1); ++i)
        {
            o.setPosition(0.5f + 0.5f * (float)i / (float)(kSr * 0.1));
            (void)o.process();
        }
        const int movingCount = o.getMorphRecomputeCount() - fixedCount;
        std::printf("  fixed position 1 s: %d rebuild(s); moving 0.1 s: %d rebuild(s)\n",
                    fixedCount, movingCount);
        CHECK(fixedCount == 1); // the no-op guard: built once, never again
        CHECK(movingCount > 10); // the frame tracks the glide
        CHECK(movingCount < 150); // at most every 32 samples
    }

    // --- (k) Position smoothing (voice level). ---
    std::printf("\n-- (k) Position smoothing (voice level) --\n");
    {
        VoiceManager vm;
        vm.prepare(kSr, 16);
        vm.noteOn(69, 1.0f);
        std::vector<float> out((size_t)(kSr * 1.5));
        bool finite = true;
        double maxOut = 0.0;
        for (int i = 0; i < (int)(kSr * 1.5); ++i)
        {
            const float pos = i < (int)(kSr * 0.5) ? 0.0f : 1.0f;
            out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, pos, pos);
            if (!std::isfinite(out[(size_t)i])) finite = false;
            maxOut = std::max(maxOut, (double)std::fabs(out[(size_t)i]));
        }
        // Fresh voice at wtPos 1.0, compared on its settled window.
        const std::vector<float> fresh = renderVoice(1.0f, 1.0f, MorphMode::Crossfade, MorphPhase::None,
                                                     MorphMode::Crossfade, MorphPhase::None, 1.0);
        const double rmsFresh = rmsWindow(fresh, (size_t)(kSr * 0.5), fresh.size());
        const double rmsStepped = rmsWindow(out, (size_t)(kSr * 1.0), out.size());
        std::printf("  step 0.0 -> 1.0: finite = %s, max|out| %.4f\n", finite ? "yes" : "NO", maxOut);
        std::printf("  settled RMS: stepped %.4f vs fresh %.4f (%.1f%% apart)\n",
                    rmsStepped, rmsFresh, 100.0 * std::fabs(rmsStepped - rmsFresh) / rmsFresh);
        CHECK(finite);
        CHECK(maxOut < 4.0);
        CHECK(std::fabs(rmsStepped - rmsFresh) <= 0.10 * rmsFresh); // settled = fresh
    }

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
