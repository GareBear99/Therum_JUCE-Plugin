#include "Fft.h"
#include <cmath>

namespace therum::fft
{

bool isPowerOfTwo(int n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

bool transform(std::vector<std::complex<float>>& io, bool inverse)
{
    const int n = (int)io.size();
    if (!isPowerOfTwo(n))
        return false;
    if (n == 1)
        return true;

    // Bit-reversal permutation.
    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        for (; (j & bit) != 0; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(io[(size_t)i], io[(size_t)j]);
    }

    // Iterative Cooley-Tukey butterflies.
    for (int len = 2; len <= n; len <<= 1)
    {
        const float ang = 2.0f * 3.14159265358979323846f / (float)len
                        * (inverse ? 1.0f : -1.0f);
        const std::complex<float> wlen(std::cos(ang), std::sin(ang));
        const int half = len >> 1;
        for (int i = 0; i < n; i += len)
        {
            std::complex<float> w(1.0f, 0.0f);
            for (int k = 0; k < half; ++k)
            {
                const std::complex<float> u = io[(size_t)(i + k)];
                const std::complex<float> v = io[(size_t)(i + k + half)] * w;
                io[(size_t)(i + k)] = u + v;
                io[(size_t)(i + k + half)] = u - v;
                w *= wlen;
            }
        }
    }

    if (inverse)
    {
        const float scale = 1.0f / (float)n;
        for (auto& x : io)
            x *= scale;
    }
    return true;
}

bool realForward(const float* time, int n, std::vector<std::complex<float>>& bins)
{
    if (!isPowerOfTwo(n) || time == nullptr)
        return false;
    std::vector<std::complex<float>> work((size_t)n);
    for (int i = 0; i < n; ++i)
        work[(size_t)i] = { time[i], 0.0f };
    if (!transform(work, false))
        return false;
    bins.assign(work.begin(), work.begin() + (n / 2 + 1));
    return true;
}

bool realInverse(const std::vector<std::complex<float>>& bins, float* time, int n)
{
    if (!isPowerOfTwo(n) || time == nullptr)
        return false;
    if ((int)bins.size() != n / 2 + 1)
        return false;
    std::vector<std::complex<float>> work((size_t)n, { 0.0f, 0.0f });
    // DC and Nyquist are real by construction; copy the rest and mirror
    // the negative frequencies as conjugates.
    work[0] = { bins[0].real(), 0.0f };
    for (int k = 1; k < n / 2; ++k)
    {
        work[(size_t)k] = bins[(size_t)k];
        work[(size_t)(n - k)] = std::conj(bins[(size_t)k]);
    }
    work[(size_t)(n / 2)] = { bins[(size_t)(n / 2)].real(), 0.0f };
    if (!transform(work, true))
        return false;
    for (int i = 0; i < n; ++i)
        time[i] = work[(size_t)i].real();
    return true;
}

} // namespace therum::fft
