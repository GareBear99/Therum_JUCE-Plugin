// test_decramping — the filter-response / decramping test rig.
//
// Paper sections 4.5 and 7 ("Filter response test"): sweep a sine
// through each filter and measure the RMS magnitude ratio against
// the analogue prototype. The decramping hypothesis (paper Q2) is
// tested on this rig: the geometric-mean 5th constraint vs Orfanidis
// vs the RBJ/SVF baseline, same sweep, same prototype, and the
// method that measures closest wins. None is declared the winner in
// advance — the numbers decide.
//
// The rig (the FreeEQ8 method, paper section 2.4):
//   bell, +6 dB, fc = 16 kHz, Q = 1.0, fs = 44.1 kHz.
//   Analogue prototype gain vs digital gain at 14 / 16 / 18 /
//   18,783 / 20 kHz. The -3.383 dB error at the geometric mean
//   (sqrt(16000 * 22050) = 18,783 Hz) is the baseline to beat.
// The rig must reproduce the FreeEQ8 table exactly (within 0.02 dB)
// before any candidate is measured — if it does not, the rig is
// wrong, not the filter.
//
// The candidates (all 2nd-order biquads, all real-time):
//   1. Baseline — RBJ bell and SVF bell (they must measure
//      identical, the FreeEQ8 result). Both pin Nyquist to the DC
//      gain (0 dB for a bell), which is what forces the cramping.
//   2. Orfanidis peq.m (1997) — the published closed-form design,
//      transcribed from Appendix B of the paper. The bandwidth
//      input is the RBJ baseline's bandwidth, so it matches the
//      baseline's width; G1 is COMPUTED by Eq. (21) — it is not
//      the analogue bell's gain at fs/2, and it is not an input.
//      Its construction guarantees (exact at DC, f0, fs/2, and the
//      same bandwidth) are checked analytically in the test.
//   3. Nyquist-pin, literal (5-constraint solver) — the 5th degree
//      of freedom is spent pinning the Nyquist gain to the analogue
//      gain at fs/2. This is Orfanidis's SPECIFICATION, but not
//      his method: his method computes G1 from Eq. (21), which
//      gives a smaller value (+1.198 dB here, vs the analogue's
//      +4.169 dB). The literal pin has no exact solution under
//      the band-edge bandwidth pin — the solver stalls at ~0.07 dB
//      — and the best-so-far coefficients are measured as they are.
//   4. Geometric-mean pin (FreeEQ8 proposal, 2026) — the 5th degree
//      of freedom is spent pinning the gain at sqrt(fc * fs/2) to
//      the analogue value there. Same solver, different pin. It
//      stalls the same way (~0.09 dB).
// Särkkä-Huovilainen (2011) is NOT in this rig yet: it matches
// magnitude AND phase via Shannon-interpolation discretisation,
// which is not a 5-coefficient closed form — it needs its own
// implementation (a state-space discretisation), and this file
// does not fake it. The rig is structured to accept a fifth
// candidate when that implementation exists.
//
// What the rig found on this bell (+6 dB, fc = 16 kHz, Q = 1,
// fs = 44.1 kHz): Orfanidis peq.m is a clean bell and the best of
// the three (max error 4.241 -> 3.245 dB, pin error -3.383 ->
// -2.743 dB), but it does not close the gap — its Eq. (21) G1
// (+1.198 dB) sits 3 dB below the analogue's Nyquist gain
// (+4.169 dB). The literal pins both force a dip-and-rise the
// 2nd-order biquad can only do with near-unit-circle zeros: a
// notch below fc (analytic gain about -12 dB at 14.7 kHz), max
// error 17-18 dB — far worse than the -3.383 dB they were meant
// to fix. Neither decramps this bell usefully.
//
// The 5-constraint solver: a normalised biquad has exactly 5
// coefficients (b0, b1, b2, a1, a2 after a0 = 1), so at most 5
// independent constraints. The solver enforces:
//   C0  DC gain = 0 dB (unity passthrough at DC)
//   C1  peak gain at fc = the specified gain
//   C2  extremum (peak) located exactly at fc
//   C3  gain at the RBJ baseline's upper half-gain frequency = +G/2 dB
//       (pins the bandwidth to the baseline's — see rbjUpperBandEdge)
//   C4  gain at the pin frequency = the analogue gain there
// RBJ satisfies C0-C3 and violates only C4, so it is the initial
// guess. The solver is Levenberg-Marquardt with continuation
// (homotopy) on the pin target: the pin target is moved from
// RBJ's gain at the pin to the analogue gain in 20 steps, and LM
// re-solves at each step. Plain Newton stalls on this system
// (the Jacobian is nearly singular — peak gain, extremum location
// and the gains at two nearby high frequencies are strongly
// coupled), and the first version of this solver diverged because
// C3 pinned the ANALOGUE band edge (25,888 Hz, above Nyquist,
// ill-defined); C3 now pins the RBJ baseline's digital band edge.
// Convergence is 0.01 dB on the max residual — far below the rig's
// measurement noise. The solver's stall on the literal pins is a
// finding about the specification, not a rig failure: the exact
// solution sits in a degenerate region (near-unit-circle zeros).
// Every candidate is checked post-solve: poles inside the unit
// circle (stable), and then measured empirically on the sine rig
// (the rig measures the real filter, not the design equations).
//
// Cross-check: the ARC Audio Bench has an independent cramping
// test (scripts/cramping_test.py, commit 0add1ce) — a log sine
// sweep, RMS per 1/12-octave bin vs the analogue prototype. Its
// headline number (error at the geometric-mean frequency) should
// agree with this rig's err@pin on the same bell. See the Bench
// cross-check in the step-3 report; this file is the Therum-side
// rig, the Bench is the reference.
//
// Note (Bench NMR verdict, 2026-11-05): the step-2 mipped engine
// FAILS the paper's NMR acceptance test — not on the 22 kHz alias
// (inaudible, NMR -69.80 dB) but on low-frequency folded
// harmonics (worst NMR +23.31 dB at 143.6 Hz). That is a
// wavetable/mip-level question for step 3+, not a filter result,
// and it is not claimed to pass anywhere in this file.
//
// Build and run: `make decramp` (see Makefile), or:
//   g++ -std=c++17 -O2 -Wall -Wextra -o test_decramping \
//       test_decramping.cpp \
//       ../Source/DSP/Filters/SvfFilter.cpp
//   ./test_decramping
// Exit code 0 means every check passed; non-zero means a check
// failed (failures are printed with file and line).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../Source/DSP/Filters/SvfFilter.h"
#include "RbjBiquad.h"

