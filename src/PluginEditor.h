/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>  // JUCEヘッダーファイルはビルド時に生成されるので、このIntelliSenseエラーは無視する
#include "PluginProcessor.h"
//#include "OscilloscopeComponent.h"

//==============================================================================
/**
 * スクロール可能な16進ダンプビューアーコンポーネント
 */
class HexDumpViewer : public juce::Component
{
public:
    HexDumpViewer();
    ~HexDumpViewer() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // データを更新する
    void updateData(const std::vector<uint8_t>& data, uint32_t baseAddress = 0x400000);
    
    // マウスホイールでスクロール
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
private:
    std::vector<uint8_t> hexData;
    uint32_t baseAddr = 0x400000;
    int scrollOffset = 0;
    int bytesPerLine = 16;
    int lineHeight = 16;
    juce::Font monoFont;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HexDumpViewer)
};

//==============================================================================
/**
*/
class _3HSPlugAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    _3HSPlugAudioProcessorEditor (_3HSPlugAudioProcessor&);
    ~_3HSPlugAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    _3HSPlugAudioProcessor& audioProcessor;

    // パンポット表示用のUIコンポーネント
    struct PanKnob
    {
        float panValue = 0.0f; // -1.0 (L) ～ 1.0 (R)
    };
    std::vector<PanKnob> panKnobs;

    void timerCallback() override;
    uint64_t currentDisplayTick = 0; // 時間経過による減衰計算用
    
    // パンポット表示用描画関数（LR分離バー表示）
    void drawPanBars(juce::Graphics& g, int x, int y, int width, int height, int panL, int panR);

    // オシロスコープコンポーネント
    //std::unique_ptr<MultiOscilloscopeComponent> multiOscilloscope;
    
    // 16進ダンプビューアーコンポーネント
    std::unique_ptr<HexDumpViewer> hexDumpViewer;
    
    // オシロスコープデータ更新メソッド
    //void updateOscilloscopeData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_3HSPlugAudioProcessorEditor)
};
