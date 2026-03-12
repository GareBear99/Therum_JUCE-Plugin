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

    explicit TherumAttachments(TherumPluginProcessor& p) : processor(p) {}

    void wire(TopBarPanel& topBar, OscPanel& oscPanel, FilterPanel& filterPanel, ModPanel& modPanel, FxPanel& fxPanel);

private:
    TherumPluginProcessor& processor;

    std::unique_ptr<SliderAttachment> masterGainAttachment;
    std::unique_ptr<SliderAttachment> oscAWtPosAttachment;
    std::unique_ptr<SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<SliderAttachment> filterResoAttachment;
    std::unique_ptr<SliderAttachment> macro1Attachment;
    std::unique_ptr<SliderAttachment> macro2Attachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    std::unique_ptr<SliderAttachment> distMixAttachment;
};
