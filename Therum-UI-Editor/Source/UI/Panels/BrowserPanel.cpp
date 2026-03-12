#include "BrowserPanel.h"

BrowserPanel::BrowserPanel()
{
    title.setText("PRESET SHARD VAULT", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(139, 92, 246));
    title.setFont(juce::Font(18.0f));
    addAndMakeVisible(title);

    listBox.setModel(&model);
    listBox.setRowHeight(28);
    addAndMakeVisible(listBox);
}

void BrowserPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
}

void BrowserPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    listBox.setBounds(area);
}
