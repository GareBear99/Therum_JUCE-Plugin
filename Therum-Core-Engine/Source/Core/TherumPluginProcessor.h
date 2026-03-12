#pragma once
#include <JuceHeader.h>
#include "../DSP/Voicing/VoiceManager.h"

class TherumPluginProcessor : public juce::AudioProcessor
{
public:
    TherumPluginProcessor();
    ~TherumPluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    const juce::String getName() const override { return "THERUM"; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    const VoiceManager& getVoiceManager() const { return voiceManager; }

private:
    void handleMidi(juce::MidiBuffer& midiMessages);

    juce::AudioProcessorValueTreeState apvts;
    VoiceManager voiceManager;
    double currentSampleRate = 44100.0;
};
