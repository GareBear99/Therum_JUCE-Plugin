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
//       ../Source/DSP/Filters/SvfFilter.cpp
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
#include <cstring>
#include <vector>

#include "../Source/DSP/Voicing/VoiceManager.h"
#include "../Source/DSP/Utils/SmoothedParameter.h"
#include "../Source/DSP/Utils/Limiter.h"
#include "../Source/DSP/Utils/Fft.h"
#include "../Source/DSP/Utils/WavImport.h"
#include "../Source/DSP/Oscillators/WavetableBank.h"
#include "../Source/DSP/Filters/SvfFilter.h"
#include "../Source/DSP/Filters/DualFilter.h"
#include "RbjBiquad.h"

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
    return vm.renderMixed(cutoff, 0.15f, macro, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

static float oscRms(WavetableOscillator& osc, int samples)
{
    double sum = 0.0;
    for (int i = 0; i < samples; ++i)
    {
        const float s = osc.process();
        sum += (double)s * (double)s;
    }
    return (float)std::sqrt(sum / (double)samples);
}

static void testWavetableEngine()
{
    std::printf("testWavetableEngine...\n");

    // Default bank: 4 classic frames, 2048 samples each, position 0 = sine.
    WavetableBank bank;
    bank.buildDefaultBank();
    CHECK(bank.getFrameCount() == 4);
    CHECK(bank.isReady());
    CHECK(bank.getFrame(0).size() == (size_t)WavetableBank::kFrameSize);
    CHECK_NEAR(bank.getFrame(0)[0], 0.0f, 1e-6f); // sine starts at zero
    float sineMax = 0.0f;
    for (float s : bank.getFrame(0))
        sineMax = std::fmax(sineMax, std::fabs(s));
    CHECK_NEAR(sineMax, 1.0f, 1e-6f);
    // Frame 2 is a saw ramp: starts at -1, ends just below +1.
    CHECK_NEAR(bank.getFrame(2)[0], -1.0f, 1e-6f);
    CHECK(bank.getFrame(2)[2047] > 0.99f);
    // Frame 3 is a square: every sample is exactly +1 or -1.
    for (float s : bank.getFrame(3))
        CHECK(s == 1.0f || s == -1.0f);
    // Out of range returns an empty table, never a crash.
    CHECK(bank.getFrame(99).empty());
    CHECK(bank.getFrame(-1).empty());

    // Compatibility: the old single-sine build still works.
    WavetableBank sineBank;
    sineBank.buildDefaultSineTable(2048);
    CHECK(sineBank.getFrameCount() == 1);
    CHECK_NEAR(sineBank.getTable()[512], 1.0f, 1e-5f); // quarter cycle

    // addFrame resamples a short cycle up to 2048.
    WavetableBank custom;
    std::vector<float> cycle(512);
    for (int i = 0; i < 512; ++i)
        cycle[(size_t)i] = std::sin(6.28318530718f * (float)i / 512.0f);
    CHECK(custom.addFrame(cycle.data(), 512));
    CHECK(custom.getFrameCount() == 1);
    CHECK(custom.getFrame(0).size() == (size_t)WavetableBank::kFrameSize);
    CHECK_NEAR(custom.getFrame(0)[512], 1.0f, 1e-3f); // quarter cycle kept
    CHECK(!custom.addFrame(nullptr, 512));
    CHECK(!custom.addFrame(cycle.data(), 0));

    // Position crossfade: frame 0 = sine, frame 1 = -sine. At 0.5 they
    // cancel exactly; at 0 and 1 the RMS is a full sine.
    WavetableBank two;
    two.addFrame(bank.getFrame(0).data(), 2048);
    std::vector<float> neg(2048);
    for (int i = 0; i < 2048; ++i)
        neg[(size_t)i] = -bank.getFrame(0)[(size_t)i];
    two.addFrame(neg.data(), 2048);

    WavetableOscillator osc;
    osc.prepare(44100.0);
    osc.setBank(&two);
    osc.setFrequency(440.0f);
    osc.setPosition(0.0f);
    CHECK_NEAR(oscRms(osc, 44100), 0.7071f, 0.01f);
    osc.resetPhase();
    osc.setPosition(1.0f);
    CHECK_NEAR(oscRms(osc, 44100), 0.7071f, 0.01f);
    osc.resetPhase();
    osc.setPosition(0.5f);
    CHECK(oscRms(osc, 44100) < 1e-6f); // exact cancellation
    // Position clamps instead of wrapping or crashing.
    osc.setPosition(2.0f);
    CHECK_NEAR(osc.getPosition(), 1.0f, 1e-6f);
    osc.setPosition(-1.0f);
    CHECK_NEAR(osc.getPosition(), 0.0f, 1e-6f);

    // Interpolation accuracy on a 2048 sine at 440 Hz, one second.
    // Hermite (Catmull-Rom) must be far more accurate than linear; the
    // measured errors are printed so the bounds below are justified.
    // The reference is computed in double: a float reference carries
    // ~2e-4 error of its own (found while writing this test — the first
    // version measured the reference, not the oscillator), which swamps
    // both interpolation errors.
    WavetableOscillator hermite;
    hermite.prepare(44100.0);
    hermite.setBank(&sineBank);
    hermite.setFrequency(440.0f);
    hermite.setInterpolation(WavetableOscillator::Interpolation::Hermite);
    double maxErrH = 0.0;
    for (int i = 0; i < 44100; ++i)
    {
        const double want = std::sin(6.2831853071795864769 * 440.0 * (double)i / 44100.0);
        maxErrH = std::fmax(maxErrH, std::fabs((double)hermite.process() - want));
    }
    WavetableOscillator linear;
    linear.prepare(44100.0);
    linear.setBank(&sineBank);
    linear.setFrequency(440.0f);
    linear.setInterpolation(WavetableOscillator::Interpolation::Linear);
    double maxErrL = 0.0;
    for (int i = 0; i < 44100; ++i)
    {
        const double want = std::sin(6.2831853071795864769 * 440.0 * (double)i / 44100.0);
        maxErrL = std::fmax(maxErrL, std::fabs((double)linear.process() - want));
    }
    std::printf("  hermite max err %.3g, linear max err %.3g\n", maxErrH, maxErrL);
    CHECK(maxErrH < 1e-5);
    CHECK(maxErrL < 1e-5);
    CHECK(maxErrH < maxErrL); // Hermite is measurably more accurate
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
            const float s = vm.renderMixed(c, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
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
        vm.renderMixed(100.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    float lateRms = 0.0f;
    for (int i = 0; i < 20000; ++i)
    {
        const float s = vm.renderMixed(100.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
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
        const float s = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        CHECK(std::isfinite(s));
        maxAbs = std::fmax(maxAbs, std::fabs(s));
    }
    CHECK(maxAbs <= 0.99f + 1e-6f);
    CHECK(maxAbs > 0.5f); // the limiter is capping, not silencing

    // And at +12 dB master gain: still capped, still finite.
    maxAbs = 0.0f;
    for (int i = 0; i < 5000; ++i)
    {
        const float s = vm.renderMixed(18000.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 12.0f, 0.0f, 0.0f);
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

// Alias-to-harmonic energy ratio (dB) of a rendered oscillator tone.
// Renders 2 seconds, FFTs the last 16384 samples under a Blackman
// window, and sums the energy at non-harmonic bins vs harmonic bins (a
// bin is harmonic if it is within 3 bins of a multiple of f0 — the
// Blackman main lobe is +-3 bins wide). Blackman, not Hann: Hann's
// -31 dB sidelobes set a leakage floor that swamps the measurement
// (found while writing this test), Blackman's are -58 dB. This is the
// same idea as the Bench alias test, reduced to one number for the
// unit test; the full Bench run on a real render is piece 6.
static double aliasRatioDb(WavetableOscillator& osc, float f0, double sr)
{
    const int total = (int)(2.0 * sr);
    const int n = 16384;
    std::vector<float> tail((size_t)n);
    osc.setFrequency(f0);
    osc.resetPhase();
    for (int i = 0; i < total; ++i)
    {
        const float s = osc.process();
        if (i >= total - n)
            tail[(size_t)(i - (total - n))] = s;
    }
    // Blackman window, then FFT.
    std::vector<std::complex<float>> bins;
    std::vector<float> win((size_t)n);
    for (int i = 0; i < n; ++i)
    {
        const float a = 6.2831853071795864769f * (float)i / (float)(n - 1);
        const float w = 0.42f - 0.5f * std::cos(a) + 0.08f * std::cos(2.0f * a);
        win[(size_t)i] = tail[(size_t)i] * w;
    }
    CHECK(therum::fft::realForward(win.data(), n, bins));
    const double binHz = sr / (double)n;
    double harmonicE = 0.0, aliasE = 0.0;
    for (int k = 1; k <= n / 2; ++k)
    {
        const double mag2 = std::norm(bins[(size_t)k]);
        // Nearest harmonic index to this bin.
        const double h = std::round((double)k * binHz / (double)f0);
        const double harmBin = h * (double)f0 / binHz;
        if (h >= 1.0 && std::fabs((double)k - harmBin) <= 3.0)
            harmonicE += mag2;
        else
            aliasE += mag2;
    }
    return 10.0 * std::log10(aliasE / harmonicE);
}

static void testMipmaps()
{
    std::printf("testMipmaps...\n");

    // One naive saw frame (a ramp -1 -> +1). Its harmonics fall as 1/k,
    // so it is the worst case for aliasing: every harmonic above
    // Nyquist folds back audibly.
    std::vector<float> saw((size_t)WavetableBank::kFrameSize);
    for (int i = 0; i < WavetableBank::kFrameSize; ++i)
        saw[(size_t)i] = -1.0f + 2.0f * (float)i / (float)WavetableBank::kFrameSize;

    WavetableBank bank;
    CHECK(bank.addFrame(saw.data(), (int)saw.size()));
    CHECK(!bank.hasMipmaps());
    CHECK(bank.buildMipmaps());
    CHECK(bank.hasMipmaps());
    const int levels = bank.getMipLevelCount();
    std::printf("  mip levels: %d\n", levels);
    CHECK(levels >= 30);

    // Ladder: strictly decreasing harmonic caps, 1024 down to 1, and
    // every table length is a power of two >= max(4, 2*K).
    const auto& mips = bank.getFrameMips(0);
    CHECK(mips.levels[0].maxHarmonic == 1024);
    CHECK(mips.levels[(size_t)(levels - 1)].maxHarmonic == 1);
    for (int i = 0; i < levels; ++i)
    {
        const auto& lv = mips.levels[(size_t)i];
        if (i > 0)
            CHECK(lv.maxHarmonic < mips.levels[(size_t)(i - 1)].maxHarmonic);
        const int len = (int)lv.table.size();
        CHECK(therum::fft::isPowerOfTwo((size_t)len));
        CHECK(len >= 4 && len >= 2 * lv.maxHarmonic);
        for (float s : lv.table)
            CHECK(std::isfinite(s));
    }

    // Harmonic content of the K=128 level: the saw's bins 1..128 are
    // kept, bins above 128 are gone. (The naive saw's bin k has
    // magnitude ~ 1/k of the fundamental, so bin 1 is the reference.)
    int idx128 = -1;
    for (int i = 0; i < levels; ++i)
        if (mips.levels[(size_t)i].maxHarmonic == 128)
            idx128 = i;
    CHECK(idx128 >= 0);
    const auto& lv128 = mips.levels[(size_t)idx128];
    std::vector<std::complex<float>> bins;
    CHECK(therum::fft::realForward(lv128.table.data(), (int)lv128.table.size(), bins));
    const double fund = std::abs(bins[1]);
    CHECK(fund > 0.1);
    for (int k = 129; k <= (int)lv128.table.size() / 2; ++k)
        CHECK(std::abs(bins[(size_t)k]) < fund * 1e-3);
    // And bin 64 is still there (the level keeps what it claims to).
    CHECK(std::abs(bins[64]) > fund * 1e-3);

    // Mip selection: at f0 = sr/(2*512) the allowed max is exactly 512,
    // so the selection is pure the K=512 level — expressed as the
    // bracketing pair (609, 512) with alpha 1.0 (all hi), not as lo==hi.
    WavetableOscillator osc;
    osc.prepare(44100.0);
    osc.setBank(&bank);
    osc.setFrequency(44100.0f / (2.0f * 512.0f));
    int idx512 = -1;
    for (int i = 0; i < levels; ++i)
        if (mips.levels[(size_t)i].maxHarmonic == 512)
            idx512 = i;
    CHECK(idx512 >= 0);
    CHECK(osc.getMipHi() == idx512 && osc.getMipAlpha() == 1.0f);

    // Between levels: f0 giving kAllowed = 400 sits between the K=431
    // and K=362 levels, so the blend is strictly inside (0, 1).
    osc.setFrequency(44100.0f / (2.0f * 400.0f));
    CHECK(osc.getMipAlpha() > 0.0f && osc.getMipAlpha() < 1.0f);
    CHECK(osc.getMipLo() >= 0 && osc.getMipHi() == osc.getMipLo() + 1);

    // Alias suppression, the headline. Two frequencies:
    // (a) f0 = sr/(2*19): the allowed max is exactly 19, so the
    // selection is pure the K=19 level — no crossfade contribution, no
    // harmonic above 19 exists. The mipped render must be essentially
    // alias-free (only window leakage remains), far below the naive.
    // (b) 997 Hz (the Bench's frequency): here the crossfade holds a
    // ~24% share of the K=23 level, and harmonic 23 folds — so the
    // mipped render is better than naive but not alias-free. That is
    // the crossfade trade, documented in the oscillator, and the bound
    // here is the measured one (10 dB), not the 20 dB the pure case
    // shows. The Bench's alias test measures -15.8 dB ASR for the
    // naive 997 Hz saw; the naive number below should land near it.
    WavetableOscillator naive;
    naive.prepare(44100.0);
    WavetableBank naiveBank;
    CHECK(naiveBank.addFrame(saw.data(), (int)saw.size()));
    naive.setBank(&naiveBank);
    WavetableOscillator mipped;
    mipped.prepare(44100.0);
    mipped.setBank(&bank);
    const float fPure = 44100.0f / (2.0f * 19.0f);
    const double naivePure = aliasRatioDb(naive, fPure, 44100.0);
    const double mipPure = aliasRatioDb(mipped, fPure, 44100.0);
    const double naiveDb = aliasRatioDb(naive, 997.0f, 44100.0);
    const double mipDb = aliasRatioDb(mipped, 997.0f, 44100.0);
    std::printf("  alias/harmonic ratio, pure level (%.1f Hz): naive %.1f dB, mipped %.1f dB\n",
                fPure, naivePure, mipPure);
    std::printf("  alias/harmonic ratio at 997 Hz: naive %.1f dB, mipped %.1f dB\n",
                naiveDb, mipDb);
    CHECK(mipPure < -40.0);              // essentially alias-free
    CHECK(mipPure < naivePure - 20.0);   // at least 20 dB cleaner
    CHECK(mipDb < naiveDb - 10.0);       // crossfade case, measured bound
}

// Amplitudes of harmonics 1..3 of a rendered oscillator tone, measured
// with an exact-cycle FFT: f0 = sr/128, so one cycle is 128 samples and
// 4096 samples hold exactly 32 cycles — a rectangular window leaks
// nothing, and harmonic k sits exactly at bin 32*k with amplitude
// 2*|X[32k]|/4096.
static void harmonicAmps(WavetableOscillator& osc, double sr,
                         double& a1, double& a2, double& a3)
{
    const double f0 = sr / 128.0;
    const int n = 4096;
    std::vector<float> buf((size_t)n);
    osc.setFrequency((float)f0);
    osc.resetPhase();
    for (int i = 0; i < n; ++i)
        buf[(size_t)i] = osc.process();
    std::vector<std::complex<float>> bins;
    CHECK(therum::fft::realForward(buf.data(), n, bins));
    a1 = 2.0 * std::abs(bins[32]) / (double)n;
    a2 = 2.0 * std::abs(bins[64]) / (double)n;
    a3 = 2.0 * std::abs(bins[96]) / (double)n;
}

static void testSpectralMorph()
{
    std::printf("testSpectralMorph...\n");
    const double sr = 44100.0;

    // Two frames: sine and naive saw. The saw's harmonic k has amplitude
    // 2/(pi*k); the sine's is 1 at k=1 and 0 elsewhere.
    std::vector<float> sine((size_t)WavetableBank::kFrameSize);
    std::vector<float> saw((size_t)WavetableBank::kFrameSize);
    for (int i = 0; i < WavetableBank::kFrameSize; ++i)
    {
        const float ph = (float)i / (float)WavetableBank::kFrameSize;
        sine[(size_t)i] = std::sin(6.2831853071795864769f * ph);
        saw[(size_t)i] = 2.0f * ph - 1.0f;
    }
    WavetableBank bank;
    CHECK(bank.addFrame(sine.data(), (int)sine.size()));
    CHECK(bank.addFrame(saw.data(), (int)saw.size()));
    CHECK(bank.buildMipmaps());
    CHECK(bank.hasSpectra());

    // Spectral morph at position 0.5 (None phase mode): the morphed
    // frame's harmonics are the 50/50 blend of the two spectra. The
    // saw's Fourier series is -(2/pi) * sum(sin(2*pi*k*t)/k), so its
    // fundamental is a NEGATIVE sine and the blend partially cancels
    // there: a1 = |0.5*1 - 0.5*(2/pi)| = 0.182, a2 = 0.5*(2/(2pi)) =
    // 0.159, a3 = 0.5*(2/(3pi)) = 0.106.
    WavetableOscillator osc;
    osc.prepare(sr);
    osc.setBank(&bank);
    osc.setMorphMode(WavetableOscillator::MorphMode::Spectral);
    osc.setPosition(0.5f);
    // Throttle first, while the rebuild counter is fresh: the first
    // sample builds the frame; a position change then rebuilds only
    // after 32 samples have passed, and only once.
    (void)osc.process();
    CHECK(osc.getMorphRecomputeCount() == 1);
    osc.setPosition(0.75f);
    for (int i = 0; i < 16; ++i)
        (void)osc.process();
    CHECK(osc.getMorphRecomputeCount() == 1); // throttled
    for (int i = 0; i < 16; ++i)
        (void)osc.process();
    CHECK(osc.getMorphRecomputeCount() == 2); // rebuilt at 32
    for (int i = 0; i < 100; ++i)
        (void)osc.process();
    CHECK(osc.getMorphRecomputeCount() == 2); // not dirty anymore
    // Back to 0.5 for the spectrum measurement (rebuilds immediately —
    // the frame has been still for 100 samples).
    osc.setPosition(0.5f);
    double a1 = 0, a2 = 0, a3 = 0;
    harmonicAmps(osc, sr, a1, a2, a3);
    std::printf("  spectral morph 0.5 (None): a1 %.3f a2 %.3f a3 %.3f\n", a1, a2, a3);
    CHECK(std::fabs(a1 - 0.182) < 0.02);
    CHECK(std::fabs(a2 - 0.159) < 0.02);
    CHECK(std::fabs(a3 - 0.106) < 0.02);

    // ZeroFundamental vs None on phase-shifted frames. Frame B is a
    // cosine (sine shifted 90 degrees). At 0.5, None blends (sin+cos)/2
    // — amplitude 0.707; ZeroFundamental rotates B's fundamental onto
    // A's first, so the blend is a full-amplitude sine.
    std::vector<float> cosine((size_t)WavetableBank::kFrameSize);
    for (int i = 0; i < WavetableBank::kFrameSize; ++i)
    {
        const float ph = (float)i / (float)WavetableBank::kFrameSize;
        cosine[(size_t)i] = std::cos(6.2831853071795864769f * ph);
    }
    WavetableBank phaseBank;
    CHECK(phaseBank.addFrame(sine.data(), (int)sine.size()));
    CHECK(phaseBank.addFrame(cosine.data(), (int)cosine.size()));
    CHECK(phaseBank.buildMipmaps());
    WavetableOscillator ph;
    ph.prepare(sr);
    ph.setBank(&phaseBank);
    ph.setMorphMode(WavetableOscillator::MorphMode::Spectral);
    ph.setPosition(0.5f);
    ph.setMorphPhase(WavetableOscillator::MorphPhase::None);
    harmonicAmps(ph, sr, a1, a2, a3);
    std::printf("  sine/cosine morph 0.5 (None): a1 %.3f\n", a1);
    CHECK(std::fabs(a1 - 0.7071) < 0.05);
    ph.setMorphPhase(WavetableOscillator::MorphPhase::ZeroFundamental);
    harmonicAmps(ph, sr, a1, a2, a3);
    std::printf("  sine/cosine morph 0.5 (ZeroFundamental): a1 %.3f\n", a1);
    CHECK(a1 > 0.95);

    // ZeroAll: the morphed frame is zero-phase, so one rendered cycle
    // is an even function — sample[i] == sample[128-i].
    WavetableOscillator za;
    za.prepare(sr);
    za.setBank(&bank);
    za.setMorphMode(WavetableOscillator::MorphMode::Spectral);
    za.setMorphPhase(WavetableOscillator::MorphPhase::ZeroAll);
    za.setPosition(0.5f);
    za.setFrequency((float)(sr / 128.0));
    za.resetPhase();
    std::vector<float> cycle(128);
    for (int i = 0; i < 128; ++i)
        cycle[(size_t)i] = za.process();
    double maxAsym = 0.0;
    for (int i = 1; i < 128; ++i)
        maxAsym = std::fmax(maxAsym, std::fabs((double)cycle[(size_t)i]
                                               - (double)cycle[(size_t)(128 - i)]));
    std::printf("  ZeroAll cycle asymmetry: %.3g\n", maxAsym);
    CHECK(maxAsym < 1e-3);

    // The morphed frame's mip levels are real: morph to a pure saw
    // (position 1.0) at 997 Hz and the alias ratio must match the plain
    // saw mips (~-32 dB), far below the naive saw (~-15.6 dB).
    WavetableOscillator morphed;
    morphed.prepare(sr);
    morphed.setBank(&bank);
    morphed.setMorphMode(WavetableOscillator::MorphMode::Spectral);
    morphed.setPosition(1.0f);
    WavetableOscillator naive;
    naive.prepare(sr);
    WavetableBank naiveBank;
    CHECK(naiveBank.addFrame(saw.data(), (int)saw.size()));
    naive.setBank(&naiveBank);
    const double naiveDb = aliasRatioDb(naive, 997.0f, sr);
    const double morphDb = aliasRatioDb(morphed, 997.0f, sr);
    std::printf("  alias/harmonic ratio at 997 Hz, morphed saw: %.1f dB (naive %.1f dB)\n",
                morphDb, naiveDb);
    CHECK(morphDb < naiveDb - 10.0);

    // Fallback: a bank with frames but no mipmaps has no spectra, so
    // Spectral mode falls back to the per-sample crossfade — the output
    // is identical to Crossfade mode, sample for sample.
    WavetableBank noMipBank;
    CHECK(noMipBank.addFrame(sine.data(), (int)sine.size()));
    CHECK(noMipBank.addFrame(saw.data(), (int)saw.size()));
    WavetableOscillator fb1, fb2;
    fb1.prepare(sr);
    fb1.setBank(&noMipBank);
    fb1.setPosition(0.5f);
    fb1.setFrequency(220.0f);
    fb2.prepare(sr);
    fb2.setBank(&noMipBank);
    fb2.setPosition(0.5f);
    fb2.setFrequency(220.0f);
    fb2.setMorphMode(WavetableOscillator::MorphMode::Spectral);
    float maxDiff = 0.0f;
    for (int i = 0; i < 4096; ++i)
        maxDiff = std::fmax(maxDiff, std::fabs(fb1.process() - fb2.process()));
    CHECK(maxDiff == 0.0f);
}

// Synthesize a minimal WAV in memory for the import tests. `samples`
// is interleaved mono/stereo/etc. float in -1..1; `format` 1 = PCM,
// 3 = float; `bits` 16/24/32 (PCM) or 32 (float). `junk` inserts an
// unknown chunk between fmt and data, to prove it is skipped.
static std::vector<uint8_t> makeWav(const std::vector<float>& samples,
                                    int channels, int format, int bits,
                                    bool junk = false)
{
    const int bytesPerSample = (bits + 7) / 8;
    const int frameBytes = bytesPerSample * channels;
    const size_t dataBytes = samples.size() * (size_t)bytesPerSample;
    const size_t fmtSize = 16;
    const size_t junkSize = junk ? 8 + 4 : 0; // "JUNK" + 4 payload bytes
    const size_t riffSize = 4 + (8 + fmtSize) + junkSize + (8 + dataBytes);
    std::vector<uint8_t> wav;
    auto putTag = [&wav](const char* t) { wav.insert(wav.end(), t, t + 4); };
    auto putU16 = [&wav](uint16_t v)
    {
        wav.push_back((uint8_t)(v & 0xFF));
        wav.push_back((uint8_t)((v >> 8) & 0xFF));
    };
    auto putU32 = [&wav](uint32_t v)
    {
        wav.push_back((uint8_t)(v & 0xFF));
        wav.push_back((uint8_t)((v >> 8) & 0xFF));
        wav.push_back((uint8_t)((v >> 16) & 0xFF));
        wav.push_back((uint8_t)((v >> 24) & 0xFF));
    };
    putTag("RIFF");
    putU32((uint32_t)riffSize);
    putTag("WAVE");
    putTag("fmt ");
    putU32((uint32_t)fmtSize);
    putU16((uint16_t)format);
    putU16((uint16_t)channels);
    putU32(44100);
    putU32((uint32_t)(44100 * frameBytes));
    putU16((uint16_t)frameBytes);
    putU16((uint16_t)bits);
    if (junk)
    {
        putTag("JUNK");
        putU32(4);
        wav.insert(wav.end(), { 0xDE, 0xAD, 0xBE, 0xEF });
    }
    putTag("data");
    putU32((uint32_t)dataBytes);
    for (float s : samples)
    {
        if (format == 3)
        {
            uint32_t v;
            std::memcpy(&v, &s, 4);
            putU32(v);
        }
        else if (bits == 16)
        {
            const int16_t v = (int16_t)std::lround(s * 32767.0f);
            putU16((uint16_t)v);
        }
        else if (bits == 24)
        {
            int32_t v = (int32_t)std::lround(s * 8388607.0f);
            wav.push_back((uint8_t)(v & 0xFF));
            wav.push_back((uint8_t)((v >> 8) & 0xFF));
            wav.push_back((uint8_t)((v >> 16) & 0xFF));
        }
        else // 32-bit PCM
        {
            const int32_t v = (int32_t)std::lround(s * 2147483647.0f);
            putU32((uint32_t)v);
        }
    }
    return wav;
}

static std::vector<float> sineCycle(int length)
{
    std::vector<float> cycle((size_t)length);
    for (int i = 0; i < length; ++i)
        cycle[(size_t)i] = std::sin(6.2831853071795864769f * (float)i / (float)length);
    return cycle;
}

static void testWavImport()
{
    std::printf("testWavImport...\n");
    const auto sine2048 = sineCycle(2048);

    // Mono 16-bit PCM, one frame: values match the sine to 16-bit
    // quantization (1/32768).
    {
        auto wav = makeWav(sine2048, 1, 1, 16);
        WavetableBank bank;
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.error == nullptr);
        CHECK(r.framesAdded == 1 && r.framesDropped == 0 && r.totalSamples == 2048);
        CHECK(bank.getFrameCount() == 1);
        const auto& frame = bank.getFrame(0);
        CHECK(frame.size() == 2048);
        float maxErr = 0.0f;
        for (int i = 0; i < 2048; ++i)
            maxErr = std::fmax(maxErr, std::fabs(frame[(size_t)i] - sine2048[(size_t)i]));
        // Two error sources: the encoder scales by 32767 while the
        // decoder divides by 32768 (1/32768 systematic), plus rounding
        // (0.5 LSB). Worst case 1.5/32768 = 4.58e-5.
        CHECK(maxErr < 5e-5f);
    }

    // Two frames: sine then cosine (Serum-style, 2048-sample frames).
    {
        std::vector<float> two = sine2048;
        // Shift the sine by a quarter cycle to make a cosine.
        std::vector<float> cosFrame(2048);
        for (int i = 0; i < 2048; ++i)
            cosFrame[(size_t)i] = std::cos(6.2831853071795864769f * (float)i / 2048.0f);
        two.insert(two.end(), cosFrame.begin(), cosFrame.end());
        auto wav = makeWav(two, 1, 1, 16);
        WavetableBank bank;
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.framesAdded == 2 && bank.getFrameCount() == 2);
        CHECK(std::fabs(bank.getFrame(1)[0] - 1.0f) < 1e-4f); // cosine starts at 1
        CHECK(std::fabs(bank.getFrame(1)[512]) < 1e-4f);      // cosine crosses 0
    }

    // Stereo 16-bit: L = sine, R = 0 -> mono is half the sine.
    {
        std::vector<float> stereo;
        for (float s : sine2048)
        {
            stereo.push_back(s);
            stereo.push_back(0.0f);
        }
        auto wav = makeWav(stereo, 2, 1, 16);
        WavetableBank bank;
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.framesAdded == 1 && r.totalSamples == 2048);
        CHECK(std::fabs(bank.getFrame(0)[512] - 0.5f) < 1e-4f); // sine peak halved
    }

    // 24-bit PCM and 32-bit float: values match.
    {
        auto wav24 = makeWav(sine2048, 1, 1, 24);
        WavetableBank bank24;
        const auto r24 = importWav(bank24, wav24.data(), wav24.size());
        CHECK(r24.ok && r24.framesAdded == 1);
        float maxErr = 0.0f;
        for (int i = 0; i < 2048; ++i)
            maxErr = std::fmax(maxErr, std::fabs(bank24.getFrame(0)[(size_t)i]
                                                 - sine2048[(size_t)i]));
        CHECK(maxErr < 1e-6f);
        auto wavF = makeWav(sine2048, 1, 3, 32);
        WavetableBank bankF;
        const auto rF = importWav(bankF, wavF.data(), wavF.size());
        CHECK(rF.ok && rF.framesAdded == 1);
        CHECK(bankF.getFrame(0)[512] == 1.0f); // exact for float
    }

    // One short cycle (512 samples): resampled to 2048 as a single frame.
    {
        const auto short_ = sineCycle(512);
        auto wav = makeWav(short_, 1, 1, 16);
        WavetableBank bank;
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.framesAdded == 1 && r.totalSamples == 512);
        // Resampled to 2048: the sine peak lands at index 512.
        CHECK(std::fabs(bank.getFrame(0)[512] - 1.0f) < 1e-4f);
    }

    // Non-multiple length: 5000 samples -> 2 frames, 904 dropped.
    {
        std::vector<float> five(5000, 0.5f);
        auto wav = makeWav(five, 1, 1, 16);
        WavetableBank bank;
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.framesAdded == 2 && r.framesDropped == 1);
        CHECK(r.samplesDropped == 904 && r.totalSamples == 5000);
    }

    // Unknown chunk between fmt and data is skipped.
    {
        auto wav = makeWav(sine2048, 1, 1, 16, true);
        WavetableBank bank;
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.framesAdded == 1);
    }

    // Bank capacity: 255 frames already there, import 2 -> 1 added, 1 skipped.
    {
        WavetableBank bank;
        for (int i = 0; i < 255; ++i)
            CHECK(bank.addFrame(sine2048.data(), 2048));
        std::vector<float> two = sine2048;
        two.insert(two.end(), sine2048.begin(), sine2048.end());
        auto wav = makeWav(two, 1, 1, 16);
        const auto r = importWav(bank, wav.data(), wav.size());
        CHECK(r.ok && r.framesAdded == 1 && r.framesSkipped == 1);
        CHECK(bank.getFrameCount() == 256);
    }

    // Malformed files: not a WAV, truncated, unsupported format, empty.
    {
        WavetableBank bank;
        const uint8_t notWav[12] = { 'N', 'O', 'P', 'E', 0, 0, 0, 0, 0, 0, 0, 0 };
        CHECK(!importWav(bank, notWav, sizeof(notWav)).ok);
        auto wav = makeWav(sine2048, 1, 1, 16);
        CHECK(!importWav(bank, wav.data(), 20).ok); // truncated mid-fmt
        auto wav6 = makeWav(sine2048, 1, 6, 16);    // format 6 (a-law)
        CHECK(!importWav(bank, wav6.data(), wav6.size()).ok);
        CHECK(!importWav(bank, nullptr, 0).ok);
        CHECK(bank.getFrameCount() == 0); // nothing added by the failures
    }
}

