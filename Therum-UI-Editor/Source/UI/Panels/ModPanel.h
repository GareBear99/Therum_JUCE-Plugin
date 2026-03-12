#pragma once
#include <JuceHeader.h>

class ModPanel : public juce::Component
{
public:
    ModPanel();

    juce::Slider& getMacro1Slider() { return macro1; }
    juce::Slider& getMacro2Slider() { return macro2; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label macro1Label;
    juce::Label macro2Label;
    juce::Slider macro1;
    juce::Slider macro2;
};
