#pragma once
#include <JuceHeader.h>
#include "PresetTagModel.h"
#include <vector>

class PresetBrowserFilter
{
public:
    std::vector<PresetTagEntry> filter(const std::vector<PresetTagEntry>& entries,
                                       const juce::String& category,
                                       const juce::String& text) const
    {
        std::vector<PresetTagEntry> out;
        for (const auto& e : entries)
        {
            const bool categoryOk = category.isEmpty() || e.category == category;
            const bool textOk = text.isEmpty()
                || e.name.containsIgnoreCase(text)
                || e.category.containsIgnoreCase(text)
                || e.tags.joinIntoString(" ").containsIgnoreCase(text);

            if (categoryOk && textOk)
                out.push_back(e);
        }
        return out;
    }
};
