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

    // The amp-envelope ADSR. These parameters existed in the layout
    // but had no panel and never reached audio (the voice hardcoded
    // 10/120/0.75/220 ms); the sliders drive the same env1_* values
    // the processor now passes to the voice per block.
    envLabel.setText("Env 1", juce::dontSendNotification);
    envLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(envLabel);
    setup(attackLabel, attack, "Attack");
    setup(decayLabel, decay, "Decay");
    setup(sustainLabel, sustain, "Sustain");
    setup(releaseLabel, release, "Release");
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

    // The ADSR row below the macros: 4 sliders of 56 px + 3 gaps of
    // 12 px = 260 px of the ~277 px usable width; total height
    // 24 + 8 + 190 + 12 + 18 + 190 = 442 of the ~478 px usable in
    // the lower row.
    area.removeFromTop(12);
    envLabel.setBounds(area.removeFromTop(18));
    auto envRow = area.removeFromTop(190);
    auto c = envRow.removeFromLeft(56);
    attackLabel.setBounds(c.removeFromTop(18));
    attack.setBounds(c);

    envRow.removeFromLeft(12);
    auto d = envRow.removeFromLeft(56);
    decayLabel.setBounds(d.removeFromTop(18));
    decay.setBounds(d);

    envRow.removeFromLeft(12);
    auto e = envRow.removeFromLeft(56);
    sustainLabel.setBounds(e.removeFromTop(18));
    sustain.setBounds(e);

    envRow.removeFromLeft(12);
    auto f = envRow.removeFromLeft(56);
    releaseLabel.setBounds(f.removeFromTop(18));
    release.setBounds(f);
}
