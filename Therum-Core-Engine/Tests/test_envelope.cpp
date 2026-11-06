// test_envelope — the amp-envelope ADSR wired into the Therum voice.
//
// Before this work the env1_attack/decay/sustain/release parameters
// existed in the layout but never reached audio: the voice hardcoded
// 10/120/0.75/220 ms, and the ModPanel had no ADSR sliders. The
// processor now reads the four parameters once per block and the
// manager applies them to the voice pool (change-detected). The
// envelope's sustain stage glides to a changed sustain level over
// ~10 ms instead of jumping it in one sample, so a mid-note sustain
// change never clicks.
//
// Expected behaviour (defined BEFORE the tests were run):
//   (E1) Attack: a voice with attack 1000 ms is still ramping at
//        100 ms after note-on, while attack 10 ms is already at
//        sustain — the RMS in [50, 150) ms is < 50% for the slow
//        attack. The attack parameter reaches the envelope.
//   (E2) Sustain: sustain 0.25 vs 0.75 settle at RMS in the ratio
//        1:3 (within 10%) on the settled window [0.5, 1.0) s.
//   (E3) Release tail: noteOff with release 1000 ms keeps the voice
//        active > 5x longer than release 100 ms (from sustain 0.75
//        the tails are 750 ms vs 75 ms — the release slope is 1.0
//        per releaseMs, so the tail is sustain * releaseMs), and
//        getTailLengthSeconds() reports the release time (1.0 vs
//        0.1 s).
//   (E4) Mid-note change is click-free: change sustain 0.75 -> 0.25
//        at 0.5 s into a held note. Finite and bounded; the largest
//        per-sample step inside the 10 ms after the change is at
//        most 2x the largest step in steady state (the E4 criterion
//        from test_voicefilter); after settling, the output matches
//        a fresh voice at sustain 0.25 (last-0.5 s RMS within 10%).
//   (E5) Defaults: without setADSR the voice uses 10/120/0.75/220
//        — a render with no setADSR call is bit-identical to one
//        with setADSR(10, 120, 0.75, 220).
// Build and run: `make envelope` (see Makefile), or:
//   g++ -std=c++17 -O2 -Wall -Wextra -o test_envelope \
//       test_envelope.cpp \
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
//   ./test_envelope
// Exit code 0 means every check passed; non-zero means a check
// failed (failures are printed with file and line).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "../Source/DSP/Voicing/VoiceManager.h"