// ---------------------------------------------------------------------------
// Simper SVF (paper section 4.5, rank 3).
//
// The measurement helper drives a filter with a sine at testFreq and
// returns the steady-state gain in dB (RMS ratio, second half of a
// 256-cycle run — the FreeEQ8 rig's method). Every number asserted
// below was first measured with a scratch probe and then encoded
// here; nothing is asserted from the paper alone.
// ---------------------------------------------------------------------------

static double measureSineGainSvf(SvfFilter& f, double testFreq, double sr)
{
    const double w = 2.0 * 3.14159265358979323846 * testFreq;
    int spc = (int)(sr / testFreq);
    if (spc < 4) spc = 4;
    const int total = spc * 256, warmup = total / 2;
    double si = 0.0, so = 0.0;
    for (int i = 0; i < total; ++i)
    {
        const double x = std::sin(w * i / sr);
        const double y = f.process((float)x);
        if (i >= warmup) { si += x * x; so += y * y; }
    }
    return si > 1e-30 ? 10.0 * std::log10(so / si) : -999.0;
}

static double measureSineGainRbj(RbjBiquad& b, double testFreq, double sr)
{
    const double w = 2.0 * 3.14159265358979323846 * testFreq;
    int spc = (int)(sr / testFreq);
    if (spc < 4) spc = 4;
    const int total = spc * 256, warmup = total / 2;
    double si = 0.0, so = 0.0;
    for (int i = 0; i < total; ++i)
    {
        const double x = std::sin(w * i / sr);
        const double y = b.process(x);
        if (i >= warmup) { si += x * x; so += y * y; }
    }
    return si > 1e-30 ? 10.0 * std::log10(so / si) : -999.0;
}

