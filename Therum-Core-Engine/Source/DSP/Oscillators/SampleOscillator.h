#pragma once
#include <vector>

class SampleOscillator
{
public:
    void setSampleData(const std::vector<float>* newData) { data = newData; }
    void setPlaybackRate(float x) { rate = x; }
    void setPosition(float x) { pos = x; }
    float process();

private:
    const std::vector<float>* data = nullptr;
    float pos = 0.0f;
    float rate = 1.0f;
};
