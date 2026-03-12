#include "OscPanel.h"

OscPanel::OscPanel()
{
    title.setText("DRAWN WAVE LAB", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(0, 246, 255));
    title.setFont(juce::Font(18.0f));
    addAndMakeVisible(title);

    wtLabel.setText("WT Pos", juce::dontSendNotification);
    wtLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(wtLabel);

    wtPos.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    wtPos.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
    wtPos.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(0, 246, 255));
    wtPos.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(255, 79, 216));
    addAndMakeVisible(wtPos);
}

void OscPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
}

void OscPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    wtLabel.setBounds(area.removeFromTop(18));
    wtPos.setBounds(area.removeFromTop(140).removeFromLeft(120));
}
