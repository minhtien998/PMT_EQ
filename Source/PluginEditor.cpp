#include "PluginEditor.h"

SmartEQAudioProcessorEditor::SmartEQAudioProcessorEditor(SmartEQAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p), eqGraph(p)
{
    setSize(800, 500);
    addAndMakeVisible(eqGraph);
    setResizable(true, true);
    setResizeLimits(600, 400, 1600, 1000);
}

void SmartEQAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkslategrey);
}

void SmartEQAudioProcessorEditor::resized() {
    eqGraph.setBounds(getLocalBounds().reduced(10));
}