// Generic version: any filter with float process(float).
template <typename Filter>
static double measureSineGain(Filter& f, double testFreq, double sr)
{
    const double w = 2.0 * 3.14159265358979323846 * testFreq;
    int spc = (int)(sr / testFreq);
    if (spc < 4) spc = 4;
    const int total = spc * 256, warmup = total / 2;
    double si = 0.0, so = 0.0;
    for (int i = 0; i < total; ++i)
    {
        const double x = std::sin(w * i / sr);
        const double y = f.process((float)x);
        if (i >= warmup) { si += x * x; so += y * y; }
    }
    return si > 1e-30 ? 10.0 * std::log10(so / si) : -999.0;
}

static void testSvfResponse()
{
    std::printf("testSvfResponse...\n");
    const double sr = 44100.0;
    SvfFilter f;
    f.prepare(sr);
    f.setQ(0.7071f);
    f.setCutoff(1000.0f);

    // Lowpass, Q = 1/sqrt(2) (Butterworth): -3.01 dB at fc, ~0 dB a
    // decade below, -43.3 dB a decade above (12 dB/oct, two poles).
    f.setType(SvfFilter::Type::LowPass);
    f.reset();
    CHECK_NEAR(measureSineGainSvf(f, 100.0, sr), 0.0, 0.05);
    CHECK_NEAR(measureSineGainSvf(f, 1000.0, sr), -3.019, 0.05);
    CHECK_NEAR(measureSineGainSvf(f, 10000.0, sr), -43.316, 0.5);

    // Highpass mirrors it.
    f.setType(SvfFilter::Type::HighPass);
    f.reset();
    CHECK_NEAR(measureSineGainSvf(f, 100.0, sr), -40.030, 0.5);
    CHECK_NEAR(measureSineGainSvf(f, 1000.0, sr), -3.019, 0.05);
    CHECK_NEAR(measureSineGainSvf(f, 10000.0, sr), 0.0, 0.05);

    // Notch: a null at fc (measured -154 dB), ~0 dB a decade away.
    f.setType(SvfFilter::Type::Notch);
    f.reset();
    CHECK(measureSineGainSvf(f, 1000.0, sr) < -100.0);
    CHECK_NEAR(measureSineGainSvf(f, 100.0, sr), -0.087, 0.1);
    CHECK_NEAR(measureSineGainSvf(f, 10000.0, sr), -0.059, 0.1);

    // Bandpass peak gain is Q (constant-skirt-gain form), measured
    // at fc for four Q values: 20*log10(Q) to within 0.01 dB.
    f.setType(SvfFilter::Type::BandPass);
    const double qs[] = { 0.7071, 1.0, 2.0, 10.0 };
    for (double q : qs)
    {
        f.setQ((float)q);
        f.reset();
        CHECK_NEAR(measureSineGainSvf(f, 1000.0, sr), 20.0 * std::log10(q), 0.05);
    }

    // Resonant lowpass peaks at fc as Q rises (Q=1: ~0 dB, Q=2:
    // +6.01 dB, Q=10: +19.99 dB) — the resonance is real, not a
    // numerical artefact.
    f.setType(SvfFilter::Type::LowPass);
    const double lpQs[] = { 1.0, 2.0, 10.0 };
    const double lpPeaks[] = { -0.008, 6.012, 19.992 };
    for (int i = 0; i < 3; ++i)
    {
        f.setQ((float)lpQs[i]);
        f.reset();
        CHECK_NEAR(measureSineGainSvf(f, 1000.0, sr), lpPeaks[i], 0.1);
    }
}

