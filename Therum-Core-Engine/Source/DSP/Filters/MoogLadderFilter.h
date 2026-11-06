// MoogLadderFilter — the Huovilainen 2004 Moog ladder filter.
//
// Paper section 4.5 ("Moog ladder — the practical choice") and
// section 6, rank 7: Huovilainen 2004, DAFx-04, is the v1 choice —
// nonlinear ODEs with tanh per differential pair, discrete form
// using 5 tanh/sample, k = 4*reso, self-oscillating at finite
// amplitude (tanh saturates), 2x oversampling published (DaisySP
// uses 4x). D'Angelo-Vaelimaeki 2013/2014 is the v2 / accurate mode:
// circuit-derived, SPICE-matched, only 12 more basic ops/sample
// than Huovilainen, same tanh count — the upgrade is nearly free
// in CPU. A Zavalishin ZDF ladder is topology-exact and unified
// with the SVF codebase, but costs a Newton/analytic solve per
// sample — deferred unless Therum commits to TPT everywhere.
//
// The continuous-time model (normalised thermal voltage Vt = 1):
//   dx1/dt = wc * (tanh(u - k*x4) - tanh(x1))
//   dx2/dt = wc * (tanh(x1)     - tanh(x2))
//   dx3/dt = wc * (tanh(x2)     - tanh(x3))
//   dx4/dt = wc * (tanh(x3)     - tanh(x4))
//   y = x4,  wc = 2*pi*fc,  k = 4*reso.
// Discretised by forward Euler at the oversampled rate: all five
// tanh terms are evaluated at the current state, then all four
// stages advance — the literal Euler discretisation of the ODEs
// above. 5 tanh evaluations per substep (tanh(u - k*x4), tanh(x1),
// tanh(x2), tanh(x3), tanh(x4) — the middle three are reused by
// the next stage), 10 per host sample at 2x.
//
// What this filter is NOT:
//   - It is not a Butterworth. The linear (small-signal) prototype
//     is H(s) = 1 / ((1 + s/wc)^4 + k): four identical real poles
//     at wc with feedback k. At fc the gain is -12.04 dB for k = 0
//     (not -3 dB); the -3 dB point is at 0.644*fc. The test rig
//     (Tests/test_moogladder.cpp) measures against this prototype,
//     not against a Butterworth.
//   - It is not decramped, and it does not claim to be. Forward
//     Euler warps frequency the other way from BLT, and the error
//     is large: the Euler mapping s = fs_os*(e^{jw} - 1) has a
//     real part, so the evaluation point leaves the jw axis —
//     the digital ladder reads BRIGHTER than the prototype near
//     Nyquist, by +0.60 dB at f = fc for fc = 1 kHz, +5.93 dB
//     for fc = 8 kHz, +14.33 dB for fc = 16 kHz (measured, see
//     Tests/test_moogladder.cpp). Once alpha = 2*pi*fc/fs_os
//     exceeds 1 (fc ~= 14 kHz at 44.1 kHz) the stopband rises
//     above 0 dB — the ladder stops being a lowpass up there.
//     2x oversampling halves the error vs 1x; it does not remove
//     it. Decramping the ladder would mean a different
//     discretisation (Sarkkae-Huovilainen matches magnitude AND
//     phase, but it is a LINEAR discretisation — it cannot carry
//     the tanh in the loop, so it is not a companion for this
//     filter; see the step-3 report). 4x oversampling (DaisySP's
//     choice) moves the alpha = 1 threshold above Nyquist and
//     is the documented alternative, for Gary's call.
//   - 1x is the voice production mode (Gary's call, 2026-11-05):
//     the VoiceFilter runs this filter at 1x and clamps the
//     cutoff to sr/(2*pi) (alpha <= 1), so the production ladder
//     stays a lowpass and stays stable — at 44.1 kHz it tops out
//     at ~7.0 kHz. At 1x the filter is unstable at high fc
//     (alpha = 2*pi*fc/fs exceeds 2 above fc = fs/pi) and
//     aliases the tanh harmonics — the test measures both, and
//     the NMR verdict on the wired 1x path is in the voice
//     filter test (Tests/test_voicefilter.cpp). The standalone
//     default stays 2x (the published Huovilainen figure).
//   - It does not have a separate drive parameter. The input stage
//     IS the drive: the first tanh saturates on the input (minus
//     feedback). Input amplitude is the drive; a 0.1 sine stays
//     linear, a 1.0 sine saturates audibly.
//   - Resonance costs bass, as in the real ladder: DC gain is
//     1/(1+k) — -9.54 dB at reso = 0.5, -13.98 dB at reso = 1.0.
//     That is the circuit's behaviour, measured in the test, not
//     a defect to "fix" without a bass-compensation spec.
//
// Oversampling: 2x by default (the published Huovilainen figure).
// Upsampling is causal linear interpolation — the midpoint between
// the current and previous host sample — and decimation takes the
// second substep's output. The interpolator's fundamental response
// is cos^2(pi*f/(2*fs)) (-3.0 dB at 16 kHz at fs = 44.1 kHz) and
// its first image is only -10.7 dB down at 16 kHz, so the
// full-path response at high fc carries both effects — and above
// the alpha = 1 threshold (fc ~= 14 kHz) the ladder's stopband
// rises above 0 dB, so the image is amplified, not attenuated;
// the test measures the core directly at the oversampled rate
// (processOversampled) AND the full path (process) and reports
// both, so the two are not conflated. 1x exists only as a
// measurement comparison mode (setOversampling(1)): it is
// unstable at high fc (alpha = 2*pi*fc/fs exceeds 2 above
// fc = fs/pi) and aliases the tanh harmonics — the test measures
// both, and 1x is not a shipping mode.
//
// Clamping follows the SvfFilter conventions: cutoff 20 Hz ..
// min(20 kHz, Nyquist), resonance 0 .. 1, non-finite values
// ignored (the previous value is kept). State is double; I/O is
// float. Denormals are flushed, as in the SVF.

#pragma once

#include <cmath>

class MoogLadderFilter
{
public:
    static constexpr int kDefaultOversampling = 2;

    void prepare(double sampleRate);
    void setCutoff(float hz);
    void setResonance(float reso); // 0..1; 1 = self-oscillation (k = 4)
    // 1 or 2. 2 is the shipped mode (Huovilainen 2004, published).
    // 1 is the measurement comparison mode only (see header note).
    void setOversampling(int factor);
    void reset();
    float process(float x);

    // One Euler substep at the oversampled rate (2*sr by default),
    // no interpolation. Used by the test rig to measure the core
    // directly at its own rate, and by process() for the substeps.
    float processOversampled(float x);

    // Accessors, for tests.
    float getCutoff() const { return cutoff; }
    float getResonance() const { return reso; }
    int getOversampling() const { return osFactor; }
    double getOversampledRate() const { return sr * (double)osFactor; }

private:
    void updateAlpha();
    void updateK();

    double sr = 44100.0;
    int osFactor = kDefaultOversampling;

    // -1 means "never set": the first setCutoff()/setResonance()
    // always recomputes, so the filter never runs on a stale value.
    float cutoff = -1.0f;
    float reso = -1.0f;

    double alpha = 0.0; // 2*pi*fc / fs_os, the Euler step
    double k = 0.0;     // 4*reso, the feedback gain

    double x1 = 0.0, x2 = 0.0, x3 = 0.0, x4 = 0.0; // stage states
    float prevInput = 0.0f; // previous host sample, for the interpolator
};
