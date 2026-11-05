#pragma once
// Small radix-2 FFT for the wavetable engine (mipmaps, spectral morph).
//
// Plain C++, stdlib only, no framework. All functions operate on
// power-of-two lengths; a non-power-of-two length is a programming error
// and is rejected (false / empty result), never silently miscomputed.
//
// Convention: forward() is unscaled; inverse() scales by 1/N, so
// inverse(forward(x)) == x. Bins follow the DFT layout: bin k holds the
// k-th harmonic, bins above N/2 are the negative frequencies.
#include <complex>
#include <vector>

namespace therum::fft
{

// In-place complex FFT. `inverse == false` is the forward transform,
// `inverse == true` is the inverse (scaled by 1/N). Length must be a
// power of two; returns false and leaves the data untouched otherwise.
bool transform(std::vector<std::complex<float>>& io, bool inverse);

// Real input -> first N/2+1 bins (DC .. Nyquist). Returns false for a
// non-power-of-two N.
bool realForward(const float* time, int n, std::vector<std::complex<float>>& bins);

// First N/2+1 bins -> real output (N samples). The bins must have been
// produced by realForward (or zeroed above some harmonic, which is how
// mipmaps are made). Returns false for a non-power-of-two N.
bool realInverse(const std::vector<std::complex<float>>& bins, float* time, int n);

// True when n is a power of two (and > 0).
bool isPowerOfTwo(int n);

} // namespace therum::fft