static constexpr double kSr = 44100.0;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond) do { \
    ++g_checks; \
    if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
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

// Render a note through a fresh VoiceManager with the given ADSR.
// Cutoff 18000 Hz (transparent), reso 0.15, macro 1.0, no FX,
// master 0 dB, note 69, velocity 1.0, positions 0.0.
static std::vector<float> renderVoice(float aMs, float dMs, float s, float rMs, double seconds)
{
    VoiceManager vm;
    vm.prepare(kSr, 16);
    vm.setADSR(aMs, dMs, s, rMs);
    vm.noteOn(69, 1.0f);
    const int n = (int)(seconds * kSr);
    std::vector<float> out((size_t)n);
    for (int i = 0; i < n; ++i)
        out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    return out;
}

int main()
{
    std::printf("Therum envelope test (ADSR wired into the voice)\n");
    std::printf("================================================\n");

    // --- (a) Attack reaches the envelope. ---
    std::printf("\n-- (a) Attack reaches the envelope --\n");
    {
        const std::vector<float> fast = renderVoice(10.0f, 120.0f, 0.75f, 220.0f, 1.0);
        const std::vector<float> slow = renderVoice(1000.0f, 120.0f, 0.75f, 220.0f, 1.0);
        const double rmsFast = rmsWindow(fast, (size_t)(kSr * 0.05), (size_t)(kSr * 0.15));
        const double rmsSlow = rmsWindow(slow, (size_t)(kSr * 0.05), (size_t)(kSr * 0.15));
        std::printf("  RMS [50, 150) ms: attack 10 ms %.4f, attack 1000 ms %.4f (ratio %.2f)\n",
                    rmsFast, rmsSlow, rmsSlow / rmsFast);
        CHECK(rmsSlow < 0.5 * rmsFast); // still ramping at 100 ms
    }

    // --- (b) Sustain sets the settled level. ---
    std::printf("\n-- (b) Sustain sets the settled level --\n");
    {
        const std::vector<float> hi = renderVoice(10.0f, 120.0f, 0.75f, 220.0f, 1.0);
        const std::vector<float> lo = renderVoice(10.0f, 120.0f, 0.25f, 220.0f, 1.0);
        const double rmsHi = rmsWindow(hi, (size_t)(kSr * 0.5), hi.size());
        const double rmsLo = rmsWindow(lo, (size_t)(kSr * 0.5), lo.size());
        const double ratio = rmsLo / rmsHi;
        std::printf("  settled RMS: sustain 0.75 %.4f, sustain 0.25 %.4f (ratio %.3f, expected 0.333)\n",
                    rmsHi, rmsLo, ratio);
        CHECK(std::fabs(ratio - (0.25 / 0.75)) <= 0.10 * (0.25 / 0.75));
    }

    // --- (c) Release tail. ---
    std::printf("\n-- (c) Release tail --\n");
    {
        auto tailSamples = [](float rMs)
        {
            VoiceManager vm;
            vm.prepare(kSr, 16);
            vm.setADSR(10.0f, 120.0f, 0.75f, rMs);
            vm.noteOn(69, 1.0f);
            for (int i = 0; i < (int)(kSr * 0.5); ++i)
                (void)vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vm.noteOff(69);
            int n = 0;
            while (vm.isNoteActive(69) && n < (int)(kSr * 2.0))
            {
                (void)vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                ++n;
            }
            return n;
        };
        const int tailLong = tailSamples(1000.0f);
        const int tailShort = tailSamples(100.0f);
        std::printf("  tail: release 1000 ms %d samples (%.0f ms), release 100 ms %d samples (%.0f ms)\n",
                    tailLong, 1000.0 * tailLong / kSr, tailShort, 1000.0 * tailShort / kSr);
        CHECK(tailLong > 5 * tailShort);
        // The tail from sustain 0.75 is 0.75 * releaseMs.
        CHECK(std::fabs(tailLong / kSr - 0.75) < 0.05);
        CHECK(std::fabs(tailShort / kSr - 0.075) < 0.02);

        VoiceManager vm;
        vm.prepare(kSr, 16);
        vm.setADSR(10.0f, 120.0f, 0.75f, 1000.0f);
        const float tail = vm.getTailLengthSeconds();
        std::printf("  getTailLengthSeconds (release 1000 ms): %.2f s\n", tail);
        CHECK(std::fabs(tail - 1.0f) < 0.01f);
    }

    // --- (d) Mid-note sustain change is click-free. ---
    std::printf("\n-- (d) Mid-note sustain change is click-free --\n");
    {
        VoiceManager vm;
        vm.prepare(kSr, 16);
        vm.setADSR(10.0f, 120.0f, 0.75f, 220.0f);
        vm.noteOn(69, 1.0f);
        const int half = (int)(kSr * 0.5);
        std::vector<float> out((size_t)(kSr * 1.5));
        double maxStepSteady = 0.0;
        for (int i = 0; i < half; ++i)
        {
            out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            if (i >= (int)(kSr * 0.25) && i > 0)
                maxStepSteady = std::max(maxStepSteady, (double)std::fabs((double)out[(size_t)i] - (double)out[(size_t)(i - 1)]));
        }
        // Change sustain 0.75 -> 0.25 mid-note.
        vm.setADSR(10.0f, 120.0f, 0.25f, 220.0f);
        double maxStepChange = 0.0;
        bool finite = true;
        for (int i = half; i < (int)(kSr * 1.5); ++i)
        {
            out[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            if (!std::isfinite(out[(size_t)i])) finite = false;
            if (i < half + (int)(kSr * 0.010))
                maxStepChange = std::max(maxStepChange, (double)std::fabs((double)out[(size_t)i] - (double)out[(size_t)(i - 1)]));
        }
        const std::vector<float> fresh = renderVoice(10.0f, 120.0f, 0.25f, 220.0f, 1.0);
        const double rmsFresh = rmsWindow(fresh, (size_t)(kSr * 0.5), fresh.size());
        const double rmsChanged = rmsWindow(out, (size_t)(kSr * 1.0), out.size());
        std::printf("  steady max step %.4f; change-window max step %.4f (ratio %.2f)\n",
                    maxStepSteady, maxStepChange, maxStepChange / maxStepSteady);
        std::printf("  settled RMS: changed %.4f vs fresh %.4f (%.1f%% apart)\n",
                    rmsChanged, rmsFresh, 100.0 * std::fabs(rmsChanged - rmsFresh) / rmsFresh);
        CHECK(finite);
        CHECK(maxStepChange <= 2.0 * maxStepSteady); // the E4 criterion
        CHECK(std::fabs(rmsChanged - rmsFresh) <= 0.10 * rmsFresh);
    }

    // --- (e) Defaults. ---
    std::printf("\n-- (e) Defaults --\n");
    {
        VoiceManager vm;
        vm.prepare(kSr, 16);
        vm.noteOn(69, 1.0f);
        std::vector<float> outDefault((size_t)kSr);
        for (int i = 0; i < (int)kSr; ++i)
            outDefault[(size_t)i] = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        const std::vector<float> outExplicit = renderVoice(10.0f, 120.0f, 0.75f, 220.0f, 1.0);
        const double d = maxDiff(outDefault, outExplicit);
        std::printf("  no setADSR vs setADSR(10, 120, 0.75, 220): max|diff| %.3g\n", d);
        CHECK(d == 0.0); // the hardcoded default is the parameter default
    }

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
