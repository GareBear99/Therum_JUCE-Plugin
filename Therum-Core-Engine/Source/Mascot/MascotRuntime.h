#pragma once
#include <JuceHeader.h>

class MascotRuntime
{
public:
    bool loadSpriteSheet(const juce::File& file)
    {
        sprite = juce::ImageFileFormat::loadFrom(file);
        return sprite.isValid();
    }

    const juce::Image& getSprite() const { return sprite; }
    bool isLoaded() const { return sprite.isValid(); }

private:
    juce::Image sprite;
};
