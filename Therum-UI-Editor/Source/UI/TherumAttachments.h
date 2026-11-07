#pragma once
#include <JuceHeader.h>

class TherumPluginProcessor;
class TopBarPanel;
class OscPanel;
class FilterPanel;
class ModPanel;
class FxPanel;

class TherumAttachments
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    explicit TherumAttachments(TherumPluginProcessor& p) : processor(p) {}

    void wire(TopBarPanel& topBar, OscPanel& oscPanel, FilterPanel& filterPanel, ModPanel& modPanel, FxPanel& fxPanel);

    // Releases every attachment while the sliders they reference are
    // still alive. Call from the editor destructor, before any panel
    // is destroyed (see the declaration-order note in
    // TherumPluginEditor.h).
    void clear();

private:
    TherumPluginProcessor& processor;

    std::unique_ptr<SliderAttachment> masterGainAttachment;
    std::unique_ptr<SliderAttachment> oscAWtPosAttachment;
    std::unique_ptr<ComboBoxAttachment> oscAMorphModeAttachment;
    std::unique_ptr<ComboBoxAttachment> oscAMorphPhaseAttachment;
    std::unique_ptr<SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<SliderAttachment> filterResoAttachment;
    std::unique_ptr<ComboBoxAttachment> filterTypeAttachment;
    std::unique_ptr<SliderAttachment> macro1Attachment;
    std::unique_ptr<SliderAttachment> macro2Attachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    std::unique_ptr<SliderAttachment> distMixAttachment;
};
