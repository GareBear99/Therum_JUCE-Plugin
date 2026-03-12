#pragma once
#include <JuceHeader.h>

class BootBarView : public juce::Component
{
public:
    void setProgress(float p) { progress = juce::jlimit(0.0f, 1.0f, p); repaint(); }
    void paint(juce::Graphics& g) override;

private:
    float progress = 0.0f;
};
