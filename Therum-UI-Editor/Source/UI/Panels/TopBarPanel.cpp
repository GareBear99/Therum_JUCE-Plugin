#include "TopBarPanel.h"

TopBarPanel::TopBarPanel()
{
    title.setText("THERUM", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(0, 246, 255));
    title.setFont(juce::Font(28.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("Flagship Hybrid Synth Platform", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
    addAndMakeVisible(subtitle);

    gainLabel.setText("Master", juce::dontSendNotification);
    gainLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(gainLabel);

    masterGain.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
    masterGain.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(0, 246, 255));
    masterGain.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(255, 79, 216));
    addAndMakeVisible(masterGain);
}

void TopBarPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(10, 14, 28));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 16.0f);
}

void TopBarPanel::resized()
{
    auto area = getLocalBounds().reduced(14, 8);
    auto left = area.removeFromLeft(380);
    title.setBounds(left.removeFromTop(34));
    subtitle.setBounds(left.removeFromTop(20));

    auto right = area.removeFromRight(110);
    gainLabel.setBounds(right.removeFromTop(18));
    masterGain.setBounds(right.removeFromTop(56));
}
