#pragma once
class SpectralOscillator
{
public:
    void setBrightness(float x) { brightness = x; }
    void setTilt(float x) { tilt = x; }
    float process() { return 0.0f; } // placeholder

private:
    float brightness = 0.5f;
    float tilt = 0.0f;
};
