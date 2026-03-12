#pragma once
class NoiseOscillator
{
public:
    float process();
private:
    unsigned int state = 22222u;
};
