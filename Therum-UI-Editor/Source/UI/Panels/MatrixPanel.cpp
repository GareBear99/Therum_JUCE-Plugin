#include "MatrixPanel.h"

void MatrixPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
    g.setColour(juce::Colour::fromRGB(139, 92, 246));
    g.drawText("ARC ROUTING TABLE", getLocalBounds().reduced(16), juce::Justification::topLeft);
}
