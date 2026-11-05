#include "WavetableOscillator.h"
#include <cmath>

void WavetableOscillator::prepare(double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
}

void WavetableOscillator::setFrequency(float hz)
{
    freq = hz;
}

void WavetableOscillator::setPosition(float normalized)
{
    pos = normalized < 0.0f ? 0.0f : (normalized > 1.0f ? 1.0f : normalized);
}

void WavetableOscillator::setBank(const WavetableBank* wtBank)
{
    bank = wtBank;
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
        const auto& tableA = bank->getFrame(a);
        const auto& tableB = bank->getFrame(a + 1);
        const float idxA = (float)(phase * (double)tableA.size());
        const float idxB = (float)(phase * (double)tableB.size());
        const float va = readTable(tableA.data(), (int)tableA.size(), idxA);
        const float vb = readTable(tableB.data(), (int)tableB.size(), idxB);
        // Per-sample crossfade (1-a)A + aB. It can sound phas-y when the
        // frames are out of phase; spectral morph (the quality mode) is
        // the answer to that, and it lands in piece 4 of step 2.
        return va + (vb - va) * alpha;
    }();

    phase += (double)freq / sr;
    if (phase >= 1.0)
        phase -= 1.0;
    else if (phase < 0.0)
        phase += 1.0;
    return out;
}
