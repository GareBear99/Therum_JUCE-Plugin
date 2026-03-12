#include "TherumAttachments.h"
#include "../Core/TherumPluginProcessor.h"
#include "../Core/ParameterIDs.h"
#include "Panels/TopBarPanel.h"
#include "Panels/OscPanel.h"
#include "Panels/FilterPanel.h"
#include "Panels/ModPanel.h"
#include "Panels/FxPanel.h"

void TherumAttachments::wire(TopBarPanel& topBar, OscPanel& oscPanel, FilterPanel& filterPanel, ModPanel& modPanel, FxPanel& fxPanel)
{
    auto& apvts = processor.getAPVTS();

    masterGainAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::masterGain, topBar.getMasterGainSlider());
    oscAWtPosAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::oscA_wtPos, oscPanel.getWtPosSlider());
    filterCutoffAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::filter1_cutoff, filterPanel.getCutoffSlider());
    filterResoAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::filter1_reso, filterPanel.getResoSlider());
    macro1Attachment = std::make_unique<SliderAttachment>(apvts, TherumParams::macro1, modPanel.getMacro1Slider());
    macro2Attachment = std::make_unique<SliderAttachment>(apvts, TherumParams::macro2, modPanel.getMacro2Slider());
    delayMixAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::fx_delayMix, fxPanel.getDelayMixSlider());
    reverbMixAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::fx_reverbMix, fxPanel.getReverbMixSlider());
    distMixAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::fx_distMix, fxPanel.getDistMixSlider());
}
