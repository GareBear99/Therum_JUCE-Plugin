#pragma once
class LfoGenerator
{
public:
    void prepare(double sampleRate) { sr = sampleRate; }
    void setRate(float hz) { rate = hz; }
    float process();

private:
    double sr = 44100.0;
    float rate = 2.0f;
    float phase = 0.0f;
};
