#pragma once
#include <JuceHeader.h>
#include <vector>
#include <mutex>

class SmartAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins  = fftSize / 2 + 1;

    SmartAnalyzer();
    void prepare(double sampleRate);
    void pushAudio(const float* left, const float* right, int numSamples);
    void reset();
    void analyzeFile(const juce::File& file);

    std::vector<float> getLongTermDb() const;
    std::vector<float> getFastDb() const;
    float binToFreq(int bin) const { return bin * (float)sampleRate / (float)fftSize; }

private:
    double sampleRate = 48000.0;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    juce::AudioFormatManager formatManager;

    std::vector<float> fftData;
    std::vector<float> ringBuffer;
    int ringWrite = 0;
    int hopCounter = 0;
    static constexpr int hopSize = 512;

    std::vector<float> fastDb;
    std::vector<float> longTermDb;
    
    mutable std::mutex lock;

    void computeFrame();
    void updateLongTerm();
};