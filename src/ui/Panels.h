// 3HSPlug UI の各区画（タイトルバー、チャンネルアクティビティ、ボイス状態、マスター、設定）
#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Theme.h"

namespace ui {

//==============================================================================
class TitleBar : public juce::Component
{
public:
    TitleBar();
    void paint(juce::Graphics&) override;
    void resized() override;

    std::function<void()> onPanic;
    std::function<void()> onSettings;

private:
    juce::TextButton panicButton { "Panic" };
    juce::TextButton settingsButton { "Settings" };
};

//==============================================================================
// 16 MIDIチャンネルの発音レベル。クリックでパッチエディタのチャンネルを選ぶ。
// GSのドットマトリクス/テキストを受け取ると、その間はLCD表示に切り替わる。
class ChannelActivityPanel : public juce::Component
{
public:
    explicit ChannelActivityPanel(_3HSPlugAudioProcessor&);
    void update();
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void focusGained(FocusChangeType) override;
    void focusLost(FocusChangeType) override;

    void setSelectedChannel(int midiChannel) { selectedChannel = midiChannel; repaint(); }
    std::function<void(int midiChannel)> onChannelSelected;

private:
    juce::Rectangle<int> columnsArea() const;
    int channelAt(juce::Point<int>) const;

    _3HSPlugAudioProcessor& processor;
    int selectedChannel = 1;
    int hoverChannel = 0; // update() が毎フレーム描き直す
    float levels[16] {};
    bool gsDots[16][16] {};
    juce::String gsText;
    bool showGsDots = false;
    bool showGsText = false;
    bool showFocusRing = false; // キーボードでフォーカスしたときだけ
};

//==============================================================================
// チップごとのFM 8ボイスとPCM 4チャンネルの割り当て状況
class VoiceStatusPanel : public juce::Component
{
public:
    explicit VoiceStatusPanel(_3HSPlugAudioProcessor&);
    void paint(juce::Graphics&) override;
    int getPreferredHeight() const;

private:
    _3HSPlugAudioProcessor& processor;
};

//==============================================================================
// マスター音量フェーダーとステレオメーター
class MasterSection : public juce::Component
{
public:
    explicit MasterSection(_3HSPlugAudioProcessor&);
    void update();
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::Rectangle<int> meterArea() const;

    _3HSPlugAudioProcessor& processor;
    juce::Slider fader;
    juce::Label valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    float peak[2] {};
    float rms[2] {};
    float hold[2] {};
    int holdFrames[2] {};
    bool clipped = false;
};

//==============================================================================
// 設定ボタンで開くシステム設定（エディタ全体に重ねる）
class SettingsOverlay : public juce::Component
{
public:
    explicit SettingsOverlay(_3HSPlugAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;

    void open();
    void close();
    void update(); // 表示中に情報を更新する
    std::function<void(bool)> onShowDebugTabChanged;

private:
    juce::Rectangle<int> panelBounds() const;

    _3HSPlugAudioProcessor& processor;
    juce::TextButton closeButton { "Close" };
    juce::Label numChipsLabel;
    juce::ComboBox numChipsBox;
    juce::Label allocationLabel;
    juce::ComboBox allocationBox;
    juce::ToggleButton pcOverrideToggle { "Override Program Change" };
    juce::Label pcBankLabel, pcProgramLabel;
    juce::Slider pcBankSlider, pcProgramSlider;
    juce::TextButton gmResetButton { "GM Reset" };
    juce::TextButton panicButton { "All Notes Off" };
    juce::Label fontLabel;
    juce::ComboBox fontBox;
    juce::Label scaleLabel;
    juce::ComboBox scaleBox;
    juce::Label themeLabel;
    juce::ComboBox themeBox;
    juce::ToggleButton showDebugToggle { "Show Debug tab" };
    juce::ToggleButton consoleLogToggle { "Log to console (stderr)" };
    juce::TextButton themesFolderButton { "Themes folder..." };
    juce::StringArray themeNames, themeErrors;
    void reloadThemeList();
    void lookAndFeelChanged() override;
    juce::TextButton pcmFolderButton { "PCM folder..." };
    juce::TextButton patchFolderButton { "Patch folder..." };
    std::unique_ptr<juce::FileChooser> folderChooser;
    void chooseAssetFolder(bool pcm);
    juce::Label infoLabel;
};

//==============================================================================
// 標準の部品（ボタン、コンボボックス、ノブ）にキーボードでフォーカスしたときのリング。
// マウスのクリックでフォーカスしたときは出さない（:focus-visible と同じ考え方）。
class FocusRingOverlay : public juce::Component, private juce::FocusChangeListener
{
public:
    explicit FocusRingOverlay(juce::Component& root);
    ~FocusRingOverlay() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void refresh(); // 対象が動いたり隠れたりしたときのためにタイマーから呼ぶ
    std::function<void(juce::Component*)> onKeyboardFocus; // リングの対象が変わった（なければ nullptr）

private:
    void globalFocusChanged(juce::Component* focused) override;

    juce::Component& root;
    juce::Component::SafePointer<juce::Component> target;
    juce::Rectangle<int> lastBounds;
};

} // namespace ui
