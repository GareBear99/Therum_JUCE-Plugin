#pragma once
#include <JuceHeader.h>

class BrowserPanel : public juce::Component
{
public:
    BrowserPanel();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label title;
    juce::ListBox listBox;

    class Model : public juce::ListBoxModel
    {
    public:
        int getNumRows() override { return 4; }
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
        {
            if (selected)
                g.fillAll(juce::Colour::fromRGBA(0, 246, 255, 40));
            g.setColour(juce::Colours::white.withAlpha(0.86f));
            static const char* rows[] = {"Init Patch", "Neon Pad Stub", "Glass Lead Stub", "Void Bass Stub"};
            g.drawText(rows[row], 8, 0, width - 16, height, juce::Justification::centredLeft);
        }
    } model;
};
