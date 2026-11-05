// Therum DSP core unit tests.
//
// Standalone C++ tests for the framework-free DSP core (paper section 6,
// rank 1). They compile ONLY the DSP files — no JUCE, no iPlug2, no DAW,
// no test framework. If any DSP file included a framework header, this
// build would fail (no framework is on the include path), so a passing
// build is itself the proof that the core is framework-free.
//
// Build and run: `make` (see Makefile), or:
//   g++ -std=c++17 -O2 -Wall -Wextra -o test_core test_core.cpp
//       ../Source/DSP/Oscillators/WavetableOscillator.cpp
//       ../Source/DSP/Oscillators/WavetableBank.cpp
//       ../Source/DSP/Filters/SimpleLPF.cpp
//       ../Source/DSP/Modulation/EnvelopeGenerator.cpp
//       ../Source/DSP/Voicing/TherumVoice.cpp
//       ../Source/DSP/Voicing/VoiceManager.cpp
//       ../Source/DSP/FX/FxRack.cpp
//   ./test_core
//
// Exit code 0 means every check passed; non-zero means a check failed
// (failures are printed with file and line).

#include <cstdio>
#include <cmath>
#include <vector>

#include "../Source/DSP/Voicing/VoiceManager.h"
#include "../Source/DSP/Utils/SmoothedParameter.h"
#include "../Source/DSP/Utils/Limiter.h"
#include "../Source/DSP/Utils/Fft.h"
#include "../Source/DSP/Oscillators/WavetableBank.h"

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

// Render one sample with a wide-open filter and no FX, master at 0 dB.
// macro = 1 so the modulated cutoff equals the cutoff passed in.
static float renderOne(VoiceManager& vm, float cutoff = 18000.0f, float macro = 1.0f)
{
    return vm.renderMixed(cutoff, macro, 0.0f, 0.0f, 0.0f, 0.0f);
}

static void testReleaseTail()
{
    std::printf("testReleaseTail...\n");
    VoiceManager vm;
    vm.prepare(44100.0, 16);

    vm.noteOn(60, 1.0f);
    float peak = 0.0f;
    for (int i = 0; i < 500; ++i)
        peak = std::fmax(peak, std::fabs(renderOne(vm)));
    CHECK(peak > 0.01f); // the voice is sounding before note-off

    vm.noteOff(60);
    CHECK(vm.getActiveVoiceCount() == 1); // still active: it is releasing

    // The release is 220 ms (~9700 samples). Count how many samples after
    // note-off are still audible: the tail must actually sound.
    int audible = 0;
    for (int i = 0; i < 20000; ++i)
    {
        if (std::fabs(renderOne(vm)) > 1e-6f)
            ++audible;
    }
    CHECK(audible > 5000); // most of the ~9700-sample release is audible
    CHECK(vm.getActiveVoiceCount() == 0); // the voice freed itself
    CHECK(renderOne(vm) == 0.0f); // and renders silence once free
    CHECK_NEAR(vm.getTailLengthSeconds(), 0.22f, 0.001f);
}

static void testVoiceStealingRoundRobin()
{
    std::printf("testVoiceStealingRoundRobin...\n");
    VoiceManager vm;
    vm.prepare(44100.0, 16);

    for (int n = 60; n < 76; ++n)
        vm.noteOn(n, 1.0f);
    CHECK(vm.getActiveVoiceCount() == 16);

    // First steal takes the voice at the cursor (note 60). The second and
    // third steals must take the NEXT voices (notes 61, 62) — not voice 0
    // again. That is what round-robin means, and it is what the old code
    // got wrong (it always stole voice 0).
    vm.noteOn(76, 1.0f);
    CHECK(vm.getActiveVoiceCount() == 16);
    CHECK(!vm.isNoteActive(60));
    CHECK(vm.isNoteActive(76));

    vm.noteOn(77, 1.0f);
    CHECK(!vm.isNoteActive(61));
    CHECK(vm.isNoteActive(77));

    vm.noteOn(78, 1.0f);
    CHECK(!vm.isNoteActive(62));
    CHECK(vm.isNoteActive(78));
    CHECK(vm.isNoteActive(63)); // untouched voices are still sounding
}

static void testDuplicateNoteRetrigger()
{
    std::printf("testDuplicateNoteRetrigger...\n");
    VoiceManager vm;
    vm.prepare(44100.0, 16);

    vm.noteOn(60, 1.0f);
    vm.noteOn(60, 1.0f); // same note again: retrigger, not a duplicate
    CHECK(vm.getActiveVoiceCount() == 1);
    CHECK(vm.isNoteActive(60));

    vm.noteOn(60, 0.5f);
    CHECK(vm.getActiveVoiceCount() == 1);
}

