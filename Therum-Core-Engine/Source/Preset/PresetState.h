#pragma once
#include <JuceHeader.h>

struct PresetState
{
    juce::String name { "Init Patch" };
    juce::String category { "Init" };
    juce::ValueTree state { "PresetState" };
};
