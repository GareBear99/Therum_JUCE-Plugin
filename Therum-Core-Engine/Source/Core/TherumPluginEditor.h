#pragma once
#include <JuceHeader.h>
#include "TherumPluginProcessor.h"
#include "UI/TherumEditorState.h"
#include "UI/TherumAttachments.h"
#include "UI/Panels/TopBarPanel.h"
#include "UI/Panels/OscPanel.h"
#include "UI/Panels/FilterPanel.h"
#include "UI/Panels/BrowserPanel.h"
#include "UI/Panels/ModPanel.h"
#include "UI/Panels/MatrixPanel.h"
#include "UI/Panels/FxPanel.h"
#include "UI/Panels/DiagnosticsPanel.h"
#include "UI/Panels/MascotPanel.h"

class TherumPluginEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit TherumPluginEditor(TherumPluginProcessor&);
    ~TherumPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void skipSplash();

    TherumPluginProcessor& processor;

    TopBarPanel topBar;
    OscPanel oscPanel;
    FilterPanel filterPanel;
    BrowserPanel browserPanel;
    ModPanel modPanel;
    MatrixPanel matrixPanel;
    FxPanel fxPanel;
    DiagnosticsPanel diagnosticsPanel;
    MascotPanel mascotPanel;

    // The attachments MUST be declared after every panel they wire to,
    // and are also released explicitly in ~TherumPluginEditor.
    // Members are destroyed in reverse declaration order, so an
    // attachment destroyed after its slider would dereference the freed
    // slider in ~SliderParameterAttachment (EXC_BAD_ACCESS at null+0x48
    // on editor close, Live-2026-11-07-014655). Declared last, the
    // attachments die first, while every slider is still alive.
    TherumAttachments attachments;

    TherumEditorState layoutState;

    bool showSplash = true;
    float splashAlpha = 1.0f;
    int splashTicks = 0;
};
