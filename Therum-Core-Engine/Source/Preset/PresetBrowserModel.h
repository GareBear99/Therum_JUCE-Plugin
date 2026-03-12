#pragma once
#include <vector>
#include "PresetState.h"

class PresetBrowserModel
{
public:
    void addPreset(const PresetState& p) { presets.push_back(p); }
    const std::vector<PresetState>& getPresets() const { return presets; }

    std::vector<PresetState> findByCategory(const juce::String& category) const
    {
        std::vector<PresetState> out;
        for (const auto& p : presets)
            if (p.category == category)
                out.push_back(p);
        return out;
    }

private:
    std::vector<PresetState> presets;
};
