#pragma once
#include <JuceHeader.h>

class DiagnosticsPanel : public juce::Component
{
public:
    void setVoiceCount(int c) { voiceCount = c; repaint(); }
    void setRouteCount(int c) { routeCount = c; repaint(); }
    void setCpuMode(juce::String m) { cpuMode = std::move(m); repaint(); }

    void paint(juce::Graphics& g) override;

private:
    int voiceCount = 0;
    int routeCount = 0;
    juce::String cpuMode { "Standard" };
};
