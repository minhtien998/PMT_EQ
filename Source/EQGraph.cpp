#include "EQGraph.h"

EQGraph::EQGraph(SmartEQAudioProcessor& p) : proc(p) {
    setInterceptsMouseClicks(true, true); // Cho phép nhận sự kiện chuột
    startTimerHz(30); // Cập nhật đồ thị 30 lần/giây
}

void EQGraph::timerCallback() { 
    repaint(); 
}

// Chuyển đổi tọa độ X (trục ngang) sang tần số (Logarit: 20Hz -> 20kHz)
float EQGraph::xToFreq(float x) const {
    float norm = juce::jlimit(0.0f, 1.0f, x / (float)getWidth());
    return 20.0f * std::pow(1000.0f, norm); 
}

float EQGraph::freqToX(float f) const {
    float norm = std::log(f / 20.0f) / std::log(1000.0f);
    return norm * (float)getWidth();
}

// Chuyển đổi tọa độ Y (trục dọc) sang Gain (dB)
float EQGraph::yToGain(float y) const {
    return juce::jmap(y, 0.0f, (float)getHeight(), 24.0f, -24.0f);
}

float EQGraph::gainToY(float g) const {
    return juce::jmap(g, 24.0f, -24.0f, 0.0f, (float)getHeight());
}

void EQGraph::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    // 1. Vẽ lưới tần số (Trục X)
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    const float freqs[] = {50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float f : freqs) {
        float x = freqToX(f);
        g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
        g.drawText(juce::String((int)f), (int)x + 2, getHeight() - 15, 50, 15, juce::Justification::left);
    }

    // Vẽ lưới Gain (Trục Y)
    for (int i = -24; i <= 24; i += 6) {
        float y = gainToY((float)i);
        g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
    }

    // 2. Vẽ phổ âm thanh (FFT)
    auto fastDb = proc.analyzer.getFastDb();
    juce::Path spectrumPath;
    bool started = false;
    for (int i = 0; i < SmartAnalyzer::numBins; ++i) {
        float f = proc.analyzer.binToFreq(i);
        if (f < 20.0f || f > 20000.0f) continue; // Chỉ vẽ dải nghe được
        
        float x = freqToX(f);
        // Map từ -100dB -> 0dB lên chiều cao màn hình
        float y = juce::jmap(juce::jlimit(-100.0f, 0.0f, fastDb[i]), -100.0f, 0.0f, (float)getHeight(), 0.0f);
        
        if (!started) { 
            spectrumPath.startNewSubPath(x, y); 
            started = true; 
        } else {
            spectrumPath.lineTo(x, y);
        }
    }
    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));

    // 3. Vẽ các điểm nút EQ (Nodes)
    for (int b = 0; b < SmartEQAudioProcessor::numBands; ++b) {
        juce::String id = "band" + juce::String(b);
        float freq = *proc.apvts.getRawParameterValue(id + "_freq");
        float gain = *proc.apvts.getRawParameterValue(id + "_gain");
        bool  on   = *proc.apvts.getRawParameterValue(id + "_on") > 0.5f;

        float x = freqToX(freq);
        float y = gainToY(gain);

        // Đổi màu cam nếu đang bật, màu xám nếu đang tắt
        g.setColour(on ? juce::Colours::orange : juce::Colours::grey);
        g.drawEllipse(x - 8, y - 8, 16, 16, 2.0f);
        g.drawText(juce::String(b + 1), (int)x - 5, (int)y - 5, 10, 10, juce::Justification::centred);
    }
}

void EQGraph::mouseDown(const juce::MouseEvent& e) {
    draggedBand = -1;
    // Tìm xem người dùng click vào Node nào
    for (int b = 0; b < SmartEQAudioProcessor::numBands; ++b) {
        juce::String id = "band" + juce::String(b);
        float freq = *proc.apvts.getRawParameterValue(id + "_freq");
        float gain = *proc.apvts.getRawParameterValue(id + "_gain");
        
        float x = freqToX(freq);
        float y = gainToY(gain);
        
        // Nếu khoảng cách từ chuột đến tâm Node < 15 pixel thì bắt đầu kéo
        if (e.position.getDistanceFrom(juce::Point<float>(x, y)) < 15.0f) {
            draggedBand = b;
            break;
        }
    }
}

void EQGraph::mouseDrag(const juce::MouseEvent& e) {
    if (draggedBand < 0) return;
    
    // Cập nhật tần số và gain theo vị trí chuột
    float freq = xToFreq(e.position.x);
    float gain = yToGain(e.position.y);
    
    juce::String id = "band" + juce::String(draggedBand);
    
    // Cập nhật giá trị vào APVTS
    if (auto* pFreq = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter(id + "_freq")))
        pFreq->setValueNotifyingHost(pFreq->convertTo0to1(freq));
        
    if (auto* pGain = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter(id + "_gain")))
        pGain->setValueNotifyingHost(pGain->convertTo0to1(gain));
}

void EQGraph::mouseUp(const juce::MouseEvent&) {
    draggedBand = -1;
}

// Xử lý Drag & Drop File
bool EQGraph::isInterestedInFileDrag(const juce::StringArray& files) {
    if (files.size() != 1) return false;
    return files[0].endsWithIgnoreCase(".wav") || 
           files[0].endsWithIgnoreCase(".mp3") || 
           files[0].endsWithIgnoreCase(".aiff");
}

void EQGraph::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/) {
    if (files.isEmpty()) return;
    
    juce::File file(files[0]);
    if (file.existsAsFile()) {
        proc.analyzer.analyzeFile(file);
        repaint();
    }
}
