#pragma once
#include <JuceHeader.h>

namespace PresetIO
{
    bool saveStateToFile(const juce::ValueTree& state, const juce::File& file);
    bool loadStateFromFile(juce::ValueTree& state, const juce::File& file);
}
