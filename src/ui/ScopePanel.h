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
    static constexpr int captureSamples = _3HSPlugAudioProcessor::scopeRingSize; // 取得長の上限（リングバッファ長）
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

    enum class ViewMode { voice = 1, midiChannel };
    enum class ColourMode { white = 1, voice, midiChannel };
    enum class DcMode { off = 0, mean = 1, highPass = 2 }; // 設定値と対応（旧: 0=なし,1=平均値）
    _3HSPlugAudioProcessor& processor;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label pageLabel;
    juce::TextButton optionsButton { "Options" };
    // Options ポップアップ（View / Window / Gain / FPS / DC removal / Colour）
    void showOptionsMenu();
    void setGainId(int id);
    void setWindowId(int id);
    void setViewMode(ViewMode m);
    void setColourMode(ColourMode m);
    void setFpsId(int id);
    void setDcMode(DcMode m);
    int gainId = 1;   // 1 = Auto, 2.. = x1, x2, ...
    int windowId = 3; // windowChoices の1始まり添字
    int fpsId = 4;    // fpsChoices の1始まり添字
    int windowMs = 20; // 表示窓の時間幅(ms)
    int captureCount() const;
    ViewMode viewMode = ViewMode::voice;
    ColourMode colourMode = ColourMode::white;
    std::array<juce::Colour, maxCells> cellColours;
    DcMode dcMode = DcMode::off;
    // Mean用DC補正値のフレーム間平滑化（指数移動平均）。1.0で平滑化なし、小さいほど滑らか。
    static constexpr float dcSmoothing = 0.1f;

    int page = 0;
    bool wasShowing = false;
    float gainSetting = 0.0f; // 0 = auto
    juce::Image image;

    struct Cell {
        std::vector<float> raw = std::vector<float>(captureSamples, 0.0f);
        std::vector<float> shown = std::vector<float>(captureSamples / 2, 0.0f);
        int shownCount = 0;
        bool valid = false;
        juce::IIRFilter dcFilter; // High-pass用（チャンネルごと）
        float dcSmoothed = 0.0f;  // Mean用に平滑化したDC補正値
        bool dcSmoothedValid = false;
        float autoPeak = 0.05f;
    };
    std::array<Cell, maxCells> cells;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopePanel)
};
} // namespace ui
