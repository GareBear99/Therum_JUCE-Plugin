#include "BootBarView.h"

void BootBarView::paint(juce::Graphics& g)
{
    auto bar = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.fillRoundedRectangle(bar, 5.0f);

    g.setColour(juce::Colour::fromRGB(0, 246, 255));
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * progress), 5.0f);
}
