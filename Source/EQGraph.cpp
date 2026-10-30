#include "EQGraph.h"

EQGraph::EQGraph(SmartEQAudioProcessor& p) : proc(p) {
    setInterceptsMouseClicks(true, false);
    startTimerHz(30);
}

void EQGraph::timerCallback() { repaint(); }

float EQGraph::xToFreq(float x) const {
    float norm = juce::jlimit(0.0f, 1.0f, x / (float)getWidth());
    return 20.0f * std::pow(1000.0f, norm);
}

float EQGraph::freqToX(float f) const {
    float norm = std::log(f / 20.0f) / std::log(1000.0f);
    return norm * (float)getWidth();
}

float EQGraph::yToGain(float y) const {
    return juce::jmap(y, 0.0f, (float)getHeight(), 24.0f, -24.0f);
}

float EQGraph::gainToY(float g) const {
    return juce::jmap(g, 24.0f, -24.0f, 0.0f, (float)getHeight());
}

void EQGraph::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    const float freqs[] = {50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float f : freqs) {
        float x = freqToX(f);
        g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
        g.drawText(juce::String((int)f), (int)x + 2, getHeight() - 15, 50, 15, juce::Justification::left);
    }
    for (int i = -24; i <= 24; i += 6) {
        float y = gainToY((float)i);
        g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
    }

    auto fastDb = proc.analyzer.getFastDb();
    juce::Path spectrumPath;
    bool started = false;
    for (int i = 0; i < SmartAnalyzer::numBins; ++i) {
        float f = proc.analyzer.binToFreq(i);
        if (f < 20.0f || f > 20000.0f) continue;
        float x = freqToX(f);
        float y = juce::jmap(juce::jlimit(-100.0f, 0.0f, fastDb[i]), -100.0f, 0.0f, (float)getHeight(), 0.0f);
        if (!started) { spectrumPath.startNewSubPath(x, y); started = true; }
        else spectrumPath.lineTo(x, y);
    }
    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));

    for (int b = 0; b < SmartEQAudioProcessor::numBands; ++b) {
        float freq = *proc.apvts.getRawParameterValue("band" + juce::String(b) + "_freq");
        float gain = *proc.apvts.getRawParameterValue("band" + juce::String(b) + "_gain");
        bool  on   = *proc.apvts.getRawParameterValue("band" + juce::String(b) + "_on") > 0.5f;

        float x = freqToX(freq);
        float y = gainToY(gain);

        g.setColour(on ? juce::Colours::orange : juce::Colours::grey);
        g.drawEllipse(x - 8, y - 8, 16, 16, 2.0f);
        g.drawText(juce::String(b + 1), (int)x - 5, (int)y - 5, 10, 10, juce::Justification::centred);
    }
}

void EQGraph::mouseDown(const juce::MouseEvent& e) {
    draggedBand = -1;
    for (int b = 0; b < SmartEQAudioProcessor::numBands; ++b) {
        float freq = *proc.apvts.getRawParameterValue("band" + juce::String(b) + "_freq");
        float gain = *proc.apvts.getRawParameterValue("band" + juce::String(b) + "_gain");
        float x = freqToX(freq);
        float y = gainToY(gain);
        if (e.position.getDistanceFrom(juce::Point<float>(x, y)) < 15.0f) {
            draggedBand = b;
            break;
        }
    }
}

void EQGraph::mouseDrag(const juce::MouseEvent& e) {
    if (draggedBand < 0) return;
    float freq = xToFreq(e.position.x);
    float gain = yToGain(e.position.y);
    
    if (auto* pFreq = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("band" + juce::String(draggedBand) + "_freq")))
        pFreq->setValueNotifyingHost(pFreq->convertTo0to1(freq));
    if (auto* pGain = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("band" + juce::String(draggedBand) + "_gain")))
        pGain->setValueNotifyingHost(pGain->convertTo0to1(gain));
}

void EQGraph::mouseUp(const juce::MouseEvent&) {
    draggedBand = -1;
}

bool EQGraph::isInterestedInFileDrag(const juce::StringArray& files) {
    return files.size() == 1 && (files[0].endsWithIgnoreCase(".wav") || files[0].endsWithIgnoreCase(".mp3"));
}

void EQGraph::filesDropped(const juce::StringArray& files, int, int) {
    if (files.isEmpty()) return;
    proc.analyzer.analyzeFile(juce::File(files[0]));
    repaint();
}