static void testSvfMatchesRbj()
{
    std::printf("testSvfMatchesRbj...\n");
    // The FreeEQ8 result, reproduced: the Simper SVF and the RBJ
    // biquad produce identical steady-state magnitude responses.
    // Cramping is a BLT property, not a topology property. If this
    // check ever fails, the SVF implementation is wrong.
    const double sr = 44100.0;
    const double fcs[] = { 100.0, 1000.0, 8000.0, 16000.0 };
    const double qs[] = { 0.5, 0.7071, 1.0, 5.0 };
    const double testFreqs[] = { 20.0, 100.0, 500.0, 1000.0, 4000.0,
                                 8000.0, 12000.0, 16000.0, 20000.0 };
    const SvfFilter::Type types[] = { SvfFilter::Type::LowPass,
                                      SvfFilter::Type::HighPass,
                                      SvfFilter::Type::BandPass,
                                      SvfFilter::Type::Notch };
    const RbjBiquad::Type rbjTypes[] = { RbjBiquad::Type::LowPass,
                                         RbjBiquad::Type::HighPass,
                                         RbjBiquad::Type::BandPass,
                                         RbjBiquad::Type::Notch };
    for (int ti = 0; ti < 4; ++ti)
    {
        for (double fc : fcs)
        {
            for (double q : qs)
            {
                SvfFilter svf;
                svf.prepare(sr);
                svf.setType(types[ti]);
                svf.setCutoff((float)fc);
                svf.setQ((float)q);
                RbjBiquad rbj;
                rbj.set(rbjTypes[ti], sr, fc, q);
                for (double tf : testFreqs)
                {
                    if (tf >= sr * 0.5) continue;
                    svf.reset();
                    rbj.reset();
                    const double s = measureSineGainSvf(svf, tf, sr);
                    const double r = measureSineGainRbj(rbj, tf, sr);
                    // Both are -inf dB at a notch null; compare only
                    // where at least one of them is measurable.
                    if (s < -120.0 && r < -120.0) continue;
                    CHECK_NEAR(s, r, 0.05);
                }
            }
        }
    }
}

