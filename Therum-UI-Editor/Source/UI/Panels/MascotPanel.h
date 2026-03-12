#pragma once
#include <JuceHeader.h>
#include "../../Mascot/MascotRuntime.h"

class MascotPanel : public juce::Component
{
public:
    MascotPanel();

    void setSpriteFile(const juce::File& file);
    void paint(juce::Graphics& g) override;

private:
    MascotRuntime runtime;
};
