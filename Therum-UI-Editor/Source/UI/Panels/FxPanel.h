#pragma once
#include <JuceHeader.h>

class FxPanel : public juce::Component
{
public:
    FxPanel();

    juce::Slider& getDelayMixSlider() { return delayMix; }
    juce::Slider& getReverbMixSlider() { return reverbMix; }
    juce::Slider& getDistMixSlider() { return distMix; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label delayLabel;
    juce::Label reverbLabel;
    juce::Label distLabel;
    juce::Slider delayMix;
    juce::Slider reverbMix;
    juce::Slider distMix;
};
