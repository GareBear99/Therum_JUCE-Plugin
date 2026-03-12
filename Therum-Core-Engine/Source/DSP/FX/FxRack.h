#pragma once
#include <algorithm>

class FxRack
{
public:
    void setDelayMix(float x) { delayMix = std::clamp(x, 0.0f, 1.0f); }
    void setReverbMix(float x) { reverbMix = std::clamp(x, 0.0f, 1.0f); }
    void setDistMix(float x) { distMix = std::clamp(x, 0.0f, 1.0f); }

    float process(float in)
    {
        const float driven = in * (1.0f + distMix * 2.0f);
        const float clipped = std::clamp(driven, -1.0f, 1.0f);
        const float wet = clipped * (1.0f - 0.25f * delayMix - 0.15f * reverbMix);
        return wet;
    }

private:
    float delayMix = 0.0f;
    float reverbMix = 0.0f;
    float distMix = 0.0f;
};
