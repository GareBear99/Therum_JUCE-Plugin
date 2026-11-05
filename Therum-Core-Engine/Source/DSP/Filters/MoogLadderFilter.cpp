#include "MoogLadderFilter.h"

#include <algorithm>

void MoogLadderFilter::prepare(double sampleRate)
{
    sr = sampleRate;
    osFactor = kDefaultOversampling;
    cutoff = -1.0f; // force the coefficients to be recomputed on first use
    reso = -1.0f;
    updateK();
    reset();
}

void MoogLadderFilter::reset()
{
    x1 = 0.0;
    x2 = 0.0;
    x3 = 0.0;
    x4 = 0.0;
    prevInput = 0.0f;
}

void MoogLadderFilter::setCutoff(float hz)
{
    // Ignore non-finite values: keep the previous cutoff rather than
    // letting a NaN or infinity poison the coefficient.
    if (!std::isfinite(hz))
        return;

    // Clamp to a sane range, same rule as the SVF. An unclamped
    // cutoff above Nyquist makes alpha > pi and the Euler update
    // unstable even at 2x.
    const float nyquist = (float)sr * 0.5f;
    const float clamped = std::clamp(hz, 20.0f, std::min(20000.0f, nyquist));

    if (clamped == cutoff)
        return;

    cutoff = clamped;
    updateAlpha();
}

void MoogLadderFilter::setResonance(float resoIn)
{
    if (!std::isfinite(resoIn))
        return;

    const float clamped = std::clamp(resoIn, 0.0f, 1.0f);

    if (clamped == reso)
        return;

    reso = clamped;
    updateK();
}

void MoogLadderFilter::setOversampling(int factor)
{
    // Only 1 and 2 are supported: 2 is the shipped mode, 1 is the
    // measurement comparison mode. Anything else clamps to the
    // nearest supported value (<= 1 -> 1, >= 2 -> 2).
    const int clamped = factor <= 1 ? 1 : 2;

    if (clamped == osFactor)
        return;

    osFactor = clamped;
    updateAlpha(); // alpha is 2*pi*fc / (sr * osFactor)
}

void MoogLadderFilter::updateAlpha()
{
    if (cutoff < 0.0f)
        return; // not set yet; the first setCutoff() runs this
    alpha = 2.0 * 3.14159265358979323846 * (double)cutoff / (sr * (double)osFactor);
}

void MoogLadderFilter::updateK()
{
    if (reso < 0.0f)
        return; // not set yet; the first setResonance() runs this
    k = 4.0 * (double)reso;
}

float MoogLadderFilter::processOversampled(float xIn)
{
    const double u = (double)xIn;

    // Forward-Euler update: all five tanh terms are evaluated at
    // the current state, then all four stages advance. This is the
    // literal Euler discretisation of the ODEs in the header.
    const double t0 = std::tanh(u - k * x4);
    const double t1 = std::tanh(x1);
    x1 += alpha * (t0 - t1);
    const double t2 = std::tanh(x2);
    x2 += alpha * (t1 - t2);
    const double t3 = std::tanh(x3);
    x3 += alpha * (t2 - t3);
    const double t4 = std::tanh(x4);
    x4 += alpha * (t3 - t4);

    // Flush subnormals: at very low cutoffs the stage states can
    // decay into denormals, which cost orders of magnitude more CPU
    // on some hardware. Flushing to zero is inaudible.
    if (std::fabs(x1) < 1e-30) x1 = 0.0;
    if (std::fabs(x2) < 1e-30) x2 = 0.0;
    if (std::fabs(x3) < 1e-30) x3 = 0.0;
    if (std::fabs(x4) < 1e-30) x4 = 0.0;

    return (float)x4;
}

float MoogLadderFilter::process(float xIn)
{
    if (osFactor == 1)
        return processOversampled(xIn);

    // 2x: causal linear-interpolation upsampling. The oversampled
    // grid is x[n] at the even substep and the midpoint between
    // the current and previous host sample at the odd substep
    // (half a substep of delay — magnitude only, no effect on the
    // measured response). Decimation takes the second substep's
    // output, i.e. the substep aligned with the host sample.
    const double mid = 0.5 * ((double)xIn + (double)prevInput);
    processOversampled((float)mid);
    const float out = processOversampled(xIn);
    prevInput = xIn;
    return out;
}
