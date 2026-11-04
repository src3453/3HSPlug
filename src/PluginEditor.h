#pragma once

#include <JuceHeader.h>  // JUCEヘッダーファイルはビルド時に生成されるので、このIntelliSenseエラーは無視する
#include "PluginProcessor.h"
#include "ui/Theme.h"
#include "ui/Panels.h"
#include "ui/PatchEditorPanel.h"

//==============================================================================
// 区画:
//   タイトルバー（Panic / Settings）
//   チャンネルアクティビティ（クリックでパッチエディタのチャンネルを選ぶ）
//   パッチエディタ（選んだチャンネルの現在のパッチ。編集は発音中の音にも即反映）
//   ボイス状態（チップごとのFM/PCMの割り当て）
//   マスター（音量フェーダーとステレオメーター）
//   設定（オーバーレイ）
class _3HSPlugAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit _3HSPlugAudioProcessorEditor(_3HSPlugAudioProcessor&);
    ~_3HSPlugAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectChannel(int midiChannel);

    _3HSPlugAudioProcessor& audioProcessor;
    ui::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 600 };

    ui::TitleBar titleBar;
    ui::ChannelActivityPanel channelActivity;
    ui::PatchEditorPanel patchEditor;
    ui::VoiceStatusPanel voiceStatus;
    ui::MasterSection master;
    ui::SettingsOverlay settings;

    int laidOutChips = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(_3HSPlugAudioProcessorEditor)
};
