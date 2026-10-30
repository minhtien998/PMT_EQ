#include "PluginProcessor.h"
#include "PluginEditor.h"

SmartEQAudioProcessor::SmartEQAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{}

juce::AudioProcessorValueTreeState::ParameterLayout SmartEQAudioProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout L;
    for (int b = 0; b < numBands; ++b) {
        juce::String id = "band" + juce::String(b);
        float defF = (b == 0) ? 100.0f : (b == numBands - 1) ? 8000.0f : 100.0f * std::pow(2.0f, (float)b);
        L.add(std::make_unique<juce::AudioParameterFloat>(id + "_freq", "Freq", juce::NormalisableRange<float>(20.f, 20000.f, 0.f, 0.3f), defF));
        L.add(std::make_unique<juce::AudioParameterFloat>(id + "_gain", "Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.01f), 0.f));
        L.add(std::make_unique<juce::AudioParameterFloat>(id + "_q", "Q", juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 0.5f), 1.0f));
        L.add(std::make_unique<juce::AudioParameterBool>(id + "_on", "On", true));
    }
    return L;
}

void SmartEQAudioProcessor::prepareToPlay(double sr, int) {
    analyzer.prepare(sr);
    for (int b = 0; b < numBands; ++b) {
        lastOn[b] = false;
        for (int ch = 0; ch < 2; ++ch)
            filters[b][ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass(sr, 1000.0f);
    }
    analyzer.reset();
}

void SmartEQAudioProcessor::updateFilters() {
    const double sr = getSampleRate();
    for (int b = 0; b < numBands; ++b) {
        juce::String id = "band" + juce::String(b);
        float freq = *apvts.getRawParameterValue(id + "_freq");
        float gain = *apvts.getRawParameterValue(id + "_gain");
        float q    = *apvts.getRawParameterValue(id + "_q");
        bool  on   = *apvts.getRawParameterValue(id + "_on") > 0.5f;
        if (freq == lastFreq[b] && gain == lastGain[b] && q == lastQ[b] && on == lastOn[b]) continue;
        lastFreq[b] = freq; lastGain[b] = gain; lastQ[b] = q; lastOn[b] = on;
        juce::dsp::IIR::Coefficients<float>::Ptr c;
        const float g = juce::Decibels::decibelsToGain(gain);
        if (b == 0) c = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, freq, q, g);
        else if (b == numBands - 1) c = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, freq, q, g);
        else c = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, freq, q, g);
        for (int ch = 0; ch < 2; ++ch) filters[b][ch].coefficients = c;
    }
}

void SmartEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    if (nCh == 0) return;

    updateFilters();

    float* L = buffer.getWritePointer(0);
    float* R = nCh > 1 ? buffer.getWritePointer(1) : L;

    for (int i = 0; i < n; ++i) {
        float l = L[i], r = R[i];
        for (int b = 0; b < numBands; ++b) {
            if (!lastOn[b]) continue;
            l = filters[b][0].processSample(l);
            r = filters[b][1].processSample(r);
        }
        L[i] = l;
        R[i] = r;
    }
    analyzer.pushAudio(L, R, n);
}

void SmartEQAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void SmartEQAudioProcessor::setStateInformation(const void* data, int sz) {
    if (auto xml = getXmlFromBinary(data, sz))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* SmartEQAudioProcessor::createEditor() {
    return new SmartEQAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SmartEQAudioProcessor();
}