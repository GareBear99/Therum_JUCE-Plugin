#include "SplashPanel.h"

void SplashPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.86f * alphaAmount));
    g.setColour(juce::Colour::fromRGBA(0, 246, 255, (juce::uint8)(255 * alphaAmount)));
    g.drawFittedText("THERUM PRE-BOOT", getLocalBounds(), juce::Justification::centred, 1);
}
