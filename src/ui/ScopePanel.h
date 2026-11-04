#pragma once
#include <array>
#include <vector>
#include "../PluginProcessor.h"
#include "Theme.h"

namespace ui {
// チャンネルごとのオシロスコープ（12チャンネルずつページ表示）。
// 描画方式: 波形は自前のブレゼンハムで ARGB の juce::Image に直接書き込み、1回の drawImage で転送する
// （juce::Path の生成・ストローク・アンチエイリアスを使わない）。ラベル/グリッドだけ Graphics で描く。
// 非表示のタブでは波形取得も描画も止まり、波形が前回と同じなら再描画しない。
class ScopePanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int pageSize = _3HSPlugAudioProcessor::scopeChannelsPerChip; // 12
    static constexpr int captureSamples = 2048; // トリガー探索用に取得する長さ
    static constexpr int columns = 3, rows = 4;

    explicit ScopePanel(_3HSPlugAudioProcessor& p);
    ~ScopePanel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override { repaint(); }

private:
    void timerCallback() override;
    void setPage(int newPage);
    int numPages() const;
    void updateRange(bool showing);
    juce::Rectangle<int> gridArea() const;
    void renderWaveforms(juce::Rectangle<int> pixelBounds, float scale);
    juce::String channelTitle(int flat) const;
    juce::String channelSubtitle(int flat) const;

    _3HSPlugAudioProcessor& processor;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label pageLabel, gainLabel;
    juce::ComboBox gainBox;

    int page = 0;
    bool wasShowing = false;
    float gainSetting = 0.0f; // 0 = auto
    juce::Image image;

    struct Cell {
        std::array<float, captureSamples> raw {};
        std::array<float, captureSamples / 2> shown {};
        int shownCount = 0;
        bool valid = false;
        float autoPeak = 0.05f;
    };
    std::array<Cell, pageSize> cells;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopePanel)
};
} // namespace ui
