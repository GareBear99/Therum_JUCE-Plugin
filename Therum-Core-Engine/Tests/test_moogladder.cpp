// test_moogladder — the Huovilainen 2004 Moog ladder filter test.
//
// Paper sections 4.5 ("Moog ladder — the practical choice") and 7
// ("Filter response test"): the ladder is measured against the
// analogue prototype — the continuous-time ladder ODEs' transfer
// function H(s) = 1 / ((1 + s/wc)^4 + k) — on the same sine rig
// the decramping test uses (steady-state RMS ratio), at the
// oversampled rate (the core, no interpolator) and at the host
// rate (the full path, interpolator included). The two are
// reported separately so the interpolator's droop and image
// leakage are not conflated with the discretisation error.
//
// The prototype is NOT a Butterworth: four identical real poles
// at wc with feedback k give -12.04 dB at fc for k = 0 (the -3 dB
// point is at 0.644*fc), DC gain 1/(1+k), and a resonance peak
// whose frequency and gain are computed numerically from the
// prototype on the same grid as the measurement.
//
// The implementation is checked against the Euler-discretised
// prototype — the analogue prototype evaluated at s =
// fs_os*(e^{jw} - 1), the exact response of the forward-Euler
// discretisation — which it matches to ~0.03 dB (worst 0.37 dB
// at fc = 16 kHz, f = 32 kHz, harmonic power in the RMS). The
// gap between the Euler prediction and the analogue prototype is
// the discretisation error, and it is large: s has a real part
// (fs_os*(cos w - 1) < 0), so the error is not just a frequency
// warp — the evaluation point leaves the jw axis. Measured:
// +0.60 dB at f = fc for fc = 1 kHz, +5.93 dB for fc = 8 kHz,
// +14.33 dB for fc = 16 kHz (the core reads +2.29 dB where the
// prototype says -12.04 dB). Once alpha = 2*pi*fc/fs_os exceeds
// 1 (fc ~= 14 kHz at 44.1 kHz) the stopband rises above 0 dB —
// the ladder stops being a lowpass up there. 2x is the published
// Huovilainen figure and the spec for this piece; 4x (DaisySP's
// choice) moves the alpha = 1 threshold above Nyquist and is the
// documented alternative, for Gary's call — the test measures
// the 2x behaviour, it does not silently switch to 4x.
//
// Sections:
//   (a) Parameter handling (clamping, non-finite ignored).
//   (b) DC gain: 1/(1+k) — 0 dB, -9.54 dB, -13.98 dB.
//   (c) Core response vs prototype, fc = 100 / 1000 / 8000 /
//       16000 Hz, small signal (0.1) — the headline table. The
//       low-fc rows are the "decramping at lower fc" measurement:
//       the Euler error concentrates at high fc and high f, and
//       is negligible at low fc.
//   (d) Rolloff: -24 dB/octave asymptotically (4-pole).
//   (e) Resonance peak vs prototype (reso = 0.5, fc = 1000 Hz).
//   (f) Self-oscillation (reso = 1.0): sustained, bounded,
//       at ~fc (Euler warping droops the pitch at high fc —
//       measured at fc = 1000 Hz and reported at 16 kHz).
//   (g) Stability sweep: fc x reso, sine + impulse, finite and
//       bounded everywhere at 2x.
//   (h) Alias comparison 1x vs 2x: folded tanh harmonics of a
//       5 kHz sine through fc = 8 kHz, reso = 0.
//   (i) Full-path response vs prototype (interpolator included).
//   (j) 1x at fc = 16 kHz (report-only: documents that 1x is
//       unstable at high fc — alpha = 2*pi*fc/fs exceeds 2).
//   (k) CPU: median ns/sample over 16 trials, 2x (paper section
//       7 method, simplified).
//
// Build and run: `make moog` (see Makefile), or:
//   g++ -std=c++17 -O2 -Wall -Wextra -o test_moogladder \
//       test_moogladder.cpp \
//       ../Source/DSP/Filters/MoogLadderFilter.cpp
//   ./test_moogladder
// Exit code 0 means every check passed; non-zero means a check
// failed (failures are printed with file and line).

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "../Source/DSP/Filters/MoogLadderFilter.h"