static void testPhaseReset()
{
    std::printf("testPhaseReset...\n");
    // A fresh voice's first sample, for the reference.
    VoiceManager fresh;
    fresh.prepare(44100.0, 16);
    fresh.noteOn(60, 1.0f);
    const float firstSample = renderOne(fresh);

    // A voice that has played, released, and been retriggered must produce
    // the same first sample: its phase (and filter state) reset on start.
    VoiceManager vm;
    vm.prepare(44100.0, 16);
    vm.noteOn(60, 1.0f);
    for (int i = 0; i < 1000; ++i)
        renderOne(vm);
    vm.noteOff(60);
    for (int i = 0; i < 20000; ++i)
        renderOne(vm);
    CHECK(vm.getActiveVoiceCount() == 0);

    vm.noteOn(60, 1.0f);
    const float retriggered = renderOne(vm);
    CHECK_NEAR(retriggered, firstSample, 1e-7f);
}

static void testCutoffClamp()
{
    std::printf("testCutoffClamp...\n");
    VoiceManager vm;
    vm.prepare(44100.0, 16);
    vm.noteOn(60, 1.0f);

    // Every one of these used to be unclamped (or NaN). None may produce a
    // non-finite sample, and none may crash.
    const float cutoffs[] = { 0.0f, -100.0f, 30000.0f, 1e9f, std::nanf("") };
    for (float c : cutoffs)
    {
        for (int i = 0; i < 100; ++i)
        {
            const float s = vm.renderMixed(c, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            CHECK(std::isfinite(s));
        }
    }
    // And the voice is still sounding afterwards (the bad values did not
    // break the filter).
    CHECK(vm.getActiveVoiceCount() == 1);
}

static void testSmoothing()
{
    std::printf("testSmoothing...\n");
    // The smoother itself: a target change glides, it does not jump.
    SmoothedParameter sm;
    sm.prepare(44100.0, 10.0f);
    sm.setTarget(1.0f);
    CHECK_NEAR(sm.process(), 1.0f, 1e-6f); // first set snaps, no ramp from 0
    sm.setTarget(0.0f);
    const float afterOne = sm.process();
    CHECK(afterOne > 0.9f && afterOne < 1.0f); // gliding, not jumped
    for (int i = 0; i < 1000; ++i)
        sm.process();
    CHECK(sm.getValue() < 0.5f); // and it does arrive
    sm.setValueImmediate(0.25f);
    CHECK_NEAR(sm.getValue(), 0.25f, 1e-6f);

    // Through the voice manager: move the cutoff from wide open to 100 Hz
    // while a 440 Hz note sustains. The output must stay loud while the
    // smoother is still near 18000 Hz, and fall once it has settled near
    // 100 Hz (a one-pole at 100 Hz passes ~22% of 440 Hz).
    //
    // RMS is measured over a full beat period (20000 samples ~ 454 ms).
    // The two oscillators are detuned (440 Hz and 442.2 Hz) and beat at
    // ~2.2 Hz, so a short window would catch an arbitrary point in the
    // beat and the comparison would be meaningless.
    VoiceManager vm;
    vm.prepare(44100.0, 16);
    vm.noteOn(69, 1.0f); // A4, 440 Hz
    for (int i = 0; i < 10000; ++i)
        renderOne(vm); // reach sustain, smoother settled at 18000

    float openRms = 0.0f;
    for (int i = 0; i < 20000; ++i)
    {
        const float s = renderOne(vm);
        openRms += s * s;
    }
    openRms = std::sqrt(openRms / 20000.0f);

    // Move the cutoff to 100 Hz, let the smoother and the filter settle
    // (20000 samples is far longer than the 10 ms smoothing time constant
    // and the filter's settling time), then measure again over a beat.
    for (int i = 0; i < 20000; ++i)
        vm.renderMixed(100.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    float lateRms = 0.0f;
    for (int i = 0; i < 20000; ++i)
    {
        const float s = vm.renderMixed(100.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        lateRms += s * s;
    }
    lateRms = std::sqrt(lateRms / 20000.0f);

    CHECK(openRms > 0.01f);
    CHECK(lateRms < openRms * 0.5f); // settled near 100 Hz: heavily cut
}

static void testLimiter()
{
    std::printf("testLimiter...\n");
    VoiceManager vm;
    vm.prepare(44100.0, 16);

    // All 16 voices at full velocity: without a limiter the sum would clip
    // hard (16 x 0.2 = 3.2 peak). With the limiter it must stay at 0.99.
    for (int n = 60; n < 76; ++n)
        vm.noteOn(n, 1.0f);

    float maxAbs = 0.0f;
    for (int i = 0; i < 20000; ++i)
    {
        const float s = vm.renderMixed(18000.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        CHECK(std::isfinite(s));
        maxAbs = std::fmax(maxAbs, std::fabs(s));
    }
    CHECK(maxAbs <= 0.99f + 1e-6f);
    CHECK(maxAbs > 0.5f); // the limiter is capping, not silencing

    // And at +12 dB master gain: still capped, still finite.
    maxAbs = 0.0f;
    for (int i = 0; i < 5000; ++i)
    {
        const float s = vm.renderMixed(18000.0f, 1.0f, 0.0f, 0.0f, 0.0f, 12.0f);
        CHECK(std::isfinite(s));
        maxAbs = std::fmax(maxAbs, std::fabs(s));
    }
    CHECK(maxAbs <= 0.99f + 1e-6f);

    // The limiter itself, on its own: silence and quiet signals pass
    // through untouched (gain stays at 1.0).
    Limiter lim;
    lim.prepare(44100.0);
    lim.setThreshold(0.99f);
    CHECK_NEAR(lim.process(0.0f), 0.0f, 1e-6f);
    CHECK_NEAR(lim.process(0.5f), 0.5f, 1e-6f);
    CHECK_NEAR(lim.process(-0.5f), -0.5f, 1e-6f);
    CHECK_NEAR(lim.process(2.0f), 0.99f, 1e-6f);
}

static void testFft()
{
    std::printf("testFft...\n");
    using namespace therum::fft;

    // A deterministic test signal: three harmonics plus DC.
    const int n = 2048;
    std::vector<float> signal((size_t)n);
    for (int i = 0; i < n; ++i)
    {
        const float p = 6.28318530718f * (float)i / (float)n;
        signal[(size_t)i] = 0.25f + 0.5f * std::sin(p * 3.0f)
                          + 0.25f * std::sin(p * 7.0f + 0.6f)
                          + 0.125f * std::sin(p * 40.0f);
    }

    // Forward: the spectrum must have energy only at bins 0, 3, 7, 40.
    std::vector<std::complex<float>> bins;
    CHECK(realForward(signal.data(), n, bins));
    CHECK(bins.size() == (size_t)(n / 2 + 1));
    CHECK_NEAR(std::abs(bins[0]), 0.25f * (float)n, 0.01f);
    CHECK_NEAR(std::abs(bins[3]), 0.5f * (float)n * 0.5f, 0.01f);
    CHECK_NEAR(std::abs(bins[7]), 0.25f * (float)n * 0.5f, 0.01f);
    CHECK_NEAR(std::abs(bins[40]), 0.125f * (float)n * 0.5f, 0.01f);
    float other = 0.0f;
    for (int k = 1; k <= n / 2; ++k)
        if (k != 3 && k != 7 && k != 40)
            other = std::fmax(other, std::abs(bins[(size_t)k]));
    CHECK(other < 0.01f); // no leakage into any other bin

    // Round-trip: inverse(forward(x)) == x.
    std::vector<float> back((size_t)n, 0.0f);
    CHECK(realInverse(bins, back.data(), n));
    for (int i = 0; i < n; ++i)
        CHECK_NEAR(back[(size_t)i], signal[(size_t)i], 1e-4f);

    // Zeroing bins above a harmonic and inverting band-limits the signal:
    // keep only bins 0..7 and the 40th harmonic must be gone.
    std::vector<std::complex<float>> limited = bins;
    for (int k = 8; k <= n / 2; ++k)
        limited[(size_t)k] = { 0.0f, 0.0f };
    CHECK(realInverse(limited, back.data(), n));
    std::vector<std::complex<float>> check;
    CHECK(realForward(back.data(), n, check));
    CHECK(std::abs(check[40]) < 0.01f);
    CHECK_NEAR(std::abs(check[3]), 0.5f * (float)n * 0.5f, 0.01f);

    // Non-power-of-two lengths are rejected, never miscomputed.
    std::vector<std::complex<float>> bad((size_t)100, { 1.0f, 0.0f });
    CHECK(!transform(bad, false));
    CHECK(!realForward(signal.data(), 100, bins));
    CHECK(!isPowerOfTwo(100));
    CHECK(isPowerOfTwo(2048));
    CHECK(isPowerOfTwo(1));
}

int main()
{
    std::printf("Therum DSP core tests (framework-free, no DAW)\n");
    std::printf("==============================================\n");

    testFft();
    testReleaseTail();
    testVoiceStealingRoundRobin();
    testDuplicateNoteRetrigger();
    testPhaseReset();
    testCutoffClamp();
    testSmoothing();
    testLimiter();

    std::printf("==============================================\n");
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    if (g_failures == 0)
        std::printf("ALL TESTS PASSED\n");
    return g_failures == 0 ? 0 : 1;
}
