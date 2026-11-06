#pragma once
#include <JuceHeader.h>

class ModPanel : public juce::Component
{
public:
    ModPanel();

    juce::Slider& getMacro1Slider() { return macro1; }
    juce::Slider& getMacro2Slider() { return macro2; }
    juce::Slider& getAttackSlider() { return attack; }
    juce::Slider& getDecaySlider() { return decay; }
    juce::Slider& getSustainSlider() { return sustain; }
    juce::Slider& getReleaseSlider() { return release; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label macro1Label;
    juce::Label macro2Label;
    juce::Slider macro1;
    juce::Slider macro2;
    juce::Label envLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Slider attack;
    juce::Slider decay;
    juce::Slider sustain;
    juce::Slider release;
};
