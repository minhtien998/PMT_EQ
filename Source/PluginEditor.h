#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EQGraph.h"

class SmartEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SmartEQAudioProcessorEditor(SmartEQAudioProcessor&);
    ~SmartEQAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SmartEQAudioProcessor& proc;
    EQGraph eqGraph;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartEQAudioProcessorEditor)
};