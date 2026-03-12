#pragma once
#include <JuceHeader.h>

class SplashPanel : public juce::Component
{
public:
    void setAlphaAmount(float a) { alphaAmount = a; repaint(); }
    void paint(juce::Graphics& g) override;

private:
    float alphaAmount = 1.0f;
};