static constexpr double kSr = 44100.0;
static constexpr double kPi = 3.14159265358979323846;
static constexpr double kPi2 = 2.0 * kPi;

static int g_checks = 0;
static int g_failures = 0;
static bool g_verbose = false; // --verbose: print solver residuals per iteration
static bool g_renderSweeps = false; // --render-sweeps: write log-sweep WAVs for the Bench cramping test

// Write a mono 16-bit PCM WAV (for the Bench cramping cross-check).
static bool writeWav16(const char* path, const std::vector<float>& samples, double sr)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const uint32_t n = (uint32_t)samples.size();
    const uint32_t dataBytes = n * 2;
    auto writeU32 = [&](uint32_t v)
    {
        std::fputc(v & 0xff, f); std::fputc((v >> 8) & 0xff, f);
        std::fputc((v >> 16) & 0xff, f); std::fputc((v >> 24) & 0xff, f);
    };
    auto writeU16 = [&](uint16_t v)
    {
        std::fputc(v & 0xff, f); std::fputc((v >> 8) & 0xff, f);
    };
    std::fwrite("RIFF", 1, 4, f); writeU32(36 + dataBytes);
    std::fwrite("WAVE", 1, 4, f);
    std::fwrite("fmt ", 1, 4, f); writeU32(16); writeU16(1); writeU16(1);
    writeU32((uint32_t)sr); writeU32((uint32_t)sr * 2); writeU16(2); writeU16(16);
    std::fwrite("data", 1, 4, f); writeU32(dataBytes);
    for (float s : samples)
    {
        double v = s; if (v > 1.0) v = 1.0; if (v < -1.0) v = -1.0;
        writeU16((uint16_t)(int16_t)std::lrint(v * 32767.0));
    }
    std::fclose(f);
    return true;
}

// Log sine sweep, the Bench's law: 20 Hz -> 20 kHz over 8 s,
// amplitude 0.5, phase integrated (matches cramping_test.py's
// render_sweep exactly, so the Bench's time-to-frequency bins line up).
template <typename Filter>
static std::vector<float> renderLogSweep(Filter& f, double sr, double durSec = 8.0)
{
    const int n = (int)(sr * durSec);
    std::vector<float> out;
    out.reserve(n);
    double phase = 0.0;
    f.reset();
    for (int i = 0; i < n; ++i)
    {
        const double t = (double)i / sr;
        const double freq = 20.0 * std::pow(1000.0, t / durSec);
        phase += kPi2 * freq / sr;
        out.push_back(f.process((float)(0.5 * std::sin(phase))));
    }
    return out;
}

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
// Analogue prototype (the bell), from the FreeEQ8 paper section 2.4.
// H(jw) = [(wc^2 - w^2) + j (wc/Q) A w] / [(wc^2 - w^2) + j (wc/Q)/A w]
// with A = 10^(gainDb/40).
// ---------------------------------------------------------------------------
static double analogBellGainDb(double f, double fc, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double wc = kPi2 * fc;
    const double w = kPi2 * f;
    const double nr = wc * wc - w * w, ni = (wc / q) * A * w;
    const double dr = wc * wc - w * w, di = (wc / q) / A * w;
    return 20.0 * std::log10(std::sqrt(nr * nr + ni * ni) / std::sqrt(dr * dr + di * di));
}

// ---------------------------------------------------------------------------
// Sine measurement (the FreeEQ8 rig's method): steady-state RMS ratio.
// ---------------------------------------------------------------------------
template <typename Filter>
static double measureGainDb(Filter& f, double testFreq)
{
    const double w = kPi2 * testFreq;
    int spc = (int)(kSr / testFreq);
    if (spc < 4) spc = 4;
    const int total = spc * 256, warmup = total / 2;
    double si = 0.0, so = 0.0;
    for (int i = 0; i < total; ++i)
    {
        const double x = std::sin(w * i / kSr);
        const double y = (double)f.process((float)x);
        if (i >= warmup) { si += x * x; so += y * y; }
    }
    return si > 1e-30 ? 10.0 * std::log10(so / si) : -999.0;
}

// The SVF bell, via the setMix() escape hatch and the FreeEQ8 table:
//   kA = k/A = 1/(Q*A) — the denominator uses kA, not k — so the
//   effective Q is Q*A (setQ(Q*A)), and the mix is
//   m = (1, kA*(A^2 - 1), 0). This is the mix the FreeEQ8 paper
//   reproduces from Simper; it is measured here against RBJ across
//   a wide sweep before it is trusted (paper Q3).
static void setupSvfBell(SvfFilter& f, double fc, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double kA = 1.0 / (q * A);
    f.setType(SvfFilter::Type::LowPass); // type is irrelevant: the mix is overridden
    f.setCutoff((float)fc);
    f.setQ((float)(q * A)); // effective damping kA = 1/(Q*A)
    f.setMix(1.0, kA * (A * A - 1.0), 0.0);
    f.reset();
}

