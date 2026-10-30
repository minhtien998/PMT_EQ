#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SmartAnalyzer.h"

class SmartEQAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numBands = 6;
    SmartEQAudioProcessor();

    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Smart EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    SmartAnalyzer analyzer;

private:
    juce::dsp::IIR::Filter<float> filters[numBands][2];
    float lastFreq[numBands] = {};
    float lastGain[numBands] = {};
    float lastQ[numBands]    = {};
    bool  lastOn[numBands]   = {};

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartEQAudioProcessor)
};