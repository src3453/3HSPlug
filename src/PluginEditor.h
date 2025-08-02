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

    void timerCallback() override;
    uint64_t currentDisplayTick = 0; // 時間経過による減衰計算用

    // オシロスコープコンポーネント
    //std::unique_ptr<MultiOscilloscopeComponent> multiOscilloscope;
    
    // オシロスコープデータ更新メソッド
    //void updateOscilloscopeData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_3HSPlugAudioProcessorEditor)
};