static void testSvfModulationStability()
{
    std::printf("testSvfModulationStability...\n");
    // Wishnick 2014 proves the trapezoidal SVF is time-varying BIBO
    // stable under per-sample coefficient change. Test it empirically:
    // audio-rate cutoff and Q modulation must stay finite and bounded
    // — no blow-up, no NaN, no runaway resonance.
    const double sr = 44100.0;
    SvfFilter f;
    f.prepare(sr);
    f.setType(SvfFilter::Type::LowPass);
    float peak = 0.0f;
    for (int i = 0; i < 44100; ++i)
    {
        // Cutoff sweeps 100 Hz .. 18 kHz at 100 Hz; Q sweeps 0.5..10.
        const double t = (double)i / sr;
        const float fc = (float)(100.0 + 17900.0 * (0.5 + 0.5 * std::sin(2.0 * 3.14159265358979323846 * 100.0 * t)));
        const float q = (float)(0.5 + 9.5 * (0.5 + 0.5 * std::sin(2.0 * 3.14159265358979323846 * 7.0 * t)));
        f.setCutoff(fc);
        f.setQ(q);
        const float x = (float)std::sin(2.0 * 3.14159265358979323846 * 440.0 * t);
        const float y = f.process(x);
        CHECK(std::isfinite(y));
        if (std::fabs(y) > peak) peak = std::fabs(y);
    }
    // A resonant peak can reach +20 dB (Q=10); 32x is far above any
    // legitimate peak and far below a blow-up (which would be 1e30+).
    CHECK(peak < 32.0f);
    CHECK(peak > 0.5f); // and the filter did actually pass signal
}

