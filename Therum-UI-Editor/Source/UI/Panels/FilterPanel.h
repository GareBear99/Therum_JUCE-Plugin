#pragma once
#include <JuceHeader.h>

class FilterPanel : public juce::Component
{
public:
    FilterPanel();

    juce::Slider& getCutoffSlider() { return cutoff; }
    juce::Slider& getResoSlider() { return reso; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::Label cutoffLabel;
    juce::Label resoLabel;
    juce::Slider cutoff;
    juce::Slider reso;
};
