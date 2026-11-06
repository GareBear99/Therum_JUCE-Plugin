#include "TherumPluginProcessor.h"
#include "TherumPluginEditor.h"
#include "ParameterIDs.h"
#include "../DSP/Filters/FilterTypeChoice.h"
#include "../DSP/Oscillators/MorphChoice.h"

TherumPluginProcessor::TherumPluginProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "THERUM_STATE", createParameterLayout())
{
}

void TherumPluginProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    voiceManager.prepare(sampleRate, 16);
}

void TherumPluginProcessor::releaseResources() {}

void TherumPluginProcessor::handleMidi(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
            voiceManager.noteOn(msg.getNoteNumber(), (float)msg.getFloatVelocity());
        else if (msg.isNoteOff())
            voiceManager.noteOff(msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            voiceManager.allNotesOff();
    }
}

void TherumPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    handleMidi(midiMessages);

    const float masterGainDb = apvts.getRawParameterValue(TherumParams::masterGain)->load();
    const float filterCutoff = apvts.getRawParameterValue(TherumParams::filter1_cutoff)->load();
    const float filterReso = apvts.getRawParameterValue(TherumParams::filter1_reso)->load();
    const int filterTypeIndex = (int)apvts.getRawParameterValue(TherumParams::filter1_type)->load();
    const float macro1 = apvts.getRawParameterValue(TherumParams::macro1)->load();
    const float delayMix = apvts.getRawParameterValue(TherumParams::fx_delayMix)->load();
    const float reverbMix = apvts.getRawParameterValue(TherumParams::fx_reverbMix)->load();
    const float distMix = apvts.getRawParameterValue(TherumParams::fx_distMix)->load();
    const float wtPosA = apvts.getRawParameterValue(TherumParams::oscA_wtPos)->load();
    const float wtPosB = apvts.getRawParameterValue(TherumParams::oscB_wtPos)->load();
    const int morphModeAIndex = (int)apvts.getRawParameterValue(TherumParams::oscA_morphMode)->load();
    const int morphPhaseAIndex = (int)apvts.getRawParameterValue(TherumParams::oscA_morphPhase)->load();
    const int morphModeBIndex = (int)apvts.getRawParameterValue(TherumParams::oscB_morphMode)->load();
    const int morphPhaseBIndex = (int)apvts.getRawParameterValue(TherumParams::oscB_morphPhase)->load();

    // The filter type is a discrete choice: apply it once per block
    // (the manager change-detects, and each voice crossfades, so a
    // change never clicks). The resonance is smoothed per sample
    // inside the manager, like the cutoff.
    voiceManager.setFilterType(TherumFilterChoice::typeFromIndex(filterTypeIndex));
    // The ADSR is a discrete set of stage times: apply it once per
    // block (the manager change-detects). The envelope value is
    // continuous, so a mid-note change just changes the slope from
    // here — it never clicks.
    voiceManager.setADSR(apvts.getRawParameterValue(TherumParams::env1_attack)->load(),
                         apvts.getRawParameterValue(TherumParams::env1_decay)->load(),
                         apvts.getRawParameterValue(TherumParams::env1_sustain)->load(),
                         apvts.getRawParameterValue(TherumParams::env1_release)->load());
    // The morph mode/phase are discrete choices too: apply them once
    // per block (the manager change-detects, and each oscillator
    // crossfades over 128 samples on a change, so a switch never
    // clicks). The frame positions are smoothed per sample inside
    // the manager, like the cutoff.
    voiceManager.setMorphA(TherumMorphChoice::modeFromIndex(morphModeAIndex),
                           TherumMorphChoice::phaseFromIndex(morphPhaseAIndex));
    voiceManager.setMorphB(TherumMorphChoice::modeFromIndex(morphModeBIndex),
                           TherumMorphChoice::phaseFromIndex(morphPhaseBIndex));

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // The core owns the master gain (it converts dB to linear, smooths
        // it, applies it, and limits the result), so the processor just
        // passes the raw parameter values through.
        const float s = voiceManager.renderMixed(filterCutoff, filterReso, macro1, delayMix, reverbMix, distMix, masterGainDb, wtPosA, wtPosB);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, sample, s);
    }
}

