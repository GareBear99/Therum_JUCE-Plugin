#pragma once
#include <JuceHeader.h>

class TopBarPanel : public juce::Component
{
public:
    TopBarPanel();

    juce::Slider& getMasterGainSlider() { return masterGain; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label subtitle;
    juce::Label gainLabel;
    juce::Slider masterGain;
};
