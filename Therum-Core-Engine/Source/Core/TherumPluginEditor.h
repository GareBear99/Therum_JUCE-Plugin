#pragma once
#include <JuceHeader.h>
#include "TherumPluginProcessor.h"
#include "../UI/TherumEditorState.h"
#include "../UI/TherumAttachments.h"
#include "../UI/Panels/TopBarPanel.h"
#include "../UI/Panels/OscPanel.h"
#include "../UI/Panels/FilterPanel.h"
#include "../UI/Panels/BrowserPanel.h"
#include "../UI/Panels/ModPanel.h"
#include "../UI/Panels/MatrixPanel.h"
#include "../UI/Panels/FxPanel.h"
#include "../UI/Panels/DiagnosticsPanel.h"
#include "../UI/Panels/MascotPanel.h"

class TherumPluginEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit TherumPluginEditor(TherumPluginProcessor&);
    ~TherumPluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void skipSplash();

    TherumPluginProcessor& processor;
    TherumAttachments attachments;

    TopBarPanel topBar;
    OscPanel oscPanel;
    FilterPanel filterPanel;
    BrowserPanel browserPanel;
    ModPanel modPanel;
    MatrixPanel matrixPanel;
    FxPanel fxPanel;
    DiagnosticsPanel diagnosticsPanel;
    MascotPanel mascotPanel;

    TherumEditorState layoutState;

    bool showSplash = true;
    float splashAlpha = 1.0f;
    int splashTicks = 0;
};
