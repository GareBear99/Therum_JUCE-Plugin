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

int main()
{
    std::printf("Therum DSP core tests (framework-free, no DAW)\n");
    std::printf("==============================================\n");

    testFft();
    testWavetableEngine();
    testMipmaps();
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
