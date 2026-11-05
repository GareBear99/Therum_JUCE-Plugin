// RbjBiquad — the RBJ Audio EQ Cookbook biquad, test-only reference.
//
// This file is NOT part of the DSP core. It exists so the tests can
// verify the Simper SVF against the RBJ biquad on the same rig — the
// FreeEQ8 paper's result is that the two produce identical
// steady-state magnitude responses (both use BLT prewarping), and
// Therum's SVF must reproduce that. If the SVF ever diverges from
// this reference, the SVF is wrong (or the test is), not the RBJ.
//
// Coefficients from the cookbook (W3C mirror), normalised by a0:
//   w0 = 2*pi*f0/fs, alpha = sin(w0)/(2*Q), cosw0 = cos(w0)
//   LPF: b = ((1-cosw0)/2, 1-cosw0, (1-cosw0)/2)
//   HPF: b = ((1+cosw0)/2, -(1+cosw0), (1+cosw0)/2)
//   BPF (constant skirt gain, peak = Q): b = (Q*alpha, 0, -Q*alpha)
//       where Q*alpha = sin(w0)/2. (alpha alone is the constant-0-dB-
//       peak variant, peak = 1 — do not confuse the two.)
//   Notch: b = (1, -2*cosw0, 1)
//   all: a = (1+alpha, -2*cosw0, 1-alpha)
// The BPF here is the constant-skirt-gain form (peak = Q) because
// that is the form Simper's v1 output matches (measured: BP peak =
// 20*log10(Q) dB). The constant-0-dB-peak variant (b0 = Q*alpha) is
// a different filter and must not be compared against the SVF BP.
#pragma once

#include <cmath>

class RbjBiquad
{
public:
    enum class Type { LowPass, HighPass, BandPass, Notch, Bell };

    // Bell (peaking) needs a gain; use setBell() for it.
    void setBell(double sampleRate, double fc, double q, double gainDb)
    {
        const double w0 = 2.0 * 3.14159265358979323846 * fc / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * q);
        const double cosw0 = std::cos(w0);
        const double A = std::pow(10.0, gainDb / 40.0);
        const double a0 = 1.0 + alpha / A;
        const double a1 = -2.0 * cosw0;
        const double a2 = 1.0 - alpha / A;
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cosw0;
        b2 = 1.0 - alpha * A;
        b0 /= a0; b1 /= a0; b2 /= a0;
        na1 = a1 / a0; na2 = a2 / a0;
        x1 = x2 = y1 = y2 = 0.0;
    }

    void set(Type t, double sampleRate, double fc, double q)
    {
        const double w0 = 2.0 * 3.14159265358979323846 * fc / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * q);
        const double cosw0 = std::cos(w0);
        const double a0 = 1.0 + alpha;
        const double a1 = -2.0 * cosw0;
        const double a2 = 1.0 - alpha;

        switch (t)
        {
            case Type::LowPass:
                b0 = (1.0 - cosw0) / 2.0; b1 = 1.0 - cosw0; b2 = (1.0 - cosw0) / 2.0;
                break;
            case Type::HighPass:
                b0 = (1.0 + cosw0) / 2.0; b1 = -(1.0 + cosw0); b2 = (1.0 + cosw0) / 2.0;
                break;
            case Type::BandPass: // constant skirt gain: peak = Q
                // b0 = sin(w0)/2 = Q*alpha (NOT alpha — alpha gives
                // the constant-0-dB-peak variant, peak = 1).
                b0 = q * alpha; b1 = 0.0; b2 = -q * alpha;
                break;
            case Type::Notch:
                b0 = 1.0; b1 = -2.0 * cosw0; b2 = 1.0;
                break;
            case Type::Bell:
                // set() has no gain parameter: this is the 0 dB bell
                // (A = 1), i.e. a pass-through. Use setBell() for a
                // gain. It is here only so the switch is exhaustive.
                b0 = 1.0 + alpha; b1 = -2.0 * cosw0; b2 = 1.0 - alpha;
                break;
        }

        // Normalise by a0.
        b0 /= a0; b1 /= a0; b2 /= a0;
        na1 = a1 / a0; na2 = a2 / a0;
        x1 = x2 = y1 = y2 = 0.0;
    }

    void reset() { x1 = x2 = y1 = y2 = 0.0; }

    double process(double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - na1 * y1 - na2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }

private:
    double b0 = 0.0, b1 = 0.0, b2 = 0.0, na1 = 0.0, na2 = 0.0;
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
};