static void testSvfClampAndDrive()
{
    std::printf("testSvfClampAndDrive...\n");
    const double sr = 44100.0;
    SvfFilter f;
    f.prepare(sr);
    f.setType(SvfFilter::Type::LowPass);
    f.setQ(0.7071f);

    // Cutoff clamping (step-1 rule): 20 Hz .. min(20 kHz, Nyquist),
    // non-finite ignored (previous value kept).
    f.setCutoff(0.0f);
    CHECK_NEAR(f.getCutoff(), 20.0f, 1e-6f);
    f.setCutoff(-100.0f);
    CHECK_NEAR(f.getCutoff(), 20.0f, 1e-6f);
    f.setCutoff(30000.0f);
    CHECK_NEAR(f.getCutoff(), 20000.0f, 1e-6f);
    f.setCutoff(1e9f);
    CHECK_NEAR(f.getCutoff(), 20000.0f, 1e-6f);
    f.setCutoff(std::nanf(""));
    CHECK_NEAR(f.getCutoff(), 20000.0f, 1e-6f); // unchanged
    f.setCutoff(1000.0f);

    // Q clamping: 0.05 .. 100, non-finite ignored.
    f.setQ(0.0f);
    CHECK_NEAR(f.getQ(), 0.05f, 1e-6f);
    f.setQ(-5.0f);
    CHECK_NEAR(f.getQ(), 0.05f, 1e-6f);
    f.setQ(1e9f);
    CHECK_NEAR(f.getQ(), 100.0f, 1e-6f);
    f.setQ(std::nanf(""));
    CHECK_NEAR(f.getQ(), 100.0f, 1e-6f); // unchanged
    f.setQ(0.7071f);

    // None of the bad values may produce a non-finite sample.
    const float badCutoffs[] = { 0.0f, -100.0f, 30000.0f, 1e9f, std::nanf("") };
    for (float c : badCutoffs)
    {
        f.setCutoff(c);
        for (int i = 0; i < 100; ++i)
            CHECK(std::isfinite(f.process(0.5f)));
    }

    // Drive: 0 is the bypass — byte-identical output to a filter
    // that never had drive set. Drive 1 saturates: a full-scale
    // sine stays bounded well below full scale (tanh(8x)/8 <= 0.125).
    SvfFilter a, b;
    a.prepare(sr); b.prepare(sr);
    a.setType(SvfFilter::Type::LowPass); b.setType(SvfFilter::Type::LowPass);
    a.setCutoff(18000.0f); b.setCutoff(18000.0f);
    a.setQ(0.7071f); b.setQ(0.7071f);
    a.setDrive(0.0f);
    for (int i = 0; i < 1000; ++i)
    {
        const float x = (float)std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / sr);
        CHECK_NEAR(a.process(x), b.process(x), 1e-7f);
    }
    b.setDrive(1.0f);
    float drivePeak = 0.0f;
    for (int i = 0; i < 4410; ++i)
    {
        const float x = (float)std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / sr);
        const float y = b.process(x);
        CHECK(std::isfinite(y));
        if (std::fabs(y) > drivePeak) drivePeak = std::fabs(y);
    }
    CHECK(drivePeak < 0.2f);
    CHECK(drivePeak > 0.01f); // and the filter did actually pass signal
}

