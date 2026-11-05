// Mipmap generation for the wavetable bank (paper section 4.1).
//
// At load, each base frame is FFT'd once; every mip level is that spectrum
// with the bins above the level's harmonic cap zeroed, IFFT'd back, and
// decimated to the level length. Decimation is exact here: the level
// length is a power of two that divides 2048, and the signal is
// band-limited to at most half the level length in harmonics, so taking
// every (2048 / L)-th sample loses nothing.
//
// The ladder is one level per 3 semitones (K *= 2^(-1/4) per step), from
// 1024 harmonics down to 1. A level's table length is the smallest power
// of two >= max(4, 2*K), so the tables shrink as they get more
// band-limited: the whole ladder costs ~8x the base frame (16,344 samples
// per frame), not 36x. The paper's "2x base" figure is for 1/octave
// levels; 1 per 3 semitones costs more and says so here, honestly.
#include "WavetableBank.h"
#include "../Utils/Fft.h"
#include <cmath>

namespace
{

// Harmonic caps, highest first, one per 3 semitones.
std::vector<int> buildLadder()
{
    std::vector<int> ladder;
    for (int m = 0; ; ++m)
    {
        const double k = 1024.0 * std::pow(2.0, -(double)m / 4.0);
        const int ki = (int)std::lround(k);
        if (ki <= 1)
        {
            ladder.push_back(1);
            break;
        }
        if (ladder.empty() || ladder.back() != ki)
            ladder.push_back(ki);
    }
    return ladder;
}

int levelLength(int maxHarmonic)
{
    int need = 2 * maxHarmonic;
    if (need < 4)
        need = 4;
    int len = 4;
    while (len < need)
        len <<= 1;
    return len;
}

} // namespace

bool WavetableBank::buildMipmaps()
{
    if (frames.empty())
        return false;

    static const std::vector<int> ladder = buildLadder();
    frameMips.assign(frames.size(), FrameMips());

    std::vector<std::complex<float>> bins;
    std::vector<float> full((size_t)kFrameSize);
    for (size_t f = 0; f < frames.size(); ++f)
    {
        if (!therum::fft::realForward(frames[f].data(), kFrameSize, bins))
            return false;
        FrameMips& fm = frameMips[f];
        fm.levels.reserve(ladder.size());
        for (int kMax : ladder)
        {
            // Zero every bin above the level's harmonic cap, then IFFT.
            std::vector<std::complex<float>> capped = bins;
            for (int k = kMax + 1; k <= kFrameSize / 2; ++k)
                capped[(size_t)k] = { 0.0f, 0.0f };
            if (!therum::fft::realInverse(capped, full.data(), kFrameSize))
                return false;
            const int len = levelLength(kMax);
            MipLevel level;
            level.maxHarmonic = kMax;
            level.table.resize((size_t)len);
            const int stride = kFrameSize / len;
            for (int i = 0; i < len; ++i)
                level.table[(size_t)i] = full[(size_t)(i * stride)];
            fm.levels.push_back(std::move(level));
        }
    }
    return true;
}

const WavetableBank::FrameMips& WavetableBank::getFrameMips(int index) const
{
    if (index < 0 || index >= (int)frameMips.size())
        return kEmptyMips;
    return frameMips[(size_t)index];
}

int WavetableBank::getMipLevelCount() const
{
    if (frameMips.empty())
        return 0;
    return (int)frameMips[0].levels.size();
}
