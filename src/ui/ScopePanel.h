#pragma once
#include <array>
#include <vector>
#include "../PluginProcessor.h"
#include "Theme.h"

namespace ui {
// チャンネルごとのオシロスコープ（12チャンネルずつページ表示）。
// 表示切替で「MIDIチャンネル別」（各ボイスを割り当て先MIDIチャンネルごとに加算、16セル）にもできる。
// 描画方式: 波形は自前のブレゼンハムで ARGB の juce::Image に直接書き込み、1回の drawImage で転送する
// （juce::Path の生成・ストローク・アンチエイリアスを使わない）。ラベル/グリッドだけ Graphics で描く。
// 非表示のタブでは波形取得も描画も止まり、波形が前回と同じなら再描画しない。
class ScopePanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int pageSize = _3HSPlugAudioProcessor::scopeChannelsPerChip; // 12
    static constexpr int captureSamples = 2048; // トリガー探索用に取得する長さ
    static constexpr int midiCells = 16; // MIDIチャンネル別ミックス表示は16セル（4x4）
    static constexpr int maxCells = midiCells > pageSize ? midiCells : pageSize;

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
    bool midiMode() const noexcept { return viewMode == ViewMode::midiChannel; }
    int cellCount() const noexcept { return midiMode() ? midiCells : pageSize; }
    int columns() const noexcept { return midiMode() ? 4 : 3; }
    int rows() const noexcept { return 4; }
    juce::Rectangle<int> gridArea() const;
    void renderWaveforms(juce::Rectangle<int> pixelBounds, float scale);
    juce::String channelTitle(int flat) const;
    juce::String channelSubtitle(int flat) const;
    juce::String midiChannelSubtitle(int midiCh) const;
    int midiChannelFor(int flat) const;
    juce::Colour cellColour(int cellIndex) const;

    _3HSPlugAudioProcessor& processor;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label pageLabel, gainLabel;
    juce::ComboBox gainBox, fpsBox, dcBox, colourBox, viewBox;
    juce::Label fpsLabel, dcLabel, colourLabel, viewLabel;
    enum class ViewMode { voice = 1, midiChannel };
    ViewMode viewMode = ViewMode::voice;
    enum class ColourMode { white = 1, voice, midiChannel };
    ColourMode colourMode = ColourMode::white;
    std::array<juce::Colour, maxCells> cellColours;
    bool removeDc = false;

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
    std::array<Cell, maxCells> cells;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopePanel)
};
} // namespace ui