juce::AudioProcessorEditor* TherumPluginProcessor::createEditor()
{
    return new TherumPluginEditor(*this);
}

void TherumPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void TherumPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout TherumPluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::masterGain, "Master Gain", -48.0f, 12.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::qualityMode, "Quality Mode", juce::StringArray{"Eco","Standard","Ultra"}, 1));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscA_wtPos, "Osc A WT Pos", 0.0f, 1.0f, 0.25f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscA_level, "Osc A Level", 0.0f, 1.0f, 0.80f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscA_detune, "Osc A Detune", 0.0f, 1.0f, 0.10f));
    // The morph mode/phase for osc A. Crossfade is the cheap default;
    // Spectral is the quality mode (paper section 4.1). The labels
    // come from TherumMorphChoice, the same list the OscPanel
    // selectors show, so the parameter and the panel can never drift
    // apart. The phase choice is inert in Crossfade mode.
    {
        juce::StringArray morphModeChoices;
        for (const char* label : TherumMorphChoice::modeLabels)
            morphModeChoices.add(label);
        p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::oscA_morphMode, "Osc A Morph Mode", morphModeChoices, TherumMorphChoice::modeDefaultIndex));
        juce::StringArray morphPhaseChoices;
        for (const char* label : TherumMorphChoice::phaseLabels)
            morphPhaseChoices.add(label);
        p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::oscA_morphPhase, "Osc A Morph Phase", morphPhaseChoices, TherumMorphChoice::phaseDefaultIndex));
    }

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscB_wtPos, "Osc B WT Pos", 0.0f, 1.0f, 0.50f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscB_level, "Osc B Level", 0.0f, 1.0f, 0.60f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscB_detune, "Osc B Detune", 0.0f, 1.0f, 0.10f));
    // The morph mode/phase for osc B (same lists as osc A).
    {
        juce::StringArray morphModeChoices;
        for (const char* label : TherumMorphChoice::modeLabels)
            morphModeChoices.add(label);
        p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::oscB_morphMode, "Osc B Morph Mode", morphModeChoices, TherumMorphChoice::modeDefaultIndex));
        juce::StringArray morphPhaseChoices;
        for (const char* label : TherumMorphChoice::phaseLabels)
            morphPhaseChoices.add(label);
        p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::oscB_morphPhase, "Osc B Morph Phase", morphPhaseChoices, TherumMorphChoice::phaseDefaultIndex));
    }

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::sub_level, "Sub Level", 0.0f, 1.0f, 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::noise_level, "Noise Level", 0.0f, 1.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::filter1_cutoff, "Filter 1 Cutoff", 20.0f, 20000.0f, 18000.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::filter1_reso, "Filter 1 Resonance", 0.1f, 1.0f, 0.15f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::filter1_drive, "Filter 1 Drive", 0.0f, 1.0f, 0.0f));
    // The voice filter type. Index 0 (Low Pass) is the shipped voice
    // filter; Ladder selects the Moog ladder (1x in production).
    // The resonance parameter drives the ladder only — in SVF mode
    // it is inert (the SVF keeps Q = 0.7071). The labels come from
    // TherumFilterChoice, the same list the FilterPanel selector
    // shows, so the parameter and the panel can never drift apart.
    juce::StringArray filterTypeChoices;
    for (const char* label : TherumFilterChoice::labels)
        filterTypeChoices.add(label);
    p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::filter1_type, "Filter 1 Type", filterTypeChoices, TherumFilterChoice::defaultIndex));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::env1_attack, "Env 1 Attack", 1.0f, 5000.0f, 10.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::env1_decay, "Env 1 Decay", 1.0f, 5000.0f, 120.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::env1_sustain, "Env 1 Sustain", 0.0f, 1.0f, 0.75f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::env1_release, "Env 1 Release", 1.0f, 5000.0f, 220.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::macro1, "Macro 1", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::macro2, "Macro 2", 0.0f, 1.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::fx_reverbMix, "Reverb Mix", 0.0f, 1.0f, 0.15f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::fx_delayMix, "Delay Mix", 0.0f, 1.0f, 0.10f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::fx_distMix, "Dist Mix", 0.0f, 1.0f, 0.10f));
    return { p.begin(), p.end() };
}
