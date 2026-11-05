#include "WavetableBank.h"
#include <cmath>

const std::vector<float> WavetableBank::kEmptyTable;
const WavetableBank::FrameMips WavetableBank::kEmptyMips;

namespace
{
constexpr float kTwoPi = 6.28318530718f;

float sineAt(float phase) { return std::sin(phase * kTwoPi); }

float triangleAt(float phase)
{
    // asin(sin) triangle, -1..1, zero at phase 0.
    return (2.0f / 3.14159265358979323846f) * std::asin(sineAt(phase));
}

float sawAt(float phase)
{
    // Naive ramp, -1..1. The base frame holds all 1024 harmonics; the
    // mipmaps (not the frame) do the band-limiting.
    return 2.0f * phase - 1.0f;
}

float squareAt(float phase)
{
    return sineAt(phase) >= 0.0f ? 1.0f : -1.0f;
}

} // namespace

void WavetableBank::resampleToFrame(const float* samples, int length,
                                    std::vector<float>& out)
{
    out.resize((size_t)kFrameSize);
    if (length <= 0 || samples == nullptr)
        return;
    if (length == kFrameSize)
    {
        for (int i = 0; i < kFrameSize; ++i)
            out[(size_t)i] = samples[i];
        return;
    }
    // Linear resample of one periodic cycle. The source is treated as
    // wrapping, so the last sample interpolates back to the first.
    for (int i = 0; i < kFrameSize; ++i)
    {
        const float pos = (float)i / (float)kFrameSize * (float)length;
        const int i0 = (int)pos % length;
        const int i1 = (i0 + 1) % length;
        const float frac = pos - (float)((int)pos);
        out[(size_t)i] = samples[i0] + (samples[i1] - samples[i0]) * frac;
    }
}

void WavetableBank::buildDefaultSineTable(int size)
{
    frames.clear();
    frameMips.clear();
    std::vector<float> sine((size_t)(size > 0 ? size : kFrameSize));
    for (int i = 0; i < (int)sine.size(); ++i)
        sine[(size_t)i] = sineAt((float)i / (float)sine.size());
    std::vector<float> frame;
    resampleToFrame(sine.data(), (int)sine.size(), frame);
    frames.push_back(std::move(frame));
}

void WavetableBank::buildDefaultBank()
{
    frames.clear();
    frameMips.clear();
    auto makeFrame = [this](float (*fn)(float))
    {
        std::vector<float> frame((size_t)kFrameSize);
        for (int i = 0; i < kFrameSize; ++i)
            frame[(size_t)i] = fn((float)i / (float)kFrameSize);
        frames.push_back(std::move(frame));
    };
    makeFrame(sineAt);
    makeFrame(triangleAt);
    makeFrame(sawAt);
    makeFrame(squareAt);
}

bool WavetableBank::addFrame(const float* samples, int length)
{
    if (samples == nullptr || length <= 0)
        return false;
    if ((int)frames.size() >= kMaxFrames)
        return false;
    std::vector<float> frame;
    resampleToFrame(samples, length, frame);
    frames.push_back(std::move(frame));
    frameMips.clear(); // the new frame has no mipmaps yet
    return true;
}

const std::vector<float>& WavetableBank::getFrame(int index) const
{
    if (index < 0 || index >= (int)frames.size())
        return kEmptyTable;
    return frames[(size_t)index];
}

// Mipmap generation lives in WavetableBankMips.cpp (piece 3 of step 2).
