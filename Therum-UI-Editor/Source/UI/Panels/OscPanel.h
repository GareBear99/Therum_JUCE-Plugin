#pragma once
#include <JuceHeader.h>

class OscPanel : public juce::Component
{
public:
    OscPanel();

    juce::Slider& getWtPosSlider() { return wtPos; }
    juce::ComboBox& getMorphModeBox() { return morphModeBox; }
    juce::ComboBox& getMorphPhaseBox() { return morphPhaseBox; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label wtLabel;
    juce::Slider wtPos;
    juce::Label morphModeLabel;
    juce::ComboBox morphModeBox;
    juce::Label morphPhaseLabel;
    juce::ComboBox morphPhaseBox;
};