static constexpr double kSr = 44100.0;
static constexpr double kSrOs = 88200.0; // 2x oversampled rate
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

// ---------------------------------------------------------------------------
// Analogue prototype: the ladder ODEs' transfer function.
// H(s) = 1 / ((1 + s/wc)^4 + k), wc = 2*pi*fc. k = 0: four identical
// real poles; k > 0: the feedback moves two poles toward the
// imaginary axis (self-oscillation at k = 4, poles at +-j*wc).
// ---------------------------------------------------------------------------
static double analogLadderGainDb(double f, double fc, double k)
{
    const double w = f / fc; // normalised frequency
    // (1 + jw)^2 = (1 - w^2) + j*2w; square again.
    const double r2 = 1.0 - w * w, i2 = 2.0 * w;
    const double r4 = r2 * r2 - i2 * i2 + k;
    const double i4 = 2.0 * r2 * i2;
    return -20.0 * std::log10(std::sqrt(r4 * r4 + i4 * i4));
}

// Euler-discretised prototype: the analogue prototype evaluated
// at s = fs_os * (e^{jw} - 1), w = 2*pi*f/fs_os — the exact
// frequency response of the forward-Euler discretisation of the
// ODEs (the mapping is exact for the simultaneous Euler update
// this filter uses). The implementation must match THIS to
// measurement noise; the gap between this and the analogue
// prototype is the discretisation error (the physics, not a bug).
// s has a real part (fs_os*(cos w - 1) < 0), so the error is NOT
// just a frequency warp: the evaluation point leaves the jw axis,
// which is why the error at f = fc is +0.6 dB at fc = 1 kHz and
// +14 dB at fc = 16 kHz, not the ~0.03 dB a magnitude-only warp
// estimate would give.
static double eulerLadderGainDb(double f, double fc, double k)
{
    const double w = kPi2 * f / kSrOs;
    const double wc = kPi2 * fc;
    // s = fs_os * (e^{jw} - 1)
    const double sr_ = kSrOs * (std::cos(w) - 1.0);
    const double si = kSrOs * std::sin(w);
    // a = 1 + s/wc (complex); a^4 + k; gain = -20*log10|a^4 + k|.
    const double ar = 1.0 + sr_ / wc, ai = si / wc;
    const double r2 = ar * ar - ai * ai, i2 = 2.0 * ar * ai;
    const double r4 = r2 * r2 - i2 * i2 + k;
    const double i4 = 2.0 * r2 * i2;
    return -20.0 * std::log10(std::sqrt(r4 * r4 + i4 * i4));
}

// ---------------------------------------------------------------------------
// Sine measurement (the FreeEQ8 rig's method): steady-state RMS
// ratio, small signal (amp 0.1) so the tanh stays linear and the
// measurement compares against the linear prototype.
// The core version drives processOversampled directly at 88.2 kHz
// (no interpolator); the full-path version drives process() at
// the host rate (interpolator included).
//
// Warmup: the default (total/2) is enough except where the
// steady-state response is deeper than ~-120 dB (fc = 100 Hz at
// Nyquist: -187 dB) — there the onset transient dominates the
// short measurement (it reads -63.7 dB with the default warmup,
// -183.7 dB with 8x warmup, matching the Euler prediction
// exactly). The Nyquist rows therefore use warmupMult = 8.
// ---------------------------------------------------------------------------
static double measureGainDbCore(MoogLadderFilter& f, double testFreq, double amp = 0.1,
                                int warmupMult = 1)
{
    const double w = kPi2 * testFreq;
    int spc = (int)(kSrOs / testFreq);
    if (spc < 4) spc = 4;
    const int total = spc * 256 * warmupMult, warmup = total / 2;
    double si = 0.0, so = 0.0;
    for (int i = 0; i < total; ++i)
    {
        const double x = amp * std::sin(w * i / kSrOs);
        const double y = (double)f.processOversampled((float)x);
        if (i >= warmup) { si += x * x; so += y * y; }
    }
    return si > 1e-30 ? 10.0 * std::log10(so / si) : -999.0;
}

