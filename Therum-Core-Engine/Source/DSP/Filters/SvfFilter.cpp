#include "SvfFilter.h"

#include <algorithm>

void SvfFilter::prepare(double sampleRate)
{
    sr = sampleRate;
    cutoff = -1.0f; // force the coefficients to be recomputed on first use
    q = -1.0f;
    drive = 0.0f;
    mixOverridden = false;
    updateMix();
    reset();
}

void SvfFilter::reset()
{
    ic1eq = 0.0;
    ic2eq = 0.0;
}

void SvfFilter::setType(Type t)
{
    if (t == type)
        return;
    type = t;
    mixOverridden = false;
    updateMix();
}

void SvfFilter::setCutoff(float hz)
{
    // Ignore non-finite values: keep the previous cutoff rather than
    // letting a NaN or infinity poison the coefficient.
    if (!std::isfinite(hz))
        return;

    // Clamp to a sane range. An unclamped cutoff above Nyquist (or below
    // zero) makes g wrong and can push the filter unstable.
    const float nyquist = (float)sr * 0.5f;
    const float clamped = std::clamp(hz, 20.0f, std::min(20000.0f, nyquist));

    if (clamped == cutoff)
        return;

    cutoff = clamped;
    updateCoefficients();
}

void SvfFilter::setQ(float qIn)
{
    if (!std::isfinite(qIn))
        return;

    const float clamped = std::clamp(qIn, 0.05f, 100.0f);

    if (clamped == q)
        return;

    q = clamped;
    updateCoefficients(); // also refreshes the mix (HP/Notch use k)
}

void SvfFilter::setDrive(float d)
{
    if (!std::isfinite(d))
        return;
    drive = std::clamp(d, 0.0f, 1.0f);
}

void SvfFilter::setMix(double m0In, double m1In, double m2In)
{
    m0 = m0In;
    m1 = m1In;
    m2 = m2In;
    mixOverridden = true;
}

void SvfFilter::updateCoefficients()
{
    if (cutoff < 0.0f || q < 0.0f)
        return; // not both set yet; the first call that sets both runs this

    g = std::tan(3.14159265358979323846 * (double)cutoff / sr);
    k = 1.0 / (double)q;
    a1 = 1.0 / (1.0 + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
    // The HP and Notch mixes contain k = 1/Q, so the mix follows the
    // coefficients (unless a custom mix was set via setMix()). This
    // also covers setCutoff() being called before setQ(): the mix is
    // corrected as soon as both values exist.
    if (!mixOverridden)
        updateMix();
}

void SvfFilter::updateMix()
{
    // Verified mixes only (Simper's core equations). k is 1/Q here;
    // the HP and Notch mixes use it directly.
    switch (type)
    {
        case Type::LowPass:  m0 = 0.0; m1 = 0.0;  m2 = 1.0;  break;
        case Type::HighPass: m0 = 1.0; m1 = -k;   m2 = -1.0; break;
        case Type::BandPass: m0 = 0.0; m1 = 1.0;  m2 = 0.0;  break;
        case Type::Notch:    m0 = 1.0; m1 = -k;   m2 = 0.0;  break;
    }
}

float SvfFilter::process(float xIn)
{
    double x = (double)xIn;

    if (drive > 0.0f)
    {
        // Blend toward tanh saturation: at drive = 0 this is the
        // bypass (x), at drive = 1 it is tanh(8x)/8. The blend is
        // smooth in drive, so automating it does not click.
        const double d = (double)drive;
        x = (1.0 - d) * x + d * (std::tanh(8.0 * x) / 8.0);
    }

    const double v0 = x;
    const double v3 = v0 - ic2eq;
    const double v1 = a1 * ic1eq + a2 * v3;             // bandpass
    const double v2 = ic2eq + a2 * ic1eq + a3 * v3;     // lowpass
    ic1eq = 2.0 * v1 - ic1eq;
    ic2eq = 2.0 * v2 - ic2eq;

    // Flush subnormals: at very low cutoffs the integrator state can
    // decay into denormals, which cost orders of magnitude more CPU
    // on some hardware. Flushing to zero is inaudible.
    if (std::fabs(ic1eq) < 1e-30) ic1eq = 0.0;
    if (std::fabs(ic2eq) < 1e-30) ic2eq = 0.0;

    return (float)(m0 * v0 + m1 * v1 + m2 * v2);
}
