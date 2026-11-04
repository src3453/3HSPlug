#pragma once

#include <JuceHeader.h>  // JUCEヘッダーファイルはビルド時に生成されるので、このIntelliSenseエラーは無視する
#include "PluginProcessor.h"
#include "ui/Theme.h"
#include "ui/Panels.h"
#include "ui/PatchEditorPanel.h"
#include "ui/ChannelStatusPanel.h"

//==============================================================================
// 区画:
//   タイトルバー（Panic / Settings）
//   チャンネルアクティビティ（クリックでパッチエディタのチャンネルを選ぶ）
//   パッチエディタ（選んだチャンネルの現在のパッチ。編集は発音中の音にも即反映）
//   ボイス状態（チップごとのFM/PCMの割り当て）
//   マスター（音量フェーダーとステレオメーター）
//   設定（オーバーレイ）
class _3HSPlugAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ChangeListener
{
public:
    explicit _3HSPlugAudioProcessorEditor(_3HSPlugAudioProcessor&);
    ~_3HSPlugAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override; // テーマやフォントが変わった
    void selectChannel(int midiChannel);
    void applyUIScale(float scale);

    _3HSPlugAudioProcessor& audioProcessor;
    ui::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::Component content;
    ui::TitleBar titleBar;
    juce::Component synthPage;
    ui::ChannelStatusPanel channelStatus;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    ui::ChannelActivityPanel channelActivity;
    ui::PatchEditorPanel patchEditor;
    ui::VoiceStatusPanel voiceStatus;
    ui::MasterSection master;
    ui::SettingsOverlay settings;
    ui::FocusRingOverlay focusRing { content };

    int laidOutChips = 0;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(_3HSPlugAudioProcessorEditor)
};
