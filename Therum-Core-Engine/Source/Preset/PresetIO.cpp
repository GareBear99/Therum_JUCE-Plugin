#include "PresetIO.h"

namespace PresetIO
{
    bool saveStateToFile(const juce::ValueTree& state, const juce::File& file)
    {
        if (auto xml = state.createXml())
            return xml->writeTo(file);
        return false;
    }

    bool loadStateFromFile(juce::ValueTree& state, const juce::File& file)
    {
        if (!file.existsAsFile())
            return false;

        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml == nullptr)
            return false;

        state = juce::ValueTree::fromXml(*xml);
        return state.isValid();
    }
}