static double measureGainDbFull(MoogLadderFilter& f, double testFreq, double amp = 0.1)
{
    const double w = kPi2 * testFreq;
    int spc = (int)(kSr / testFreq);
    if (spc < 4) spc = 4;
    const int total = spc * 256, warmup = total / 2;
    double si = 0.0, so = 0.0;
    for (int i = 0; i < total; ++i)
    {
        const double x = amp * std::sin(w * i / kSr);
        const double y = (double)f.process((float)x);
        if (i >= warmup) { si += x * x; so += y * y; }
    }
    return si > 1e-30 ? 10.0 * std::log10(so / si) : -999.0;
}

// Goertzel magnitude at one frequency (rectangular window; the
// frequencies used are exact bins of the analysis window, so
// there is no leakage). Returns the sine amplitude.
static double goertzelAmp(const std::vector<float>& x, double sr, double freq)
{
    const int n = (int)x.size();
    const double k = std::round(n * freq / sr);
    const double w = kPi2 * k / n;
    const double coeff = 2.0 * std::cos(w);
    double s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double s0 = (double)x[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const double re = s1 - s2 * std::cos(w);
    const double im = s2 * std::sin(w);
    return 2.0 * std::sqrt(re * re + im * im) / n;
}

static void setupLadder(MoogLadderFilter& f, double fc, double reso, int os = 2)
{
    f.prepare(kSr);
    f.setOversampling(os);
    f.setCutoff((float)fc);
    f.setResonance((float)reso);
    f.reset();
}

int main()
{
    std::printf("Therum Moog ladder test (paper section 4.5, Huovilainen 2004 at 2x)\n");
    std::printf("=====================================================================\n");

    // --- (a) Parameter handling. ---
    std::printf("\n-- Parameter handling --\n");
    {
        MoogLadderFilter f;
        f.prepare(kSr);
        CHECK(f.getOversampling() == 2); // 2x is the default
        CHECK(std::fabs(f.getOversampledRate() - 88200.0) < 1.0);
        f.setCutoff(1000.0f);
        f.setResonance(0.5f);
        CHECK_NEAR(f.getCutoff(), 1000.0, 1e-6);
        CHECK_NEAR(f.getResonance(), 0.5, 1e-6);
        // Clamping, same rules as the SVF.
        f.setCutoff(1.0f);
        CHECK_NEAR(f.getCutoff(), 20.0, 1e-6);
        f.setCutoff(100000.0f);
        CHECK_NEAR(f.getCutoff(), 20000.0, 1e-6);
        f.setResonance(-1.0f);
        CHECK_NEAR(f.getResonance(), 0.0, 1e-6);
        f.setResonance(2.0f);
        CHECK_NEAR(f.getResonance(), 1.0, 1e-6);
        // Non-finite values are ignored (previous value kept).
        f.setCutoff(1000.0f);
        f.setCutoff(std::nan(""));
        CHECK_NEAR(f.getCutoff(), 1000.0, 1e-6);
        f.setResonance(0.25f);
        f.setResonance(std::numeric_limits<float>::infinity());
        CHECK_NEAR(f.getResonance(), 0.25, 1e-6);
        // Oversampling: only 1 and 2; anything else clamps.
        f.setOversampling(1);
        CHECK(f.getOversampling() == 1);
        f.setOversampling(8);
        CHECK(f.getOversampling() == 2);
        f.setOversampling(0);
        CHECK(f.getOversampling() == 1);
        // reset() clears the state: silence in, silence out.
        f.prepare(kSr);
        f.setCutoff(1000.0f);
        f.setResonance(0.0f);
        f.process(1.0f);
        f.reset();
        CHECK_NEAR(f.process(0.0f), 0.0, 1e-12);
    }

    // --- (b) DC gain: 1/(1+k). ---
    // Small constant input (0.1, linear region), 2 s, mean of the
    // last 0.5 s. The Euler discretisation is exact at DC — but
    // only where the fixed point is stable. At reso = 1.0 (k = 4)
    // the fixed point is unstable (the discrete poles sit at
    // 1 +- j*alpha, |z| > 1) and the filter self-oscillates, so
    // the DC level shifts by nonlinear rectification around the
    // limit cycle (measured -14.486 dB vs the -13.979 dB fixed
    // point). That row is reported, not asserted: "DC gain" is
    // ill-defined for a self-oscillating filter.
    std::printf("\n-- DC gain (1/(1+k)) --\n");
    {
        const double cases[][2] = { { 0.0, 0.0 }, { 0.5, -9.542 }, { 1.0, -13.979 } };
        for (const auto& c : cases)
        {
            MoogLadderFilter f;
            setupLadder(f, 1000.0, c[0]);
            double sum = 0.0;
            const int n = (int)kSr * 2;
            for (int i = 0; i < n; ++i)
            {
                const float y = f.process(0.1f);
                if (i >= (int)(kSr * 1.5)) sum += y;
            }
            const double mean = sum / (kSr * 0.5);
            const double gainDb = 20.0 * std::log10(mean / 0.1);
            std::printf("  reso %.1f (k=%.0f): DC gain %+.3f dB (prototype %+.3f dB)%s\n",
                        c[0], 4.0 * c[0], gainDb, c[1],
                        c[0] >= 1.0 ? "  (report-only: self-oscillating, see note)" : "");
            if (c[0] < 1.0)
                CHECK_NEAR(gainDb, c[1], 0.05);
        }
    }

    // --- (c) Core response vs prototype (the headline table). ---
    // Small signal (0.1) at the oversampled rate, no interpolator.
    // Two comparisons per row: against the Euler-discretised
    // prototype (the implementation check — it must match to
    // measurement noise, tolerance 0.5 dB; the worst deviation is
    // 0.37 dB at fc = 16 kHz, f = 32 kHz, which is harmonic power
    // in the RMS from the tanh at 0.1 amplitude, not a
    // discretisation error) and against the analogue prototype
    // (the finding — the Euler error). The low-fc rows are the
    // "decramping at lower fc" measurement: the Euler error scales
    // with f/fs_os and with fc, so at fc = 100 Hz it is <= 0.09 dB
    // everywhere in band, while at fc = 16 kHz it is +14 dB at fc
    // and +35 dB at 2fc — the same pattern as the SVF/RBJ cramping
    // (worst at high fc), opposite sign (brighter, not darker),
    // and it crosses 0 dB in the stopband once alpha =
    // 2*pi*fc/fs_os exceeds 1 (fc ~= 14 kHz at 44.1 kHz): the
    // ladder stops being a lowpass up there. The analogue-error
    // check is therefore asserted only where the ladder tracks
    // the prototype (fc <= 1 kHz, f <= 2fc, < 1.5 dB); above that
    // the table IS the result.
    std::printf("\n-- Core response vs analogue prototype (2x, small signal) --\n");
    std::printf("  %-8s %-8s %10s %10s %10s %10s\n", "fc", "f", "proto dB", "meas dB", "err dB", "errEuler dB");
    {
        const double fcs[] = { 100.0, 1000.0, 8000.0, 16000.0 };
        const double ratios[] = { 0.1, 0.5, 1.0, 2.0, 4.0 };
        for (double fc : fcs)
        {
            MoogLadderFilter f;
            setupLadder(f, fc, 0.0);
            for (double r : ratios)
            {
                const double tf = fc * r;
                if (tf >= 40000.0) continue; // above the OS Nyquist
                f.reset();
                const double meas = measureGainDbCore(f, tf);
                const double proto = analogLadderGainDb(tf, fc, 0.0);
                const double euler = eulerLadderGainDb(tf, fc, 0.0);
                const double err = meas - proto;
                std::printf("  %-8.0f %-8.0f %+10.3f %+10.3f %+10.3f %+10.3f\n",
                            fc, tf, proto, meas, err, meas - euler);
                CHECK(std::fabs(meas - euler) < 0.5); // the implementation IS Euler
                if (fc <= 1000.0 && r <= 2.0)
                    CHECK(std::fabs(err) < 1.5); // tracks the prototype here
            }
            // Nyquist row (host Nyquist = fs_os/4). Long warmup:
            // the steady state is -100 dB down or deeper, and the
            // onset transient dominates a short measurement.
            f.reset();
            const double measNyq = measureGainDbCore(f, kSr / 2.0, 0.1, 8);
            const double protoNyq = analogLadderGainDb(kSr / 2.0, fc, 0.0);
            const double eulerNyq = eulerLadderGainDb(kSr / 2.0, fc, 0.0);
            std::printf("  %-8.0f %-8.0f %+10.3f %+10.3f %+10.3f %+10.3f  (host Nyquist)\n",
                        fc, kSr / 2.0, protoNyq, measNyq, measNyq - protoNyq, measNyq - eulerNyq);
            CHECK(std::fabs(measNyq - eulerNyq) < 0.5);
        }
        std::printf("  (err = measured - analogue prototype; errEuler = measured - Euler prediction)\n");
    }

    // --- (d) Rolloff: -24 dB/octave asymptotically. ---
    // Prototype: |H(4fc)| = -49.22 dB, |H(8fc)| = -72.26 dB,
    // difference 23.04 dB per octave (asymptotically 24 dB/oct).
    // The absolute gains carry the Euler error (+1.29 dB at 4fc),
    // so the per-octave difference is checked against the Euler
    // prediction (22.90 dB), not the prototype — the rolloff
    // shape is the assertion, the level is section (c)'s table.
    std::printf("\n-- Rolloff (fc = 1000 Hz, reso = 0) --\n");
    {
        MoogLadderFilter f;
        setupLadder(f, 1000.0, 0.0);
        f.reset();
        const double g4 = measureGainDbCore(f, 4000.0);
        f.reset();
        const double g8 = measureGainDbCore(f, 8000.0);
        const double eulerPerOct = eulerLadderGainDb(4000.0, 1000.0, 0.0)
                                 - eulerLadderGainDb(8000.0, 1000.0, 0.0);
        std::printf("  gain at 4fc: %+.3f dB, at 8fc: %+.3f dB, per-octave: %.3f dB (Euler %.2f dB, proto 23.04 dB)\n",
                    g4, g8, g4 - g8, eulerPerOct);
        CHECK_NEAR(g4 - g8, eulerPerOct, 0.5);
    }

    // --- (e) Resonance peak vs prototype (reso = 0.5, fc = 1000 Hz). ---
    // Peak frequency and gain are computed from the prototype on
    // the same log grid as the measurement, so the comparison is
    // apples-to-apples (no analytic peak formula is assumed). The
    // peak frequency matches the analogue prototype exactly
    // (815 Hz); the peak gain is +1.32 dB above it (Euler
    // brightening), so the gain is checked against the Euler
    // prediction on the same grid, and the analogue gap is
    // reported.
    std::printf("\n-- Resonance peak (reso = 0.5, fc = 1000 Hz) --\n");
    {
        MoogLadderFilter f;
        setupLadder(f, 1000.0, 0.5);
        double measPeakGain = -1e9, measPeakFreq = 0.0;
        double protoPeakGain = -1e9, protoPeakFreq = 0.0;
        double eulerPeakGain = -1e9;
        for (int i = 0; i <= 60; ++i)
        {
            const double tf = 100.0 * std::pow(30.0, (double)i / 60.0); // 100 Hz .. 3 kHz
            f.reset();
            const double meas = measureGainDbCore(f, tf);
            const double proto = analogLadderGainDb(tf, 1000.0, 2.0);
            const double euler = eulerLadderGainDb(tf, 1000.0, 2.0);
            if (meas > measPeakGain) { measPeakGain = meas; measPeakFreq = tf; }
            if (proto > protoPeakGain) { protoPeakGain = proto; protoPeakFreq = tf; }
            if (euler > eulerPeakGain) eulerPeakGain = euler;
        }
        std::printf("  measured peak: %+.3f dB at %.0f Hz\n", measPeakGain, measPeakFreq);
        std::printf("  prototype peak: %+.3f dB at %.0f Hz\n", protoPeakGain, protoPeakFreq);
        std::printf("  Euler peak: %+.3f dB\n", eulerPeakGain);
        CHECK_NEAR(measPeakGain, eulerPeakGain, 0.5);
        CHECK(std::fabs(measPeakFreq - protoPeakFreq) / protoPeakFreq < 0.10);
    }

    // --- (f) Self-oscillation (reso = 1.0, k = 4). ---
    // Impulse, then zeros for 2 s. The oscillation must sustain
    // (RMS in the last 0.5 s well above zero), stay bounded
    // (tanh saturates — finite amplitude, no blow-up), and sit at
    // ~fc. At fc = 16 kHz the pitch droops (Euler warping: the
    // discrete pole angle is atan(alpha), not alpha) — reported,
    // not asserted.
    std::printf("\n-- Self-oscillation (reso = 1.0) --\n");
    {
        for (double fc : { 1000.0, 16000.0 })
        {
            MoogLadderFilter f;
            setupLadder(f, fc, 1.0);
            const int n = (int)kSr * 2;
            double maxAbs = 0.0, sumSq = 0.0;
            int crossings = 0;
            double prev = 0.0;
            bool first = true;
            for (int i = 0; i < n; ++i)
            {
                const float y = f.process(i == 0 ? 1.0f : 0.0f);
                if (!std::isfinite(y)) break;
                maxAbs = std::max(maxAbs, (double)std::fabs(y));
                if (i >= (int)(kSr * 1.5))
                {
                    sumSq += (double)y * y;
                    if (!first && prev < 0.0 && y >= 0.0) ++crossings;
                }
                prev = y;
                first = false;
            }
            const double rms = std::sqrt(sumSq / (kSr * 0.5));
            const double fOsc = (double)crossings / 0.5; // rising crossings per second
            std::printf("  fc %.0f Hz: RMS (last 0.5 s) %.3f, max|out| %.3f, f_osc %.0f Hz\n",
                        fc, rms, maxAbs, fOsc);
            CHECK(std::isfinite(maxAbs));
            CHECK(maxAbs < 2.0); // bounded by the tanh saturation
            if (fc == 1000.0)
            {
                CHECK(rms > 0.05); // sustained, not decaying away
                CHECK(std::fabs(fOsc - 1000.0) / 1000.0 < 0.10);
            }
        }
    }

    // --- (g) Stability sweep: fc x reso, sine + impulse. ---
    // 2x oversampling. Every combination must stay finite and
    // bounded. (1x is not swept here — it is unstable at high fc
    // by construction; see section (j).)
    std::printf("\n-- Stability sweep (2x, sine 440 Hz amp 0.5 + impulse) --\n");
    {
        const double fcs[] = { 20.0, 100.0, 1000.0, 8000.0, 16000.0, 20000.0 };
        const double resos[] = { 0.0, 0.5, 0.9, 1.0 };
        double worstMax = 0.0;
        for (double fc : fcs)
        {
            for (double reso : resos)
            {
                MoogLadderFilter f;
                setupLadder(f, fc, reso);
                double maxAbs = 0.0;
                bool finite = true;
                const int n = (int)kSr;
                for (int i = 0; i < n; ++i)
                {
                    const double x = 0.5 * std::sin(kPi2 * 440.0 * i / kSr)
                                   + (i == 0 ? 1.0 : 0.0);
                    const float y = f.process((float)x);
                    if (!std::isfinite(y)) { finite = false; break; }
                    maxAbs = std::max(maxAbs, (double)std::fabs(y));
                }
                worstMax = std::max(worstMax, maxAbs);
                CHECK(finite);
                CHECK(maxAbs < 4.0);
            }
        }
        std::printf("  24 combinations, worst max|out| %.3f (all finite, all < 4.0)\n", worstMax);
    }

    // --- (h) Alias comparison 1x vs 2x. ---
    // 5 kHz sine, amplitude 1.0 (the tanh saturates — that is the
    // drive), fc = 8 kHz, reso = 0. The tanh generates harmonics
    // n*5 kHz; those above Nyquist fold to 44.1k - n*5k. The
    // folded products at 19.1k / 14.1k / 9.1k / 4.1k / 0.9k are
    // measured by Goertzel on the last 0.75 s (exact bins, no
    // leakage) and summed as alias energy vs the fundamental.
    // At 2x the harmonics are generated at their true frequencies
    // and filtered by the ladder there before decimation folds
    // them; at 1x they are generated already aliased.
    std::printf("\n-- Alias comparison 1x vs 2x (5 kHz sine, amp 1.0, fc = 8 kHz) --\n");
    {
        const double folded[] = { 19100.0, 14100.0, 9100.0, 4100.0, 900.0 };
        double aliasDb[2] = { 0.0, 0.0 };
        for (int os = 1; os <= 2; ++os)
        {
            MoogLadderFilter f;
            setupLadder(f, 8000.0, 0.0, os);
            std::vector<float> out;
            out.reserve((int)kSr);
            for (int i = 0; i < (int)kSr; ++i)
                out.push_back(f.process((float)std::sin(kPi2 * 5000.0 * i / kSr)));
            // Last 0.75 s for the analysis (transient discarded).
            std::vector<float> tail(out.begin() + (int)(kSr * 0.25), out.end());
            const double fund = goertzelAmp(tail, kSr, 5000.0);
            double aliasEnergy = 0.0;
            std::printf("  %dx: fundamental %+.2f dBFS", os, 20.0 * std::log10(fund));
            for (double ff : folded)
            {
                const double a = goertzelAmp(tail, kSr, ff);
                aliasEnergy += a * a;
                std::printf(", %.0fk %+.1f dBFS", ff / 1000.0, 20.0 * std::log10(a));
            }
            aliasDb[os - 1] = 10.0 * std::log10(aliasEnergy / (fund * fund));
            std::printf("\n  %dx: total alias energy %+.2f dB below fundamental\n", os, aliasDb[os - 1]);
        }
        std::printf("  2x improvement: %.2f dB\n", aliasDb[0] - aliasDb[1]);
        CHECK(aliasDb[1] < aliasDb[0] - 10.0); // 2x must beat 1x by >= 10 dB
    }

    // --- (i) Full-path response vs prototype (interpolator included). ---
    // Same sine rig at the host rate through process(). The
    // interpolator's droop (cos^2(pi*f/(2*fs)): -3.0 dB at 16 kHz)
    // and the first-image leakage show up here; the core table in
    // (c) is the clean discretisation measurement, this is the
    // shipped path. At fc = 1 kHz the full path tracks the
    // prototype within 1 dB (asserted). At fc = 16 kHz it does
    // NOT — and the reason is measured, not hidden: alpha > 1
    // makes the ladder's stopband rise above 0 dB (the core reads
    // +2.29 dB at fc, +7.25 dB at 2fc), so the interpolator's
    // first image (28.1 kHz, only -10.7 dB down from linear
    // interpolation) is AMPLIFIED by the ladder (+6.4 dB) and
    // folds back onto the fundamental at decimation. The fc = 16 kHz
    // rows are therefore report-only: they document that the 2x +
    // linear-interpolation path is not a clean lowpass at high fc,
    // which is a finding for Gary (4x oversampling, DaisySP's
    // choice, moves the alpha = 1 threshold above Nyquist).
    std::printf("\n-- Full-path response vs prototype (process(), 2x, small signal) --\n");
    {
        for (double fc : { 1000.0, 16000.0 })
        {
            MoogLadderFilter f;
            setupLadder(f, fc, 0.0);
            for (double r : { 0.1, 0.5, 1.0, 2.0 })
            {
                const double tf = fc * r;
                if (tf >= 20000.0) continue;
                f.reset();
                const double meas = measureGainDbFull(f, tf);
                const double proto = analogLadderGainDb(tf, fc, 0.0);
                std::printf("  fc %-6.0f f %-6.0f: proto %+7.3f dB, meas %+7.3f dB, err %+7.3f dB%s\n",
                            fc, tf, proto, meas, meas - proto,
                            fc >= 16000.0 ? "  (report-only: image leakage, see note)" : "");
                if (fc <= 1000.0)
                    CHECK(std::fabs(meas - proto) < 2.0);
            }
        }
    }

    // --- (j) 1x at fc = 16 kHz (report-only). ---
    // alpha = 2*pi*16000/44100 = 2.28 > 2: the Euler update is
    // unstable even with no feedback (pole at -1 maps to
    // |1 - alpha| = 1.28 > 1). 1x is not a shipping mode; this
    // documents why, with a number.
    std::printf("\n-- 1x at fc = 16 kHz (report-only: 1x is unstable at high fc) --\n");
    {
        MoogLadderFilter f;
        setupLadder(f, 16000.0, 0.0, 1);
        double maxAbs = 0.0;
        bool finite = true;
        for (int i = 0; i < (int)(kSr * 0.1); ++i)
        {
            const float y = f.process((float)(0.5 * std::sin(kPi2 * 440.0 * i / kSr)));
            if (!std::isfinite(y)) { finite = false; break; }
            maxAbs = std::max(maxAbs, (double)std::fabs(y));
        }
        std::printf("  1x, fc 16 kHz, 0.1 s: finite = %s, max|out| %.3f\n",
                    finite ? "yes" : "NO", maxAbs);
    }

    // --- (k) CPU: median ns/sample over 16 trials, 2x. ---
    // Paper section 7 method, simplified: warmup discarded,
    // median of 16 trials of 1M samples. The number is hardware-
    // dependent (this VM); the check is a sanity bound only.
    std::printf("\n-- CPU (2x, process(), 1M samples/trial) --\n");
    {
        MoogLadderFilter f;
        setupLadder(f, 8000.0, 0.5);
        for (int i = 0; i < 100000; ++i) f.process(0.5f); // warmup
        std::vector<double> times;
        for (int t = 0; t < 16; ++t)
        {
            f.reset();
            const auto start = std::chrono::steady_clock::now();
            double acc = 0.0;
            for (int i = 0; i < 1000000; ++i)
                acc += f.process((float)(0.5 * std::sin(0.001 * i)));
            const auto end = std::chrono::steady_clock::now();
            (void)acc; // keep the loop from being optimised away
            const double ns = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            times.push_back(ns / 1000000.0);
        }
        std::sort(times.begin(), times.end());
        const double median = times[8];
        std::printf("  median %.1f ns/sample (min %.1f, max %.1f) — 10 tanh/host sample\n",
                    median, times.front(), times.back());
        CHECK(median > 0.0 && median < 1000.0);
    }

    std::printf("\n=====================================================================\n");
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    if (g_failures == 0)
        std::printf("ALL TESTS PASSED\n");
    return g_failures == 0 ? 0 : 1;
}
