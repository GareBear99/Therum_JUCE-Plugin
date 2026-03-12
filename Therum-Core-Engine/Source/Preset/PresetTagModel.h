#pragma once
#include <JuceHeader.h>
#include <vector>

struct PresetTagEntry
{
    juce::String name;
    juce::String category;
    juce::StringArray tags;
};

class PresetTagModel
{
public:
    void add(PresetTagEntry e) { entries.push_back(std::move(e)); }
    const std::vector<PresetTagEntry>& getAll() const { return entries; }

private:
    std::vector<PresetTagEntry> entries;
};
