#include "MascotPanel.h"

MascotPanel::MascotPanel() {}

void MascotPanel::setSpriteFile(const juce::File& file)
{
    runtime.loadSpriteSheet(file);
    repaint();
}

void MascotPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);

    auto area = getLocalBounds().reduced(12);
    g.setColour(juce::Colour::fromRGB(255, 140, 70));
    g.setFont(18.0f);
    g.drawText("PUNCH REACTOR", area.removeFromTop(24), juce::Justification::left);

    area.removeFromTop(8);

    if (runtime.isLoaded())
    {
        g.drawImageWithin(runtime.getSprite(), area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawFittedText("Mascot sprite runtime\nplaceholder", area, juce::Justification::centred, 2);
    }
}
