#include "TherumPluginProcessor.h"
#include "TherumPluginEditor.h"
#include "ParameterIDs.h"

// Maps the filter1_type choice index (0 Low Pass, 1 High Pass,
// 2 Band Pass, 3 Notch, 4 Ladder) to the voice filter type. Any
// out-of-range index falls back to the shipped Low Pass.
static TherumFilterType filterTypeFromIndex(int index)
{
    switch (index)
    {
        case 1: return TherumFilterType::HighPass;
        case 2: return TherumFilterType::BandPass;
        case 3: return TherumFilterType::Notch;
        case 4: return TherumFilterType::Ladder;
        case 0:
        default: return TherumFilterType::LowPass;
    }
}

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

    // The filter type is a discrete choice: apply it once per block
    // (the manager change-detects, and each voice crossfades, so a
    // change never clicks). The resonance is smoothed per sample
    // inside the manager, like the cutoff.
    voiceManager.setFilterType(filterTypeFromIndex(filterTypeIndex));

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // The core owns the master gain (it converts dB to linear, smooths
        // it, applies it, and limits the result), so the processor just
        // passes the raw parameter values through.
        const float s = voiceManager.renderMixed(filterCutoff, filterReso, macro1, delayMix, reverbMix, distMix, masterGainDb);
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

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscB_wtPos, "Osc B WT Pos", 0.0f, 1.0f, 0.50f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscB_level, "Osc B Level", 0.0f, 1.0f, 0.60f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::oscB_detune, "Osc B Detune", 0.0f, 1.0f, 0.10f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::sub_level, "Sub Level", 0.0f, 1.0f, 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::noise_level, "Noise Level", 0.0f, 1.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::filter1_cutoff, "Filter 1 Cutoff", 20.0f, 20000.0f, 18000.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::filter1_reso, "Filter 1 Resonance", 0.1f, 1.0f, 0.15f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(TherumParams::filter1_drive, "Filter 1 Drive", 0.0f, 1.0f, 0.0f));
    // The voice filter type. Index 0 (Low Pass) is the shipped voice
    // filter; Ladder selects the Moog ladder (1x in production).
    // The resonance parameter drives the ladder only — in SVF mode
    // it is inert (the SVF keeps Q = 0.7071).
    p.push_back(std::make_unique<juce::AudioParameterChoice>(TherumParams::filter1_type, "Filter 1 Type", juce::StringArray{"Low Pass", "High Pass", "Band Pass", "Notch", "Ladder"}, 0));

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
