#include "FilterPanel.h"
#include "../../DSP/Filters/FilterTypeChoice.h"

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

    typeLabel.setText("Type", juce::dontSendNotification);
    typeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(typeLabel);

    // The filter type selector. Items are the shared FilterTypeChoice
    // labels, in choice order, with item IDs 1..N — the
    // ComboBoxAttachment convention (item ID = choice index + 1),
    // so the box writes the same index the processor maps.
    juce::StringArray typeChoices;
    for (const char* label : TherumFilterChoice::labels)
        typeChoices.add(label);
    typeBox.addItemList(typeChoices, 1);
    typeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(20, 26, 48));
    typeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(255, 79, 216));
    typeBox.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.92f));
    typeBox.setColour(juce::ComboBox::arrowColourId, juce::Colour::fromRGB(255, 79, 216));
    addAndMakeVisible(typeBox);

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
    area.removeFromTop(6);

    auto row = area.removeFromTop(128);
    auto left = row.removeFromLeft(120);
    cutoffLabel.setBounds(left.removeFromTop(18));
    cutoff.setBounds(left);

    row.removeFromLeft(10);
    auto right = row.removeFromLeft(120);
    resoLabel.setBounds(right.removeFromTop(18));
    reso.setBounds(right);

    area.removeFromTop(6);
    typeLabel.setBounds(area.removeFromTop(18));
    typeBox.setBounds(area.removeFromTop(26));
}
