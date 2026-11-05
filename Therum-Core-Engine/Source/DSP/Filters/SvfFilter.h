// SvfFilter — the Simper state-variable filter (trapezoidal / TPT).
//
// The one 2-pole engine for Therum's voice filter (paper section 4.5).
// Simper 2013, "Solving the continuous SVF equations using trapezoidal
// integration and equivalent currents" (Cytomic). The same method as
// ProEQ8's SVF — same method, not the same filter.
//
// Why the SVF and not a Direct-Form biquad: Wishnick 2014 (DAFx-14)
// proves the trapezoidal SVF is time-varying BIBO stable under
// per-sample coefficient change; DF-II / TDF-II / lattice structures
// are not necessarily stable when coefficients move. That is the
// property a synth filter needs — audio-rate cutoff / resonance
// modulation, key tracking, envelope — with no transient-suppression
// or crossfade machinery. Smoothing is still good practice for zipper
// noise, but it is not required for stability.
//
// What the SVF does NOT do: decramp. The FreeEQ8 paper proved the SVF
// and the RBJ biquad produce identical steady-state magnitude responses
// — both use BLT prewarping (g = tan(pi*fc/fs)) and both cramp near
// Nyquist, with a -3.383 dB error remaining at the geometric mean of
// fc and Nyquist. The SVF's advantage is modulation stability, not
// decramping. Decramping is a tested hypothesis (see Tests/
// test_decramping.cpp); nothing in this file claims it.
//
// Verified output mixes only (Simper's core equations):
//   LP (0,0,1), HP (1,-k,-1), BP (0,1,0), Notch (1,-k,0).
// Bell / shelves / allpass are NOT in this class: Simper's PDF text
// extraction is garbled for those sections and the FreeEQ8 paper's
// shelf table carries a measured +50.96 dB defect at +24 dB / Q=24.
// They are measured in the test rig (test_decramping.cpp) before any
// of them ships in the runtime core. The setMix() escape hatch exists
// so the rig — and later PZSVF morphing, whose stability Simper
// p.32 proves — can drive the mix directly.
//
// Bandpass peak gain is Q (the constant-skirt-gain form): at fc the
// BP output is 20*log10(Q) dB — measured: Q=0.7071 -> -3.010 dB,
// Q=1 -> 0.000 dB, Q=2 -> +6.021 dB, Q=10 -> +20.000 dB. Callers
// wanting a 0 dB peak should scale the BP output by 1/Q (or use the
// RBJ constant-0-dB-peak-gain variant). The LP/HP are -3.01 dB at fc
// for Q = 1/sqrt(2) (Butterworth), and peak above fc as Q rises
// (Q=1: -0.008 dB, Q=2: +6.012 dB, Q=10: +19.992 dB at fc).
// do not mix the two. Cutoff is clamped to 20 Hz .. min(20 kHz,
// Nyquist); non-finite values are ignored (the previous cutoff is
// kept), matching the step-1 clamping rule. Q is clamped to
// 0.05 .. 100; non-finite values are ignored.
//
// Coefficients are recomputed only when the cutoff or Q actually
// changes (same pattern as the step-1 one-pole). The caller smooths
// the cutoff per sample (SmoothedParameter, 10 ms) and calls
// setCutoff() per sample, so the coefficients follow the smoothed
// value per sample — that is the per-sample interpolation the paper
// requires. During a sweep the tan() runs per sample; for a static
// cutoff it runs once.

#pragma once

#include <cmath>

class SvfFilter
{
public:
    enum class Type { LowPass, HighPass, BandPass, Notch };

    void prepare(double sampleRate);
    void setType(Type t);
    void setCutoff(float hz);
    void setQ(float q);
    // Pre-filter drive: 0 = bypass. Above 0 the input is blended
    // toward tanh saturation (fixed k = 8). This stage runs at 1x
    // with no oversampling or ADAA yet, so it aliases at high drive
    // — the oversampling infrastructure (paper section 4.6) addresses
    // that; the default is 0, so the voice path is unaffected.
    void setDrive(float drive);
    void reset();
    float process(float x);

    // Direct mix control (m0*v0 + m1*v1 + m2*v2). Used by the test
    // rig to measure unverified mixes (bell) and, later, by PZSVF
    // morphing. setType() overwrites the mix with the verified
    // table; call setMix() after setType() to override it. Once
    // overridden, setQ() no longer touches the mix (the HP/Notch
    // mixes contain k = 1/Q, so a naive setQ() would clobber a
    // custom mix); setType() clears the override.
    void setMix(double m0, double m1, double m2);

    // Accessors, for tests.
    float getCutoff() const { return cutoff; }
    float getQ() const { return q; }
    Type getType() const { return type; }

private:
    void updateCoefficients();
    void updateMix();

    double sr = 44100.0;
    Type type = Type::LowPass;

    // -1 means "never set": the first setCutoff()/setQ() always
    // recomputes the coefficients, so the filter never runs on a
    // stale initial value.
    float cutoff = -1.0f;
    float q = -1.0f;
    float drive = 0.0f;

    double g = 0.0;   // tan(pi*fc/fs), prewarped cutoff
    double k = 0.0;   // 1/Q, damping
    double a1 = 1.0;  // 1/(1+g*(g+k))
    double a2 = 0.0;  // g*a1
    double a3 = 0.0;  // g*a2

    double m0 = 0.0, m1 = 0.0, m2 = 1.0; // output mix (LP by default)
    bool mixOverridden = false; // setMix() was called after setType()

    double ic1eq = 0.0; // integrator state (bandpass)
    double ic2eq = 0.0; // integrator state (lowpass)
};
