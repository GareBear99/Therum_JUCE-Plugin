#include "WavetableOscillator.h"
#include <cmath>

void WavetableOscillator::prepare(double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    selectMip();
}

void WavetableOscillator::setFrequency(float hz)
{
    freq = hz;
    selectMip();
}

void WavetableOscillator::setPosition(float normalized)
{
    pos = normalized < 0.0f ? 0.0f : (normalized > 1.0f ? 1.0f : normalized);
}

void WavetableOscillator::setBank(const WavetableBank* wtBank)
{
    bank = wtBank;
    selectMip();
}

void WavetableOscillator::selectMip()
{
    mipLo = -1;
    mipHi = -1;
    mipAlpha = 0.0f;
    if (bank == nullptr || !bank->hasMipmaps())
        return;

    const auto& mips = bank->getFrameMips(0);
    const int levels = (int)mips.levels.size();
    if (levels == 0)
        return;

    // Highest harmonic that fits below Nyquist at this pitch.
    const double kAllowed = freq > 0.0f ? std::floor(sr / (2.0 * (double)freq))
                                        : (double)mips.levels[0].maxHarmonic;
    if (kAllowed >= (double)mips.levels[0].maxHarmonic)
    {
        mipLo = 0;
        mipHi = 0;
        return;
    }
    if (kAllowed <= (double)mips.levels[(size_t)(levels - 1)].maxHarmonic)
    {
        mipLo = levels - 1;
        mipHi = levels - 1;
        return;
    }
    // The two levels bracketing kAllowed: U is the smallest cap still
    // >= kAllowed (the brighter level), L the largest cap <= kAllowed
    // (the fully safe level). The blend moves from U to L in
    // log-frequency as the pitch rises: at kAllowed == U it is pure U,
    // at kAllowed == L it is pure L. The fading share of U's top
    // harmonics is the standard mipmap crossfade trade — a small,
    // fading alias contribution in exchange for no hard switching tick
    // where the harmonic count drops (paper section 4.1).
    int l = 0;
    for (int i = 0; i < levels; ++i)
    {
        if ((double)mips.levels[(size_t)i].maxHarmonic <= kAllowed)
        {
            l = i;
            break;
        }
    }
    const int u = l - 1;
    const double logU = std::log2((double)mips.levels[(size_t)u].maxHarmonic);
    const double logL = std::log2((double)mips.levels[(size_t)l].maxHarmonic);
    const double logK = std::log2(kAllowed);
    mipLo = u;
    mipHi = l;
    mipAlpha = (float)((logU - logK) / (logU - logL));
}

float WavetableOscillator::readHermite(const float* table, int size, float index)
{
    // 4-point Catmull-Rom spline. The table is periodic, so every tap
    // wraps; that is what makes the read click-free at the seam.
    const int i1 = (int)index % size;
    const int i0 = (i1 + size - 1) % size;
    const int i2 = (i1 + 1) % size;
    const int i3 = (i1 + 2) % size;
    const float f = index - (float)((int)index);
    const float f2 = f * f;
    const float f3 = f2 * f;
    const float p0 = table[i0];
    const float p1 = table[i1];
    const float p2 = table[i2];
    const float p3 = table[i3];
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * f
                   + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * f2
                   + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * f3);
}

float WavetableOscillator::readLinear(const float* table, int size, float index)
{
    const int i0 = (int)index % size;
    const int i1 = (i0 + 1) % size;
    const float f = index - (float)((int)index);
    return table[i0] + (table[i1] - table[i0]) * f;
}

float WavetableOscillator::process()
{
    const float out = [&]() -> float
    {
        if (bank == nullptr || !bank->isReady())
        {
            // No bank: direct sine, the scaffold fallback. The voice
            // always sets a bank; this only covers a miswired oscillator.
            return (float)std::sin(phase * 6.2831853071795864769);
        }

        const int frames = bank->getFrameCount();
        if (frames == 1)
        {
            if (mipLo >= 0)
            {
                const auto& mips = bank->getFrameMips(0);
                const auto& lo = mips.levels[(size_t)mipLo].table;
                const auto& hi = mips.levels[(size_t)mipHi].table;
                const float vLo = readTable(lo.data(), (int)lo.size(),
                                            (float)(phase * (double)lo.size()));
                const float vHi = readTable(hi.data(), (int)hi.size(),
                                            (float)(phase * (double)hi.size()));
                return vLo + mipAlpha * (vHi - vLo);
            }
            const auto& table = bank->getFrame(0);
            return readTable(table.data(), (int)table.size(),
                             (float)(phase * (double)table.size()));
        }

        // Frame position: fractional frame index across the bank.
        const float fpos = pos * (float)(frames - 1);
        int a = (int)fpos;
        if (a >= frames - 1)
            a = frames - 2;
        const float alpha = fpos - (float)a;

        float out;
        if (mipLo >= 0)
        {
            // Mipmaps exist: each frame is read from the two cached mip
            // levels (blended by mipAlpha), then the frames are
            // crossfaded by alpha. Four table reads per sample.
            const auto& mipsA = bank->getFrameMips(a);
            const auto& mipsB = bank->getFrameMips(a + 1);
            const auto& loA = mipsA.levels[(size_t)mipLo].table;
            const auto& hiA = mipsA.levels[(size_t)mipHi].table;
            const auto& loB = mipsB.levels[(size_t)mipLo].table;
            const auto& hiB = mipsB.levels[(size_t)mipHi].table;
            const float vLoA = readTable(loA.data(), (int)loA.size(),
                                         (float)(phase * (double)loA.size()));
            const float vHiA = readTable(hiA.data(), (int)hiA.size(),
                                         (float)(phase * (double)hiA.size()));
            const float vLoB = readTable(loB.data(), (int)loB.size(),
                                         (float)(phase * (double)loB.size()));
            const float vHiB = readTable(hiB.data(), (int)hiB.size(),
                                         (float)(phase * (double)hiB.size()));
            const float vA = vLoA + mipAlpha * (vHiA - vLoA);
            const float vB = vLoB + mipAlpha * (vHiB - vLoB);
            out = vA + alpha * (vB - vA);
        }
        else
        {
            // No mipmaps (bank never built them): read the base frames.
            const auto& tableA = bank->getFrame(a);
            const auto& tableB = bank->getFrame(a + 1);
            const float idxA = (float)(phase * (double)tableA.size());
            const float idxB = (float)(phase * (double)tableB.size());
            const float vA = readTable(tableA.data(), (int)tableA.size(), idxA);
            const float vB = readTable(tableB.data(), (int)tableB.size(), idxB);
            out = vA + alpha * (vB - vA);
        }
        // Per-sample crossfade (1-a)A + aB. It can sound phas-y when the
        // frames are out of phase; spectral morph (the quality mode) is
        // the answer to that, and it lands in piece 4 of step 2.
        return out;
    }();

    phase += (double)freq / sr;
    if (phase >= 1.0)
        phase -= 1.0;
    else if (phase < 0.0)
        phase += 1.0;
    return out;
}
