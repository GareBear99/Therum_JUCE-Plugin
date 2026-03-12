#include "ModPanel.h"

ModPanel::ModPanel()
{
    title.setText("LFO / ENV ARC", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(139, 92, 246));
    title.setFont(juce::Font(18.0f));
    addAndMakeVisible(title);

    auto setup = [this](juce::Label& label, juce::Slider& slider, const juce::String& name)
    {
        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
        slider.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(139, 92, 246));
        slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(255, 79, 216));
        addAndMakeVisible(slider);
    };

    setup(macro1Label, macro1, "Macro 1");
    setup(macro2Label, macro2, "Macro 2");
}

void ModPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
}

void ModPanel::resized()
{
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    auto row = area.removeFromTop(190);
    auto a = row.removeFromLeft(56);
    macro1Label.setBounds(a.removeFromTop(18));
    macro1.setBounds(a);

    row.removeFromLeft(12);
    auto b = row.removeFromLeft(56);
    macro2Label.setBounds(b.removeFromTop(18));
    macro2.setBounds(b);
}