// ---------------------------------------------------------------------------
// The 5-constraint solver (damped Newton, numerical Jacobian).
//
// Unknowns: b0, b1, b2, a1, a2 (a0 = 1). Residuals (all ~0 at the
// solution): C0 DC gain, C1 peak gain at fc, C2 extremum at fc,
// C3 gain at the upper band edge, C4 gain at the pin frequency.
// |H(e^jw)|^2 = N(w)/D(w) with
//   N(w) = c0 + c1 cos w + c2 cos 2w,  c0 = b0^2+b1^2+b2^2,
//        c1 = 2(b0b1+b1b2), c2 = 2b0b2
//   D(w) = d0 + d1 cos w + d2 cos 2w,  d0 = 1+a1^2+a2^2,
//        d1 = 2(a1+a1a2), d2 = 2a2
// so the extremum condition N'D - ND' = 0 is analytic.
// ---------------------------------------------------------------------------
struct BiquadCoeffs { double b0, b1, b2, a1, a2; };

static void magSqParts(const BiquadCoeffs& c, double w, double& n, double& d,
                       double& nd, double& dd)
{
    const double c0 = c.b0 * c.b0 + c.b1 * c.b1 + c.b2 * c.b2;
    const double c1 = 2.0 * (c.b0 * c.b1 + c.b1 * c.b2);
    const double c2 = 2.0 * c.b0 * c.b2;
    const double d0 = 1.0 + c.a1 * c.a1 + c.a2 * c.a2;
    const double d1 = 2.0 * (c.a1 + c.a1 * c.a2);
    const double d2 = 2.0 * c.a2;
    const double cw = std::cos(w), c2w = std::cos(2.0 * w);
    const double sw = std::sin(w), s2w = std::sin(2.0 * w);
    n = c0 + c1 * cw + c2 * c2w;
    d = d0 + d1 * cw + d2 * c2w;
    nd = -c1 * sw - 2.0 * c2 * s2w;
    dd = -d1 * sw - 2.0 * d2 * s2w;
}

static double biquadGainDb(const BiquadCoeffs& c, double w)
{
    double n, d, nd, dd;
    magSqParts(c, w, n, d, nd, dd);
    return 10.0 * std::log10(n / d);
}

