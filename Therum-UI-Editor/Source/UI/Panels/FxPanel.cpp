#include "FxPanel.h"

FxPanel::FxPanel()
{
    title.setText("VOID CHAIN", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(255, 79, 216));
    title.setFont(juce::Font(18.0f));
    addAndMakeVisible(title);

    auto setup = [this](juce::Label& label, juce::Slider& slider, const juce::String& name, juce::Colour fill, juce::Colour thumb)
    {
        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
        slider.setColour(juce::Slider::rotarySliderFillColourId, fill);
        slider.setColour(juce::Slider::thumbColourId, thumb);
        addAndMakeVisible(slider);
    };

    setup(delayLabel, delayMix, "Delay", juce::Colour::fromRGB(255, 79, 216), juce::Colour::fromRGB(139, 92, 246));
    setup(reverbLabel, reverbMix, "Reverb", juce::Colour::fromRGB(139, 92, 246), juce::Colour::fromRGB(0, 246, 255));
    setup(distLabel, distMix, "Dist", juce::Colour::fromRGB(255, 140, 70), juce::Colour::fromRGB(255, 79, 216));
}

void FxPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
}

void FxPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    auto row = area.removeFromTop(160);
    for (auto* label : { &delayLabel, &reverbLabel, &distLabel })
    {
        auto cell = row.removeFromLeft(72);
        label->setBounds(cell.removeFromTop(18));
        if (label == &delayLabel) delayMix.setBounds(cell);
        else if (label == &reverbLabel) reverbMix.setBounds(cell);
        else distMix.setBounds(cell);
        row.removeFromLeft(8);
    }
}
