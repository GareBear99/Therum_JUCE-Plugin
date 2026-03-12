#pragma once
#include <JuceHeader.h>

struct TherumEditorState
{
    juce::Rectangle<int> topBar;
    juce::Rectangle<int> oscPanel;
    juce::Rectangle<int> filterPanel;
    juce::Rectangle<int> browserPanel;
    juce::Rectangle<int> modPanel;
    juce::Rectangle<int> matrixPanel;
    juce::Rectangle<int> fxPanel;
    juce::Rectangle<int> diagnosticsPanel;
    juce::Rectangle<int> mascotPanel;
};