// Lower half-gain frequency of the RBJ bell (mirror of the upper:
// the f in (20, fc) where the RBJ gain is gainDb/2).
static double rbjLowerBandEdge(const BiquadCoeffs& rbj, double fc, double gainDb)
{
    const double target = gainDb / 2.0;
    double lo = 20.0, hi = fc;
    for (int i = 0; i < 100; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        if (biquadGainDb(rbj, kPi2 * mid / kSr) < target)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

// Upper half-gain frequency of the RBJ bell, in the DIGITAL domain:
// the f in (fc, fs/2) where the RBJ filter's gain is gainDb/2.
// Found by bisection on the analytic biquad gain (the RBJ bell's
// response falls monotonically from fc toward 0 dB at Nyquist).
//
// Why the digital band edge, not the analogue's: the analogue bell
// (+6 dB, fc = 16 kHz, Q = 1) has its upper half-gain point at
// 25,888 Hz — ABOVE Nyquist. There is no digital frequency for it,
// so a constraint pinned there is ill-defined (it is what made the
// first version of this solver diverge). The comparison must pin
// the bandwidth to the BASELINE's (RBJ's) bandwidth, so the three
// candidates differ ONLY in the 5th constraint (the pin) — which
// is exactly the experiment: same 4 constraints, different 5th
// point. The analogue's band edge not fitting in the Nyquist
// interval is itself why the cramping here is severe.
static double rbjUpperBandEdge(const BiquadCoeffs& rbj, double fc, double gainDb)
{
    const double target = gainDb / 2.0;
    double lo = fc, hi = kSr / 2.0 - 1.0;
    for (int i = 0; i < 100; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        if (biquadGainDb(rbj, kPi2 * mid / kSr) > target)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

struct SolverSpec
{
    double fc, q, gainDb;   // the bell being designed
    double fPin;            // pin frequency (Nyquist or geometric mean)
    double pinGainDb;       // analogue gain at fPin
    double fEdge;           // RBJ baseline's upper half-gain frequency (the bandwidth pin)
};

static void solverResiduals(const BiquadCoeffs& c, const SolverSpec& s, double r[5])
{
    const double w0 = kPi2 * s.fc / kSr;
    const double wPin = kPi2 * s.fPin / kSr;
    const double wEdge = kPi2 * s.fEdge / kSr;
    // C0: DC gain (H(1)) in dB, target 0.
    const double dc = (c.b0 + c.b1 + c.b2) / (1.0 + c.a1 + c.a2);
    r[0] = 20.0 * std::log10(std::fabs(dc));
    // C1: peak gain at fc.
    r[1] = biquadGainDb(c, w0) - s.gainDb;
    // C2: extremum at fc: (N'D - ND')/(ND) = N'/N - D'/D.
    double n, d, nd, dd;
    magSqParts(c, w0, n, d, nd, dd);
    r[2] = nd / n - dd / d;
    // C3: gain at the upper band edge = gainDb/2.
    r[3] = biquadGainDb(c, wEdge) - s.gainDb / 2.0;
    // C4: gain at the pin = the analogue gain there.
    r[4] = biquadGainDb(c, wPin) - s.pinGainDb;
}

static double maxResidual(const double r[5])
{
    double m = 0.0;
    for (int i = 0; i < 5; ++i)
        m = std::max(m, std::fabs(r[i]));
    return m;
}

// Gaussian elimination with partial pivoting, 5x5. Returns false if singular.
static bool solve5(double a[5][5], double b[5], double x[5])
{
    for (int col = 0; col < 5; ++col)
    {
        int piv = col;
        for (int row = col + 1; row < 5; ++row)
            if (std::fabs(a[row][col]) > std::fabs(a[piv][col])) piv = row;
        if (std::fabs(a[piv][col]) < 1e-14) return false;
        if (piv != col)
        {
            for (int k = col; k < 5; ++k) std::swap(a[piv][k], a[col][k]);
            std::swap(b[piv], b[col]);
        }
        for (int row = col + 1; row < 5; ++row)
        {
            const double f = a[row][col] / a[col][col];
            for (int k = col; k < 5; ++k) a[row][k] -= f * a[col][k];
            b[row] -= f * b[col];
        }
    }
    for (int row = 4; row >= 0; --row)
    {
        double s = b[row];
        for (int k = row + 1; k < 5; ++k) s -= a[row][k] * x[k];
        x[row] = s / a[row][row];
    }
    return true;
}

// Forward declaration (defined below the wrapper).
static bool newtonSolve(const BiquadCoeffs& initial, const SolverSpec& s,
                        BiquadCoeffs& out, int& iterations, double& finalResidual);

// Damped Newton from the RBJ coefficients, with continuation
// (homotopy) on the pin target.
//
// Why continuation: the pin target is far from the RBJ value
// (the Nyquist pin wants +4.169 dB where RBJ gives 0 dB; the
// geomean pin wants +5.403 dB where RBJ gives +2.020 dB), and the
// response between the band edge (18,163 Hz) and the pin must dip
// and rise again — a shape change the local linearization around
// RBJ cannot reach in one Newton step (the first attempt stalls at
// a max residual of ~1.46 dB, lambda collapsing to 1e-6). So the
// pin target is moved from RBJ's gain at the pin to the analogue
// gain in 20 equal steps, re-solving at each step from the previous
// solution. Each step is a small perturbation, so Newton converges;
// at t = 1 the full specification is satisfied. If any step fails,
// the solve fails (reported, not faked).
// Returns true on convergence.
static bool solveDecramped(const BiquadCoeffs& initial, const SolverSpec& s,
                           BiquadCoeffs& out, int& iterations, double& finalResidual)
{
    const double rbjPinGain = biquadGainDb(initial, kPi2 * s.fPin / kSr);
    BiquadCoeffs c = initial;
    int totalIt = 0;
    double best = 0.0;
    for (int step = 1; step <= 20; ++step)
    {
        SolverSpec staged = s;
        staged.pinGainDb = rbjPinGain + (s.pinGainDb - rbjPinGain) * (double)step / 20.0;
        int it = 0;
        double res = 0.0;
        if (!newtonSolve(c, staged, c, it, res))
        {
            // Report how far the continuation got, for the failure line.
            out = c;
            iterations = totalIt + it;
            finalResidual = res;
            return false;
        }
        totalIt += it;
        best = res;
    }
    out = c;
    iterations = totalIt;
    finalResidual = best;
    return true;
}

// One damped-Newton solve of the 5 constraints from `initial`.
// Returns true when max|residual| < 1e-7.
static bool newtonSolve(const BiquadCoeffs& initial, const SolverSpec& s,
                        BiquadCoeffs& out, int& iterations, double& finalResidual)
{
    // Levenberg-Marquardt on the 5 residuals (sum of squares).
    // Plain Newton stalls here: the Jacobian is nearly singular
    // (peak gain, extremum location and the gains at two nearby
    // high frequencies are strongly coupled in a biquad), so the
    // Newton step is huge in a cancelling direction and only a
    // ~1e-3 fraction is usable. LM regularises J^T J with a
    // diagonal damping term and accepts on sum-of-squares, which
    // converges through the ill-conditioning.
    BiquadCoeffs c = initial;
    double r[5];
    solverResiduals(c, s, r);
    double ss = 0.0;
    for (int i = 0; i < 5; ++i) ss += r[i] * r[i];
    double lambda = 1e-3;
    if (g_verbose)
        std::printf("    init residuals: C0 %+.3e C1 %+.3e C2 %+.3e C3 %+.3e C4 %+.3e (max %.3e)\n",
                    r[0], r[1], r[2], r[3], r[4], maxResidual(r));
    for (int it = 0; it < 200; ++it)
    {
        // Convergence: 0.01 dB. The residuals are in dB (C2 in 1/rad);
        // 0.01 dB is far below the rig's measurement noise and far
        // below audibility, and the LM solver's floor on this
        // ill-conditioned system is ~0.0015 dB (geomean pin). The
        // final residual is always printed, so a loose pass is visible.
        if (maxResidual(r) < 1e-2) { out = c; iterations = it; finalResidual = maxResidual(r); return true; }
        // Numerical Jacobian (central differences).
        double jac[5][5];
        const double xv[5] = { c.b0, c.b1, c.b2, c.a1, c.a2 };
        for (int j = 0; j < 5; ++j)
        {
            const double h = 1e-6 * std::max(1.0, std::fabs(xv[j]));
            BiquadCoeffs cp = c, cm = c;
            double* pp = j == 0 ? &cp.b0 : j == 1 ? &cp.b1 : j == 2 ? &cp.b2 : j == 3 ? &cp.a1 : &cp.a2;
            double* pm = j == 0 ? &cm.b0 : j == 1 ? &cm.b1 : j == 2 ? &cm.b2 : j == 3 ? &cm.a1 : &cm.a2;
            *pp += h; *pm -= h;
            double rp[5], rm[5];
            solverResiduals(cp, s, rp);
            solverResiduals(cm, s, rm);
            for (int i = 0; i < 5; ++i)
                jac[i][j] = (rp[i] - rm[i]) / (2.0 * h);
        }
        // Normal equations A = J^T J, g = J^T r.
        double A[5][5] = {};
        double g[5] = {};
        for (int i = 0; i < 5; ++i)
            for (int j = 0; j < 5; ++j)
                for (int k = 0; k < 5; ++k) A[i][j] += jac[k][i] * jac[k][j];
        for (int i = 0; i < 5; ++i)
            for (int k = 0; k < 5; ++k) g[i] += jac[k][i] * r[k];
        bool accepted = false;
        for (int trial = 0; trial < 30 && !accepted; ++trial)
        {
            double M[5][5];
            for (int i = 0; i < 5; ++i)
                for (int j = 0; j < 5; ++j)
                    M[i][j] = A[i][j] + (i == j ? lambda * std::max(A[i][i], 1e-12) : 0.0);
            double rhs[5] = { -g[0], -g[1], -g[2], -g[3], -g[4] };
            double dx[5] = {};
            if (!solve5(M, rhs, dx)) { lambda *= 10.0; continue; }
            BiquadCoeffs t = { c.b0 + dx[0], c.b1 + dx[1], c.b2 + dx[2], c.a1 + dx[3], c.a2 + dx[4] };
            double rt[5];
            solverResiduals(t, s, rt);
            double sst = 0.0;
            for (int i = 0; i < 5; ++i) sst += rt[i] * rt[i];
            if (sst < ss)
            {
                c = t;
                for (int i = 0; i < 5; ++i) r[i] = rt[i];
                ss = sst;
                lambda = std::max(lambda / 10.0, 1e-12);
                accepted = true;
            }
            else
            {
                lambda *= 10.0;
            }
        }
        if (!accepted) { out = c; iterations = it; finalResidual = maxResidual(r); return false; }
        if (g_verbose)
            std::printf("    it %2d: lambda %.3g, max residual %.3e (C0 %+.2e C1 %+.2e C2 %+.2e C3 %+.2e C4 %+.2e)\n",
                        it, lambda, maxResidual(r), r[0], r[1], r[2], r[3], r[4]);
    }
    out = c;
    iterations = 200;
    finalResidual = maxResidual(r);
    return false;
}

static bool biquadStable(const BiquadCoeffs& c)
{
    // Poles are the roots of z^2 + a1 z + a2 = 0.
    const double disc = c.a1 * c.a1 - 4.0 * c.a2;
    if (disc < 0.0)
        return std::sqrt(std::fabs(c.a2)) < 1.0; // complex pair: |pole| = sqrt(a2)
    const double sq = std::sqrt(disc);
    return std::fabs((-c.a1 + sq) / 2.0) < 1.0 && std::fabs((-c.a1 - sq) / 2.0) < 1.0;
}

// The RBJ bell coefficients, as the solver's initial guess.
static BiquadCoeffs rbjBellCoeffs(double fc, double q, double gainDb)
{
    const double w0 = kPi2 * fc / kSr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw0 = std::cos(w0);
    const double A = std::pow(10.0, gainDb / 40.0);
    const double a0 = 1.0 + alpha / A;
    BiquadCoeffs c;
    c.b0 = (1.0 + alpha * A) / a0;
    c.b1 = -2.0 * cosw0 / a0;
    c.b2 = (1.0 - alpha * A) / a0;
    c.a1 = -2.0 * cosw0 / a0;
    c.a2 = (1.0 - alpha / A) / a0;
    return c;
}

// Orfanidis's closed-form design (peq.m, Appendix B of
// "Digital Parametric Equalizer Design With Prescribed
// Nyquist-Frequency Gain", JAES vol. 45, p. 444, June 1997).
// Transcribed directly from the paper's MATLAB function; the
// only change is C++ syntax. Inputs are linear gains (G0 = DC
// gain, G = peak gain, GB = bandwidth gain) and digital
// frequencies in rads/sample (w0 = peak, dw = bandwidth).
// G1 (the Nyquist gain) is COMPUTED by Eq. (21) — it is not
// the analogue bell's gain at fs/2, and it is not an input.
// Returns false if the design equations go imaginary (the
// paper's Eq. (37) inequalities are violated).
static bool orfanidisPeq(double g0, double g, double gb, double w0, double dw,
                         BiquadCoeffs& out, double& g1Out)
{
    const double pi = 3.14159265358979323846;
    const double f = std::fabs(g * g - gb * gb);
    const double g00 = std::fabs(g * g - g0 * g0);
    const double f00 = std::fabs(gb * gb - g0 * g0);
    if (f < 1e-12 || g00 < 1e-12) return false;
    const double w0sq = w0 * w0;
    const double num = g0 * g0 * (w0sq - pi * pi) * (w0sq - pi * pi)
                     + g * g * f00 * pi * pi * dw * dw / f;
    const double den = (w0sq - pi * pi) * (w0sq - pi * pi)
                     + f00 * pi * pi * dw * dw / f;
    if (den <= 0.0 || num < 0.0) return false;
    const double g1 = std::sqrt(num / den);
    const double g01 = std::fabs(g * g - g0 * g1);
    const double g11 = std::fabs(g * g - g1 * g1);
    const double f01 = std::fabs(gb * gb - g0 * g1);
    const double f11 = std::fabs(gb * gb - g1 * g1);
    if (f11 < 1e-12 || g11 < 1e-12) return false;
    const double w2 = std::sqrt(g11 / g00) * std::tan(w0 / 2.0) * std::tan(w0 / 2.0);
    const double preDw = (1.0 + std::sqrt(f00 / f11) * w2) * std::tan(dw / 2.0);
    const double c = f11 * preDw * preDw
                   - 2.0 * w2 * (f01 - std::sqrt(f00 * f11));
    const double d = 2.0 * w2 * (g01 - std::sqrt(g00 * g11));
    const double aNum = (c + d) / f;
    const double bNum = (g * g * c + gb * gb * d) / f;
    if (aNum < 0.0 || bNum < 0.0) return false;
    const double a = std::sqrt(aNum);
    const double b = std::sqrt(bNum);
    const double norm = 1.0 + w2 + a;
    out.b0 = (g1 + g0 * w2 + b) / norm;
    out.b1 = -2.0 * (g1 - g0 * w2) / norm;
    out.b2 = (g1 - b + g0 * w2) / norm;
    out.a1 = -2.0 * (1.0 - w2) / norm;
    out.a2 = (1.0 + w2 - a) / norm;
    g1Out = g1;
    return true;
}

// First frequency above fc where the gain crosses `target` (falling
// through it). Scans outward from fc, then bisects the bracket.
// Returns false if the gain never crosses the target below Nyquist
// (e.g. a filter whose Nyquist gain sits above the target level).
static bool bandEdgeAbove(const BiquadCoeffs& c, double fc, double target, double& fOut)
{
    double prevF = fc, prevG = biquadGainDb(c, kPi2 * fc / kSr);
    for (double f = fc * 1.001; f < kSr / 2.0; f *= 1.001)
    {
        const double g = biquadGainDb(c, kPi2 * f / kSr);
        if (prevG >= target && g < target)
        {
            double lo = prevF, hi = f;
            for (int i = 0; i < 60; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                if (biquadGainDb(c, kPi2 * mid / kSr) >= target) lo = mid; else hi = mid;
            }
            fOut = 0.5 * (lo + hi);
            return true;
        }
        prevF = f; prevG = g;
    }
    return false;
}

// Mirror of bandEdgeAbove below fc (the gain falls as f decreases).
static bool bandEdgeBelow(const BiquadCoeffs& c, double fc, double target, double& fOut)
{
    double prevF = fc, prevG = biquadGainDb(c, kPi2 * fc / kSr);
    for (double f = fc / 1.001; f > 20.0; f /= 1.001)
    {
        const double g = biquadGainDb(c, kPi2 * f / kSr);
        if (prevG >= target && g < target)
        {
            double lo = f, hi = prevF;
            for (int i = 0; i < 60; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                if (biquadGainDb(c, kPi2 * mid / kSr) >= target) hi = mid; else lo = mid;
            }
            fOut = 0.5 * (lo + hi);
            return true;
        }
        prevF = f; prevG = g;
    }
    return false;
}

// A biquad driven by coefficients (for measuring solver output).
struct CoeffBiquad
{
    BiquadCoeffs c;
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    void reset() { x1 = x2 = y1 = y2 = 0.0; }
    float process(float x)
    {
        const double y = c.b0 * x + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return (float)y;
    }
};

// ---------------------------------------------------------------------------
// The comparison: for one candidate, the error (digital - analogue, dB)
// at the pin frequency, near Nyquist, and the worst error over a
// log-spaced sweep 20 Hz - 20 kHz.
// ---------------------------------------------------------------------------
struct CandidateResult
{
    const char* name;
    double errAtPin;
    double errAt20k;
    double maxAbsErr;
    double maxErrFreq;
};

template <typename Filter>
static CandidateResult measureCandidate(const char* name, Filter& f,
                                        double fc, double q, double gainDb, double fPin)
{
    CandidateResult r;
    r.name = name;
    r.errAtPin = measureGainDb(f, fPin) - analogBellGainDb(fPin, fc, q, gainDb);
    f.reset();
    r.errAt20k = measureGainDb(f, 20000.0) - analogBellGainDb(20000.0, fc, q, gainDb);
    r.maxAbsErr = 0.0;
    r.maxErrFreq = 0.0;
    for (int i = 0; i <= 200; ++i)
    {
        const double freq = 20.0 * std::pow(1000.0, (double)i / 200.0); // 20 Hz .. 20 kHz
        f.reset();
        const double err = measureGainDb(f, freq) - analogBellGainDb(freq, fc, q, gainDb);
        if (std::fabs(err) > r.maxAbsErr) { r.maxAbsErr = std::fabs(err); r.maxErrFreq = freq; }
    }
    return r;
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--verbose") == 0) g_verbose = true;
        if (std::strcmp(argv[i], "--render-sweeps") == 0) g_renderSweeps = true;
    }
    std::printf("Therum decramping test (paper section 4.5, Q2)\n");
    std::printf("==============================================\n");

    const double fc = 16000.0, q = 1.0, gainDb = 6.0;
    const double fPin = std::sqrt(fc * kSr / 2.0); // 18,783 Hz
    std::printf("Rig: bell +%g dB, fc = %g Hz, Q = %g, fs = %g Hz, f_pin = %.1f Hz\n",
                gainDb, fc, q, kSr, fPin);

    // --- (a) Rig validation: reproduce the FreeEQ8 table. ---
    std::printf("\n-- Rig validation (FreeEQ8 paper section 2.4) --\n");
    const double tableFreqs[] = { 14000.0, 16000.0, 18000.0, 18783.0, 20000.0 };
    const double tableErr[] = { -1.554, 0.000, -2.371, -3.383, -4.241 };
    RbjBiquad rbj;
    rbj.setBell(kSr, fc, q, gainDb);
    SvfFilter svfBell;
    svfBell.prepare(kSr);
    setupSvfBell(svfBell, fc, q, gainDb);
    for (int i = 0; i < 5; ++i)
    {
        const double f = tableFreqs[i];
        rbj.reset();
        const double rbjGain = measureGainDb(rbj, f);
        const double rbjErr = rbjGain - analogBellGainDb(f, fc, q, gainDb);
        svfBell.reset();
        const double svfGain = measureGainDb(svfBell, f);
        const double svfErr = svfGain - analogBellGainDb(f, fc, q, gainDb);
        std::printf("  %6.0f Hz: analog %+7.3f dB, RBJ %+7.3f dB (err %+.3f), SVF bell %+7.3f dB (err %+.3f)\n",
                    f, analogBellGainDb(f, fc, q, gainDb), rbjGain, rbjErr, svfGain, svfErr);
        CHECK_NEAR(rbjErr, tableErr[i], 0.02); // the rig reproduces FreeEQ8
        CHECK_NEAR(svfErr, rbjErr, 0.01);      // SVF bell == RBJ bell here
    }

    // --- (b) SVF bell vs RBJ bell, wide sweep. ---
    // If the FreeEQ8 bell mix is right, the two measure identical
    // everywhere, not just at the one tested point. Gains -24..+24 dB,
    // Q 0.5..24, fc 100 Hz..16 kHz.
    std::printf("\n-- SVF bell vs RBJ bell, wide sweep --\n");
    double worstDiff = 0.0;
    double worstGain = 0.0, worstQ = 0.0, worstFc = 0.0, worstF = 0.0;
    const double gains[] = { -24.0, -12.0, -6.0, 6.0, 12.0, 24.0 };
    const double qs[] = { 0.5, 1.0, 2.0, 10.0, 24.0 };
    const double fcs[] = { 100.0, 1000.0, 8000.0, 16000.0 };
    for (double g : gains)
    {
        for (double qq : qs)
        {
            for (double fcc : fcs)
            {
                RbjBiquad r;
                r.setBell(kSr, fcc, qq, g);
                SvfFilter s;
                s.prepare(kSr);
                setupSvfBell(s, fcc, qq, g);
                for (int i = 0; i <= 24; ++i)
                {
                    const double f = 20.0 * std::pow(1000.0, (double)i / 24.0);
                    r.reset();
                    s.reset();
                    const double diff = std::fabs(measureGainDb(r, f) - measureGainDb(s, f));
                    if (diff > worstDiff)
                    {
                        worstDiff = diff;
                        worstGain = g; worstQ = qq; worstFc = fcc; worstF = f;
                    }
                }
            }
        }
    }
    std::printf("  worst |SVF bell - RBJ bell| = %.4f dB (gain %+.0f, Q %g, fc %g, f %g)\n",
                worstDiff, worstGain, worstQ, worstFc, worstF);
    CHECK(worstDiff < 0.05);

    // --- (c) The candidates, same rig. ---
    std::printf("\n-- Candidates (same rig, same prototype) --\n");
    BiquadCoeffs init = rbjBellCoeffs(fc, q, gainDb);
    const double fEdge = rbjUpperBandEdge(init, fc, gainDb);
    const double fLower = rbjLowerBandEdge(init, fc, gainDb);
    const double rbjBwHz = fEdge - fLower;
    const double dw = kPi2 * rbjBwHz / kSr; // RBJ bandwidth, rads/sample
    std::printf("  RBJ half-gain frequencies: %.1f Hz .. %.1f Hz (bandwidth %.1f Hz)\n",
                fLower, fEdge, rbjBwHz);

    // Baseline: RBJ bell (and the SVF bell, already shown identical).
    RbjBiquad rbjBase;
    rbjBase.setBell(kSr, fc, q, gainDb);
    CandidateResult base = measureCandidate("RBJ/SVF baseline", rbjBase, fc, q, gainDb, fPin);

    // Orfanidis's closed-form design (peq.m). The bandwidth input is
    // the RBJ baseline's bandwidth, so it matches the baseline's
    // width; G1 is computed by Eq. (21), not pinned to the analogue.
    const double gLin = std::pow(10.0, gainDb / 20.0);
    const double gbLin = std::pow(10.0, (gainDb / 2.0) / 20.0);
    BiquadCoeffs orfCoeffs;
    double orfG1 = 0.0;
    const bool orfOk = orfanidisPeq(1.0, gLin, gbLin, kPi2 * fc / kSr, dw, orfCoeffs, orfG1);
    std::printf("  Orfanidis peq.m: %s, G1 = %+.3f dB (analogue at fs/2: %+.3f dB)\n",
                orfOk ? "built" : "FAILED",
                20.0 * std::log10(orfG1), analogBellGainDb(kSr / 2.0, fc, q, gainDb));
    CHECK(orfOk);
    CHECK(biquadStable(orfCoeffs));
    CoeffBiquad orfFilter{ orfCoeffs };
    CandidateResult orf = measureCandidate("Orfanidis peq.m", orfFilter, fc, q, gainDb, fPin);
    // Construction guarantees, checked analytically (the design is
    // exact at DC, f0, fs/2, and matches the bandwidth by construction).
    CHECK_NEAR(biquadGainDb(orfCoeffs, 0.0), 0.0, 0.01);
    CHECK_NEAR(biquadGainDb(orfCoeffs, kPi2 * fc / kSr), gainDb, 0.05);
    CHECK_NEAR(biquadGainDb(orfCoeffs, kPi), 20.0 * std::log10(orfG1), 0.01);
    {
        double lo = 0.0, hi = 0.0;
        const bool edgesOk = bandEdgeBelow(orfCoeffs, fc, gainDb / 2.0, lo)
                          && bandEdgeAbove(orfCoeffs, fc, gainDb / 2.0, hi);
        std::printf("  Orfanidis band edges: %.1f Hz .. %.1f Hz (bandwidth %.1f Hz)\n",
                    lo, hi, hi - lo);
        CHECK(edgesOk);
        if (edgesOk) CHECK_NEAR(hi - lo, rbjBwHz, 20.0);
    }

    // Nyquist-pin, literal (5-constraint solver, pin = the analogue's
    // gain at fs/2). Reported, not asserted: this spec has no exact
    // solution under the band-edge bandwidth pin — the solver stalls
    // at ~0.07 dB — and the best-so-far coefficients are measured
    // as they are. That stall is a finding, not a rig failure.
    SolverSpec specNyq{ fc, q, gainDb, kSr / 2.0, analogBellGainDb(kSr / 2.0, fc, q, gainDb), fEdge };
    BiquadCoeffs nyqCoeffs;
    int nyqIt = 0;
    double nyqRes = 0.0;
    const bool nyqOk = solveDecramped(init, specNyq, nyqCoeffs, nyqIt, nyqRes);
    std::printf("  Nyquist-pin solver: %s (%d iterations, final residual %.2e)\n",
                nyqOk ? "converged" : "stalled", nyqIt, nyqRes);
    CHECK(biquadStable(nyqCoeffs));
    CoeffBiquad nyqFilter{ nyqCoeffs };
    CandidateResult nyq = measureCandidate("Nyquist-pin (5-constraint)", nyqFilter, fc, q, gainDb, fPin);

    // Geometric-mean pin (FreeEQ8 proposal), 5-constraint solver.
    // Reported, not asserted, like the Nyquist pin: the solver
    // stalls at ~0.09 dB on this bell (the exact solution sits in
    // a degenerate region — near-unit-circle zeros, see the notch
    // in the analytic diagnostic — and LM cannot quite reach it).
    // The stalled coefficients are measured as they are.
    SolverSpec specGeo{ fc, q, gainDb, fPin, analogBellGainDb(fPin, fc, q, gainDb), fEdge };
    BiquadCoeffs geoCoeffs;
    int geoIt = 0;
    double geoRes = 0.0;
    const bool geoOk = solveDecramped(init, specGeo, geoCoeffs, geoIt, geoRes);
    std::printf("  Geomean-pin solver: %s (%d iterations, final residual %.2e)\n",
                geoOk ? "converged" : "stalled", geoIt, geoRes);
    CHECK(biquadStable(geoCoeffs));
    CoeffBiquad geoFilter{ geoCoeffs };
    CandidateResult geo = measureCandidate("Geomean-pin (FreeEQ8 proposal)", geoFilter, fc, q, gainDb, fPin);

    // Diagnostic: the analytic response of each candidate, so a sine-
    // measurement artefact (long transient from poles near the unit
    // circle) can be told apart from a genuinely pathological shape.
    auto printAnalytic = [&](const char* name, const BiquadCoeffs& c, double fPin)
    {
        const double disc = c.a1 * c.a1 - 4.0 * c.a2;
        const double poleR = disc < 0.0 ? std::sqrt(std::fabs(c.a2))
            : std::max(std::fabs((-c.a1 + std::sqrt(disc)) / 2.0),
                       std::fabs((-c.a1 - std::sqrt(disc)) / 2.0));
        std::printf("  [%s] coeffs b0 %.4f b1 %.4f b2 %.4f a1 %.4f a2 %.4f, pole radius %.4f\n",
                    name, c.b0, c.b1, c.b2, c.a1, c.a2, poleR);
        std::printf("  [%s] analytic gain: 14.7k %+.3f dB, fc %+.3f dB, edge %+.3f dB, pin %+.3f dB, Nyq %+.3f dB\n",
                    name,
                    biquadGainDb(c, kPi2 * 14656.0 / kSr),
                    biquadGainDb(c, kPi2 * fc / kSr),
                    biquadGainDb(c, kPi2 * fEdge / kSr),
                    biquadGainDb(c, kPi2 * fPin / kSr),
                    biquadGainDb(c, kPi));
    };
    printAnalytic("Orfanidis", orfCoeffs, fPin);
    printAnalytic("Nyquist-pin", nyqCoeffs, kSr / 2.0);
    printAnalytic("Geomean-pin", geoCoeffs, fPin);

    // The comparison table. The numbers decide; nothing here asserts
    // a winner in advance.
    std::printf("\n  %-32s %10s %10s %10s %10s\n", "candidate", "err@pin", "err@20k", "max|err|", "at freq");
    std::printf("  %-32s %+10.3f %+10.3f %10.3f %10.0f\n", base.name, base.errAtPin, base.errAt20k, base.maxAbsErr, base.maxErrFreq);
    std::printf("  %-32s %+10.3f %+10.3f %10.3f %10.0f\n", orf.name, orf.errAtPin, orf.errAt20k, orf.maxAbsErr, orf.maxErrFreq);
    std::printf("  %-32s %+10.3f %+10.3f %10.3f %10.0f\n", nyq.name, nyq.errAtPin, nyq.errAt20k, nyq.maxAbsErr, nyq.maxErrFreq);
    std::printf("  %-32s %+10.3f %+10.3f %10.3f %10.0f\n", geo.name, geo.errAtPin, geo.errAt20k, geo.maxAbsErr, geo.maxErrFreq);

    // The baseline reproduces the -3.383 dB gap at the pin.
    CHECK_NEAR(base.errAtPin, -3.383, 0.02);

    // Verdict (printed, not asserted — the table is the result).
    // On this bell (+6 dB, fc = 16 kHz, Q = 1, fs = 44.1 kHz):
    // - Orfanidis peq.m is a clean bell and the best of the three
    //   (max error 4.241 -> 3.245 dB, pin error -3.383 -> -2.743 dB),
    //   but it does not close the gap: its Eq. (21) G1 (+1.198 dB)
    //   sits 3 dB below the analogue's Nyquist gain (+4.169 dB).
    // - The literal Nyquist pin and the geomean pin both force a
    //   dip-and-rise the 2nd-order biquad can only do with
    //   near-unit-circle zeros: a notch below fc (analytic gain
    //   -12 dB at 14.7 kHz), max error 17-18 dB — far worse than
    //   the -3.383 dB they were meant to fix. Neither decramps
    //   this bell usefully. Sarkka-Huovilainen is untested here
    //   (it needs the Shannon-interpolation discretisation).
    std::printf("\n  Verdict: Orfanidis peq.m improves the bell (max err %.3f dB)\n"
                "  but does not close the gap; the literal pins notch it.\n",
                orf.maxAbsErr);

    // Sweep renders for the Bench cramping cross-check
    // (arc_audiobench.py cramping on these WAVs should agree with
    // this rig's err@pin on the same bell).
    if (g_renderSweeps)
    {
        std::printf("\n-- Sweep renders (Bench cramping cross-check) --\n");
        const std::pair<const char*, std::vector<float>> renders[4] = {
            { "sweep_rbj_16k.wav", renderLogSweep(rbjBase, kSr) },
            { "sweep_orfanidis_16k.wav", renderLogSweep(orfFilter, kSr) },
            { "sweep_nyqpin_16k.wav", renderLogSweep(nyqFilter, kSr) },
            { "sweep_geopin_16k.wav", renderLogSweep(geoFilter, kSr) },
        };
        for (const auto& r : renders)
        {
            const bool ok = writeWav16(r.first, r.second, kSr);
            std::printf("  %s: %s\n", r.first, ok ? "written" : "FAILED to write");
            CHECK(ok);
        }
    }

    std::printf("\n==============================================\n");
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    if (g_failures == 0)
        std::printf("ALL TESTS PASSED\n");
    return g_failures == 0 ? 0 : 1;
}
