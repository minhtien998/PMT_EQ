#include "SmartAnalyzer.h"

SmartAnalyzer::SmartAnalyzer()
    : fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    formatManager.registerBasicFormats();
    fftData.resize(fftSize * 2, 0.0f);
    ringBuffer.resize(fftSize, 0.0f);
    fastDb.resize(numBins, -100.0f);
    longTermDb.resize(numBins, -100.0f);
}

void SmartAnalyzer::prepare(double sr) {
    std::lock_guard<std::mutex> lockGuard(lock);
    sampleRate = sr;
    reset();
}

void SmartAnalyzer::reset() {
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);
    std::fill(fastDb.begin(), fastDb.end(), -100.0f);
    std::fill(longTermDb.begin(), longTermDb.end(), -100.0f);
    ringWrite = 0;
    hopCounter = 0;
}

void SmartAnalyzer::pushAudio(const float* left, const float* right, int numSamples) {
    std::lock_guard<std::mutex> lockGuard(lock);
    for (int i = 0; i < numSamples; ++i) {
        float sample = (left ? left[i] : 0.0f) + (right ? right[i] : 0.0f);
        ringBuffer[ringWrite] = sample * 0.5f;
        ringWrite = (ringWrite + 1) % fftSize;
        
        if (++hopCounter >= hopSize) {
            hopCounter = 0;
            computeFrame();
        }
    }
}

void SmartAnalyzer::computeFrame() {
    for (int i = 0; i < fftSize; ++i) {
        int idx = (ringWrite + i) % fftSize;
        fftData[i] = ringBuffer[idx];
    }
    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    for (int i = 0; i < numBins; ++i) {
        float mag = fftData[i];
        float db = juce::Decibels::gainToDecibels(mag, -100.0f);
        fastDb[i] = juce::jmax(fastDb[i] * 0.7f + db * 0.3f, -100.0f);
    }
    updateLongTerm();
}

void SmartAnalyzer::updateLongTerm() {
    for (int i = 0; i < numBins; ++i) {
        if (fastDb[i] > longTermDb[i]) 
            longTermDb[i] += (fastDb[i] - longTermDb[i]) * 0.3f;
        else 
            longTermDb[i] -= (longTermDb[i] - fastDb[i]) * 0.01f;
    }
}

void SmartAnalyzer::analyzeFile(const juce::File& file) {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return;

    std::lock_guard<std::mutex> lockGuard(lock);
    reset();
    
    juce::AudioBuffer<float> buffer(reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);

    double originalSr = sampleRate;
    sampleRate = reader->sampleRate;

    for (int i = 0; i < buffer.getNumSamples(); i += hopSize) {
        int blockSize = juce::jmin(hopSize, buffer.getNumSamples() - i);
        for (int s = 0; s < blockSize; ++s) {
            float l = buffer.getSample(0, i + s);
            float r = buffer.getNumChannels() > 1 ? buffer.getSample(1, i + s) : l;
            ringBuffer[ringWrite] = (l + r) * 0.5f;
            ringWrite = (ringWrite + 1) % fftSize;
        }
        computeFrame();
    }
    sampleRate = originalSr;
}

std::vector<float> SmartAnalyzer::getFastDb() const {
    std::lock_guard<std::mutex> lockGuard(lock);
    return fastDb;
}

std::vector<float> SmartAnalyzer::getLongTermDb() const {
    std::lock_guard<std::mutex> lockGuard(lock);
    return longTermDb;
}