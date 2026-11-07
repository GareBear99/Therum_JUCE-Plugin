#include "TherumAttachments.h"
#include "Core/TherumPluginProcessor.h"
#include "Core/ParameterIDs.h"
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
    oscAMorphModeAttachment = std::make_unique<ComboBoxAttachment>(apvts, TherumParams::oscA_morphMode, oscPanel.getMorphModeBox());
    oscAMorphPhaseAttachment = std::make_unique<ComboBoxAttachment>(apvts, TherumParams::oscA_morphPhase, oscPanel.getMorphPhaseBox());
    filterCutoffAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::filter1_cutoff, filterPanel.getCutoffSlider());
    filterResoAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::filter1_reso, filterPanel.getResoSlider());
    filterTypeAttachment = std::make_unique<ComboBoxAttachment>(apvts, TherumParams::filter1_type, filterPanel.getTypeComboBox());
    macro1Attachment = std::make_unique<SliderAttachment>(apvts, TherumParams::macro1, modPanel.getMacro1Slider());
    macro2Attachment = std::make_unique<SliderAttachment>(apvts, TherumParams::macro2, modPanel.getMacro2Slider());
    attackAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::env1_attack, modPanel.getAttackSlider());
    decayAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::env1_decay, modPanel.getDecaySlider());
    sustainAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::env1_sustain, modPanel.getSustainSlider());
    releaseAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::env1_release, modPanel.getReleaseSlider());
    delayMixAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::fx_delayMix, fxPanel.getDelayMixSlider());
    reverbMixAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::fx_reverbMix, fxPanel.getReverbMixSlider());
    distMixAttachment = std::make_unique<SliderAttachment>(apvts, TherumParams::fx_distMix, fxPanel.getDistMixSlider());
}

void TherumAttachments::clear()
{
    // Destroy in reverse wire order; each reset detaches from its
    // slider, so every slider must still be alive here.
    distMixAttachment.reset();
    reverbMixAttachment.reset();
    delayMixAttachment.reset();
    releaseAttachment.reset();
    sustainAttachment.reset();
    decayAttachment.reset();
    attackAttachment.reset();
    macro2Attachment.reset();
    macro1Attachment.reset();
    filterTypeAttachment.reset();
    filterResoAttachment.reset();
    filterCutoffAttachment.reset();
    oscAMorphPhaseAttachment.reset();
    oscAMorphModeAttachment.reset();
    oscAWtPosAttachment.reset();
    masterGainAttachment.reset();
}
