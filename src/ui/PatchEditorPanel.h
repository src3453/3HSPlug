// 選択したMIDIチャンネルのパッチをリアルタイムに編集する区画
#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Theme.h"
#include "DrumKitPanel.h"
#include "AlgorithmDiagram.h"

namespace ui {

// 名前付きのロータリーノブ
class Knob : public juce::Component
{
public:
    explicit Knob(const juce::String& name);
    void paint(juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    juce::String name;
};

// 1オペレーター分のパネル
class OperatorPanel : public juce::Component
{
public:
    explicit OperatorPanel(int index);
    void paint(juce::Graphics&) override;
    void resized() override;

    void setOperator(const Operator& op);
    void writeTo(Operator& op) const;
    void setRole(bool isCarrier, bool isFeedbackTarget);
    void lookAndFeelChanged() override { applyRoleColours(); }

    std::function<void()> onEdit;

private:
    void applyRoleColours();
    void drawWave(juce::Graphics&, juce::Rectangle<float>) const;
    void drawEnvelope(juce::Graphics&, juce::Rectangle<float>) const;
    juce::Rectangle<int> graphArea() const;

    static constexpr int kHeaderTop = 6;
    static constexpr int kHeaderHeight = 28;
    const int index;
    bool carrier = true;
    bool feedbackTarget = false;
    juce::ComboBox waveBox;
    Knob coarse { "Coarse" }, fine { "Fine" }, level { "Level" }, attack { "Attack" }, decay { "Decay" }, sustain { "Sustain" }, release { "Release" };
};

class PatchEditorPanel : public juce::Component
{
public:
    explicit PatchEditorPanel(_3HSPlugAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

    void setChannel(int midiChannel);
    int getChannel() const noexcept { return channel; }
    void refresh(); // タイマーから呼ぶ。プロセッサー側の変更を読み込む。

    std::function<void(int midiChannel)> onChannelChanged;
    void lookAndFeelChanged() override { updateStatus(); }

private:
    void loadFromProcessor();
    void pushEdit();
    void updateRoles();
    void updateStatus();
    void commitProgramEdit();
    juce::Rectangle<int> globalArea() const;
    juce::Rectangle<int> operatorsArea() const;

    static constexpr int kCaptionHeight = 14; // BANK/PATCH 見出しの高さ
    static constexpr int kGlobalHeight = kCaptionHeight + 3 * space::control + 2 * space::xs; // 3段: 名前と状態 / 経路図 / ボタン
    static constexpr int kOpGap = 2 * space::md;

    _3HSPlugAudioProcessor& processor;
    int channel = 1;
    Patch patch;
    uint32_t loadedRevision = ~0u;
    int loadedBank = -1, loadedProgram = -1;
    bool drumChannel = false;
    bool loading = false;

    juce::ComboBox channelBox;
    juce::ComboBox modeBox;
    juce::ComboBox fbDestBox;
    AlgorithmDiagram diagram;
    Knob feedback { "Feedback" };
    Knob keyShift { "Key shift" };
    juce::TextButton revertButton { "Revert" };
    juce::TextButton saveButton { "Save Bank" };
    juce::TextEditor bankEdit;  // バンク(0～127)。確定でバンクセレクト+プログラムチェンジを送る
    juce::TextEditor patchEdit; // パッチ(1～128)
    juce::Label bankCaption, patchCaption, colonLabel; // BANK / PATCH の見出しと区切りのコロン
    juce::Label programLabel;   // GM音色名（入力欄の下）
    juce::Label statusLabel;
    DrumKitPanel drumKit;
    juce::String saveMessage;
    uint32_t saveMessageFrames = 0;
    std::array<std::unique_ptr<OperatorPanel>, 8> operators;
};

} // namespace ui
