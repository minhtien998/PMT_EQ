#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class EQGraph : public juce::Component, 
                public juce::Timer,
                public juce::FileDragAndDropTarget // <--- BỔ SUNG DÒNG NÀY ĐỂ HẾT LỖI C3668
{
public:
    EQGraph(SmartEQAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    // Tương tác chuột
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Drag & Drop File (Sẽ override từ FileDragAndDropTarget)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    SmartEQAudioProcessor& proc;
    int draggedBand = -1;

    // Các hàm hỗ trợ chuyển đổi tọa độ
    float xToFreq(float x) const;
    float freqToX(float f) const;
    float yToGain(float y) const;
    float gainToY(float g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQGraph)
};
