#include "DiagnosticsPanel.h"

void DiagnosticsPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour::fromRGB(20, 26, 48));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);

    auto area = getLocalBounds().reduced(16);
    g.setColour(juce::Colour::fromRGB(255, 79, 216));
    g.setFont(18.0f);
    g.drawText("DIAGNOSTICS", area.removeFromTop(24), juce::Justification::left);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(14.0f);
    g.drawText("Voices: " + juce::String(voiceCount), area.removeFromTop(22), juce::Justification::left);
    g.drawText("Routes: " + juce::String(routeCount), area.removeFromTop(22), juce::Justification::left);
    g.drawText("Mode: " + cpuMode, area.removeFromTop(22), juce::Justification::left);
}
