#include "FilterPanel.h"

FilterPanel::FilterPanel()
{
    title.setText("NEON CATHEDRAL", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(255, 79, 216));
    title.setFont(juce::Font(18.0f));
    addAndMakeVisible(title);

    cutoffLabel.setText("Cutoff", juce::dontSendNotification);
    cutoffLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(cutoffLabel);

    resoLabel.setText("Reso", juce::dontSendNotification);
    resoLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(resoLabel);

    for (auto* s : { &cutoff, &reso })
    {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        s->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(255, 79, 216));
        s->setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(255, 140, 70));
        addAndMakeVisible(*s);
    }
}

void FilterPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
}

void FilterPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    auto row = area.removeFromTop(160);
    auto left = row.removeFromLeft(120);
    cutoffLabel.setBounds(left.removeFromTop(18));
    cutoff.setBounds(left);

    row.removeFromLeft(10);
    auto right = row.removeFromLeft(120);
    resoLabel.setBounds(right.removeFromTop(18));
    reso.setBounds(right);
}
