#pragma once
class GranularOscillator
{
public:
    void setGrainSizeMs(float x) { grainSizeMs = x; }
    void setDensity(float x) { density = x; }
    float process() { return 0.0f; } // placeholder

private:
    float grainSizeMs = 40.0f;
    float density = 0.5f;
};
