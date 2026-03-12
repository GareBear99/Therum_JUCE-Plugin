#include "TherumPluginEditor.h"

TherumPluginEditor::TherumPluginEditor(TherumPluginProcessor& p)
: AudioProcessorEditor(&p), processor(p), attachments(p)
{
    setSize(1460, 900);

    addAndMakeVisible(topBar);
    addAndMakeVisible(oscPanel);
    addAndMakeVisible(filterPanel);
    addAndMakeVisible(browserPanel);
    addAndMakeVisible(modPanel);
    addAndMakeVisible(matrixPanel);
    addAndMakeVisible(fxPanel);
    addAndMakeVisible(diagnosticsPanel);
    addAndMakeVisible(mascotPanel);

    attachments.wire(topBar, oscPanel, filterPanel, modPanel, fxPanel);

    startTimerHz(60);
}

void TherumPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(5, 8, 22));

    if (showSplash)
    {
        g.setColour(juce::Colours::black.withAlpha(0.88f * splashAlpha));
        g.fillAll();

        auto fb = getLocalBounds().toFloat();

        g.setGradientFill(juce::ColourGradient(
            juce::Colour::fromRGBA(0, 246, 255, (juce::uint8)(75 * splashAlpha)),
            fb.getCentreX(), fb.getCentreY() - 120.0f,
            juce::Colour::fromRGBA(139, 92, 246, (juce::uint8)(14 * splashAlpha)),
            fb.getCentreX(), fb.getCentreY() + 170.0f,
            true));
        g.fillEllipse(fb.getCentreX() - 200.0f, fb.getCentreY() - 230.0f, 400.0f, 400.0f);

        auto mascot = juce::Rectangle<float>(fb.getCentreX() - 120.0f, fb.getCentreY() - 208.0f, 240.0f, 240.0f);
        g.setColour(juce::Colour::fromRGBA(255, 140, 70, (juce::uint8)(230 * splashAlpha)));
        g.drawRoundedRectangle(mascot, 26.0f, 3.0f);
        g.setFont(18.0f);
        g.drawFittedText("PUNCH REACTOR", mascot.toNearestInt().reduced(18), juce::Justification::centred, 2);

        g.setColour(juce::Colour::fromRGBA(238, 244, 255, (juce::uint8)(255 * splashAlpha)));
        g.setFont(48.0f);
        g.drawFittedText("THERUM", (int)fb.getCentreX() - 260, (int)fb.getCentreY() + 55, 520, 58, juce::Justification::centred, 1);

        g.setFont(15.0f);
        g.setColour(juce::Colour::fromRGBA(168, 184, 216, (juce::uint8)(220 * splashAlpha)));
        g.drawFittedText("Mascot Splash Pre-Boot", (int)fb.getCentreX() - 230, (int)fb.getCentreY() + 110, 460, 24, juce::Justification::centred, 1);
        g.drawFittedText("Click to skip", (int)fb.getCentreX() - 170, (int)fb.getCentreY() + 136, 340, 22, juce::Justification::centred, 1);

        auto progress = juce::jlimit(0.0f, 1.0f, splashTicks / 72.0f);
        auto bar = juce::Rectangle<float>(fb.getCentreX() - 180.0f, fb.getCentreY() + 170.0f, 360.0f, 10.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f * splashAlpha));
        g.fillRoundedRectangle(bar, 5.0f);
        g.setColour(juce::Colour::fromRGBA(0, 246, 255, (juce::uint8)(255 * splashAlpha)));
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * progress), 5.0f);
    }
}

void TherumPluginEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    layoutState.topBar = area.removeFromTop(76);

    auto hero = area.removeFromTop(258);
    layoutState.oscPanel = hero.removeFromLeft((int)(getWidth() * 0.40f)).reduced(6);
    layoutState.filterPanel = hero.removeFromLeft((int)(getWidth() * 0.22f)).reduced(6);
    layoutState.browserPanel = hero.reduced(6);

    auto lower = area.reduced(0, 6);
    layoutState.modPanel = lower.removeFromLeft((int)(getWidth() * 0.22f)).reduced(6);
    layoutState.matrixPanel = lower.removeFromLeft((int)(getWidth() * 0.22f)).reduced(6);
    layoutState.fxPanel = lower.removeFromLeft((int)(getWidth() * 0.18f)).reduced(6);
    layoutState.mascotPanel = lower.removeFromLeft((int)(getWidth() * 0.14f)).reduced(6);
    layoutState.diagnosticsPanel = lower.reduced(6);

    topBar.setBounds(layoutState.topBar);
    oscPanel.setBounds(layoutState.oscPanel);
    filterPanel.setBounds(layoutState.filterPanel);
    browserPanel.setBounds(layoutState.browserPanel);
    modPanel.setBounds(layoutState.modPanel);
    matrixPanel.setBounds(layoutState.matrixPanel);
    fxPanel.setBounds(layoutState.fxPanel);
    mascotPanel.setBounds(layoutState.mascotPanel);
    diagnosticsPanel.setBounds(layoutState.diagnosticsPanel);
}

void TherumPluginEditor::mouseDown(const juce::MouseEvent&)
{
    if (showSplash)
        skipSplash();
}

void TherumPluginEditor::timerCallback()
{
    ++splashTicks;

    diagnosticsPanel.setVoiceCount(processor.getVoiceManager().getActiveVoiceCount());
    diagnosticsPanel.setRouteCount(0);
    diagnosticsPanel.setCpuMode("Standard");

    if (showSplash && splashTicks > 72)
    {
        splashAlpha -= 0.06f;
        if (splashAlpha <= 0.0f)
        {
            splashAlpha = 0.0f;
            showSplash = false;
            stopTimer();
        }
    }

    repaint();
}

void TherumPluginEditor::skipSplash()
{
    splashTicks = 999;
    splashAlpha = 0.0f;
    showSplash = false;
    stopTimer();
    repaint();
}
