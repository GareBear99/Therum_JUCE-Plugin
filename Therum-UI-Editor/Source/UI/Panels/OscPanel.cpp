#include "OscPanel.h"
#include "DSP/Oscillators/MorphChoice.h"

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

    morphModeLabel.setText("Morph", juce::dontSendNotification);
    morphModeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(morphModeLabel);

    morphPhaseLabel.setText("Phase", juce::dontSendNotification);
    morphPhaseLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(morphPhaseLabel);

    // The morph mode / phase selectors for osc A. Items are the shared
    // TherumMorphChoice labels, in choice order, with item IDs 1..N —
    // the ComboBoxAttachment convention (item ID = choice index + 1),
    // so the boxes write the same indices the processor maps. The
    // phase choice is inert in Crossfade mode; Spectral is the quality
    // mode (paper section 4.1). Osc B has the same parameters but no
    // panel yet, like its WT Pos slider.
    juce::StringArray morphModeChoices;
    for (const char* label : TherumMorphChoice::modeLabels)
        morphModeChoices.add(label);
    morphModeBox.addItemList(morphModeChoices, 1);
    juce::StringArray morphPhaseChoices;
    for (const char* label : TherumMorphChoice::phaseLabels)
        morphPhaseChoices.add(label);
    morphPhaseBox.addItemList(morphPhaseChoices, 1);
    for (auto* b : { &morphModeBox, &morphPhaseBox })
    {
        b->setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(20, 26, 48));
        b->setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(0, 246, 255));
        b->setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.92f));
        b->setColour(juce::ComboBox::arrowColourId, juce::Colour::fromRGB(0, 246, 255));
        addAndMakeVisible(*b);
    }
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
    area.removeFromTop(6);
    wtLabel.setBounds(area.removeFromTop(18));
    // The knob shrank 140 -> 110 to make room for the two selector
    // rows below, the same trade the FilterPanel made for its type
    // selector (knob row 160 -> 128). Total: 24 + 6 + 18 + 110 + 6
    // + 18 + 26 = 208 of the 214 available in the 258 px hero slot.
    wtPos.setBounds(area.removeFromTop(110).removeFromLeft(120));
    area.removeFromTop(6);
    auto labelRow = area.removeFromTop(18);
    auto leftLabel = labelRow.removeFromLeft((labelRow.getWidth() - 10) / 2);
    morphModeLabel.setBounds(leftLabel);
    labelRow.removeFromLeft(10);
    morphPhaseLabel.setBounds(labelRow);
    auto boxRow = area.removeFromTop(26);
    auto leftBox = boxRow.removeFromLeft((boxRow.getWidth() - 10) / 2);
    morphModeBox.setBounds(leftBox);
    boxRow.removeFromLeft(10);
    morphPhaseBox.setBounds(boxRow);
}