static void testDualFilter()
{
    std::printf("testDualFilter...\n");
    const double sr = 44100.0;

    // Serial LP + LP at 1 kHz rolls off far steeper than one LP:
    // at 10 kHz a single LP is -43.3 dB, the serial pair must be
    // at least 20 dB deeper.
    DualFilter d;
    d.prepare(sr);
    d.setTypeA(SvfFilter::Type::LowPass);
    d.setTypeB(SvfFilter::Type::LowPass);
    d.setCutoffA(1000.0f);
    d.setCutoffB(1000.0f);
    d.setQA(0.7071f);
    d.setQB(0.7071f);
    d.setRouting(DualFilter::Routing::Serial);
    SvfFilter single;
    single.prepare(sr);
    single.setType(SvfFilter::Type::LowPass);
    single.setCutoff(1000.0f);
    single.setQ(0.7071f);
    d.reset();
    single.reset();
    const double serialDb = measureSineGain(d, 10000.0, sr);
    const double singleDb = measureSineGain(single, 10000.0, sr);
    CHECK_NEAR(singleDb, -43.316, 0.5);
    CHECK(serialDb < singleDb - 20.0);

    // Parallel: two identical LPs in parallel must equal one LP
    // (the average of two identical responses is the response).
    d.setRouting(DualFilter::Routing::Parallel);
    d.reset();
    single.reset();
    CHECK_NEAR(measureSineGain(d, 1000.0, sr), measureSineGain(single, 1000.0, sr), 0.05);

    // Parallel LP + HP at the same cutoff: at fc each is -3 dB and
    // in opposite phase, so their average nulls (the classic
    // parallel LP+HP notch). It must be deep and finite.
    d.setTypeA(SvfFilter::Type::LowPass);
    d.setTypeB(SvfFilter::Type::HighPass);
    d.setRouting(DualFilter::Routing::Parallel);
    d.reset();
    const double nullDb = measureSineGain(d, 1000.0, sr);
    CHECK(nullDb < -40.0);
    // And away from fc it passes signal: a decade below fc the LP
    // is ~0 dB and the HP is ~-40 dB, so their average is ~-6 dB.
    d.reset();
    CHECK_NEAR(measureSineGain(d, 100.0, sr), -6.0, 0.5);
}

int main()
{
    std::printf("Therum DSP core tests (framework-free, no DAW)\n");
    std::printf("==============================================\n");

    testFft();
    testWavetableEngine();
    testMipmaps();
    testSpectralMorph();
    testWavImport();
    testReleaseTail();
    testVoiceStealingRoundRobin();
    testDuplicateNoteRetrigger();
    testPhaseReset();
    testCutoffClamp();
    testSmoothing();
    testLimiter();
    testSvfResponse();
    testSvfMatchesRbj();
    testSvfModulationStability();
    testSvfClampAndDrive();
    testDualFilter();

    std::printf("==============================================\n");
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    if (g_failures == 0)
        std::printf("ALL TESTS PASSED\n");
    return g_failures == 0 ? 0 : 1;
}
