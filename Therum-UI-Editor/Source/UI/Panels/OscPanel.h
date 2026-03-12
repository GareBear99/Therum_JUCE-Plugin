#pragma once
#include <JuceHeader.h>

class OscPanel : public juce::Component
{
public:
    OscPanel();

    juce::Slider& getWtPosSlider() { return wtPos; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label wtLabel;
    juce::Slider wtPos;
};
