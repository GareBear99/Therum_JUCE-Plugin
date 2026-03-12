#pragma once
#include <JuceHeader.h>
class ScopeView : public juce::Component
{
public:
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
};
