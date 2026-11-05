#include "ScopePanel.h"
#include "ScopeTrigger.h"
#include "UserSettings.h"
#include <cstdlib>
#include <cstring>

namespace ui {
namespace {
constexpr int headerHeight = 36;
constexpr int fpsChoices[] { 10, 15, 20, 30, 60, 120 };
constexpr const char* kFpsKey = "scopeFps";
constexpr const char* kColourKey = "scopeColourMode"; // 1 = 白, 2 = ボイスごと, 3 = MIDIチャンネルごと
constexpr const char* kViewKey = "scopeViewMode"; // 1 = ボイスごと, 2 = MIDIチャンネルごと(ミックス)
constexpr const char* kDcKey = "scopeDcRemoval"; // 0 = なし, 1 = 平均値ベース

// 1ピクセルを書く（範囲外は無視）。コンポーネントの背景は透明なので、不透明な色をそのまま書き込む。
struct Canvas {
    juce::Image::BitmapData data;
    int w, h;
    juce::PixelARGB px;
    inline void plot(int x, int y) noexcept
    {
        if (static_cast<unsigned>(x) < static_cast<unsigned>(w) && static_cast<unsigned>(y) < static_cast<unsigned>(h))
            reinterpret_cast<juce::PixelARGB*>(data.getPixelPointer(x, y))->set(px);
    }
    // ブレゼンハム。クリップ矩形 [y0c, y1c) に収める
    void line(int x0, int y0, int x1, int y1, int thickness, int minY, int maxY) noexcept
    {
        const int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
        const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            for (int t = 0; t < thickness; ++t) {
                const int yy = y0 + t;
                if (yy >= minY && yy < maxY) plot(x0, yy);
            }
            if (x0 == x1 && y0 == y1) break;
            const int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
};
} // namespace

ScopePanel::ScopePanel(_3HSPlugAudioProcessor& p) : processor(p)
{
    setOpaque(false);
    setTitle("Per-channel oscilloscope");
    for (auto* b : { &prevButton, &nextButton }) {
        addAndMakeVisible(*b);
        b->setWantsKeyboardFocus(true);
    }
    prevButton.setTooltip("Previous page");
    nextButton.setTooltip("Next page");
    prevButton.onClick = [this] { setPage(page - 1); };
    nextButton.onClick = [this] { setPage(page + 1); };

    addAndMakeVisible(pageLabel);
    setLabelFont(pageLabel, type::small, true, true);
    pageLabel.setJustificationType(juce::Justification::centred);
    pageLabel.setColour(juce::Label::textColourId, colours::text);

    addAndMakeVisible(gainLabel);
    gainLabel.setText("Gain", juce::dontSendNotification);
    setLabelFont(gainLabel, type::small);
    gainLabel.setColour(juce::Label::textColourId, colours::textDim);
    gainLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(gainBox);
    gainBox.addItem("Auto", 1);
    int id = 2;
    for (int g : { 1, 2, 4, 8, 16, 32 }) gainBox.addItem("x" + juce::String(g), id++);
    gainBox.setSelectedId(1, juce::dontSendNotification);
    gainBox.onChange = [this] {
        const int sel = gainBox.getSelectedId();
        gainSetting = sel <= 1 ? 0.0f : static_cast<float>(1 << (sel - 2));
        repaint();
    };

    auto setupLabel = [this](juce::Label& l, const char* text) {
        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        setLabelFont(l, type::small);
        l.setColour(juce::Label::textColourId, colours::textDim);
        l.setJustificationType(juce::Justification::centredRight);
    };
    setupLabel(fpsLabel, "FPS");
    setupLabel(dcLabel, "DC");
    setupLabel(colourLabel, "Colour");
    setupLabel(viewLabel, "View");

    addAndMakeVisible(viewBox);
    viewBox.addItem("Voice", 1);
    viewBox.addItem("MIDI channel", 2);
    viewMode = static_cast<ViewMode>(juce::jlimit(1, 2, ui::userSettings().getIntValue(kViewKey, 1)));
    viewBox.setSelectedId(static_cast<int>(viewMode), juce::dontSendNotification);
    viewBox.onChange = [this] {
        viewMode = static_cast<ViewMode>(juce::jlimit(1, 2, viewBox.getSelectedId()));
        ui::userSettings().setValue(kViewKey, static_cast<int>(viewMode));
        ui::userSettings().saveIfNeeded();
        for (auto& c : cells) { c.valid = false; c.autoPeak = 0.05f; }
        if (wasShowing) updateRange(true);
        resized();
        repaint();
    };

    addAndMakeVisible(colourBox);
    colourBox.addItem("White", 1);
    colourBox.addItem("Per voice", 2);
    colourBox.addItem("Per MIDI channel", 3);
    colourMode = static_cast<ColourMode>(juce::jlimit(1, 3, ui::userSettings().getIntValue(kColourKey, 1)));
    colourBox.setSelectedId(static_cast<int>(colourMode), juce::dontSendNotification);
    colourBox.onChange = [this] {
        colourMode = static_cast<ColourMode>(juce::jlimit(1, 3, colourBox.getSelectedId()));
        ui::userSettings().setValue(kColourKey, static_cast<int>(colourMode));
        ui::userSettings().saveIfNeeded();
        repaint();
    };

    addAndMakeVisible(fpsBox);
    const int savedFps = ui::userSettings().getIntValue(kFpsKey, 30);
    int fpsId = 4; // 30
    for (int i = 0; i < static_cast<int>(std::size(fpsChoices)); ++i) {
        fpsBox.addItem(juce::String(fpsChoices[i]), i + 1);
        if (fpsChoices[i] == savedFps) fpsId = i + 1;
    }
    fpsBox.setSelectedId(fpsId, juce::dontSendNotification);
    fpsBox.onChange = [this] {
        const int fps = fpsChoices[juce::jlimit(1, static_cast<int>(std::size(fpsChoices)), fpsBox.getSelectedId()) - 1];
        startTimerHz(fps);
        ui::userSettings().setValue(kFpsKey, fps);
        ui::userSettings().saveIfNeeded();
    };

    addAndMakeVisible(dcBox);
    dcBox.addItem("Off", 1);
    dcBox.addItem("Mean", 2);
    removeDc = ui::userSettings().getIntValue(kDcKey, 0) == 1;
    dcBox.setSelectedId(removeDc ? 2 : 1, juce::dontSendNotification);
    dcBox.onChange = [this] {
        removeDc = dcBox.getSelectedId() == 2;
        for (auto& c : cells) c.valid = false; // 次の取得で作り直す
        ui::userSettings().setValue(kDcKey, removeDc ? 1 : 0);
        ui::userSettings().saveIfNeeded();
    };

    startTimerHz(fpsChoices[fpsId - 1]);
}

ScopePanel::~ScopePanel()
{
    stopTimer();
    processor.setScopeRange(0, 0);
    processor.setScopeMidiMix(false);
}

int ScopePanel::numPages() const
{
    return juce::jmax(1, (processor.getNumChips() * pageSize + pageSize - 1) / pageSize);
}

void ScopePanel::setPage(int newPage)
{
    newPage = juce::jlimit(0, numPages() - 1, newPage);
    if (newPage == page) return;
    page = newPage;
    for (auto& c : cells) c.valid = false;
    if (wasShowing) updateRange(true);
    repaint();
}

void ScopePanel::updateRange(bool showing)
{
    processor.setScopeMidiMix(midiMode());
    if (midiMode())
        processor.setScopeRange(0, showing ? midiCells : 0);
    else
        processor.setScopeRange(page * pageSize, showing ? pageSize : 0);
}

void ScopePanel::resized()
{
    auto area = getLocalBounds().reduced(space::sm);
    auto bar = area.removeFromTop(headerHeight - space::sm);
    auto right = bar.removeFromRight(760);
    auto place = [&right](juce::Label& l, juce::ComboBox& b, int boxW) {
        b.setBounds(right.removeFromRight(boxW).reduced(0, 2));
        l.setBounds(right.removeFromRight(juce::jmax(40, l.getFont().getStringWidth(l.getText()) + 8)));
    };
    place(colourLabel, colourBox, 140);
    place(dcLabel, dcBox, 80);
    place(fpsLabel, fpsBox, 70);
    place(gainLabel, gainBox, 90);
    prevButton.setBounds(bar.removeFromLeft(36).reduced(0, 2));
    pageLabel.setBounds(bar.removeFromLeft(210));
    nextButton.setBounds(bar.removeFromLeft(36).reduced(0, 2));
    // 表示切替はページ送りの右隣（左側の空き）に置く
    viewLabel.setBounds(bar.removeFromLeft(48));
    viewBox.setBounds(bar.removeFromLeft(130).reduced(0, 2));
    for (auto* b : { &prevButton, &nextButton }) b->setVisible(!midiMode());
}

juce::Rectangle<int> ScopePanel::gridArea() const
{
    return getLocalBounds().reduced(space::sm).withTrimmedTop(headerHeight - space::sm + space::xs);
}

juce::String ScopePanel::channelTitle(int flat) const
{
    const int ch = flat % pageSize;
    return ch < 8 ? "FM " + juce::String(ch + 1) : "PCM " + juce::String(ch - 7);
}

int ScopePanel::midiChannelFor(int flat) const
{
    const int chip = flat / pageSize, ch = flat % pageSize;
    int midiCh = 0;
    if (ch < 8) {
        const auto& slots = processor.getVoiceSlots();
        const size_t idx = static_cast<size_t>(chip * _3HSPlugAudioProcessor::numVoices + ch);
        if (idx < slots.size()) midiCh = slots[idx].midiChannel;
    } else {
        const size_t idx = static_cast<size_t>(chip * 4 + (ch - 8));
        if (idx < processor.drumPcmChannelStates.size()) midiCh = processor.drumPcmChannelStates[idx].midiChannel;
    }
    return midiCh >= 1 && midiCh <= 16 ? midiCh : 0;
}

juce::Colour ScopePanel::cellColour(int cellIndex) const
{
    switch (colourMode) {
        case ColourMode::voice: return colours::forChannel(cellIndex % 16);
        case ColourMode::midiChannel: {
            const int midiCh = midiMode() ? cellIndex + 1 : midiChannelFor(page * pageSize + cellIndex);
            return midiCh > 0 ? colours::forChannel(midiCh - 1) : colours::textDim; // 未使用は灰色
        }
        case ColourMode::white: break;
    }
    return juce::Colours::white;
}

juce::String ScopePanel::midiChannelSubtitle(int midiCh) const
{
    if (processor.isDrumChannel(midiCh)) return "Drum kit";
    return juce::MidiMessage::getGMInstrumentName(processor.getCurrentProgramForChannel(midiCh));
}

juce::String ScopePanel::channelSubtitle(int flat) const
{
    const int midiCh = midiChannelFor(flat);
    if (midiCh == 0) return "--";
    const auto num = juce::String(midiCh).paddedLeft('0', 2);
    return "CH" + num + " " + midiChannelSubtitle(midiCh);
}

// メッセージスレッド: 表示中ページの波形を取得してトリガーをかける。変化があれば再描画する
void ScopePanel::timerCallback()
{
    const bool showing = isShowing();
    const int nCells = cellCount();
    if (showing != wasShowing) {
        wasShowing = showing;
        updateRange(showing);
        for (auto& c : cells) c.valid = false;
    }
    if (!showing) return;

    if (page >= numPages()) setPage(numPages() - 1);
    const auto text = midiMode() ? juce::String("MIDI channels 1-16 (mixed)")
                                 : "Chip " + juce::String(page + 1) + " / " + juce::String(numPages())
                                       + "   (ch " + juce::String(page * pageSize + 1) + "-" + juce::String(page * pageSize + pageSize) + ")";
    if (pageLabel.getText() != text) pageLabel.setText(text, juce::dontSendNotification);
    prevButton.setEnabled(page > 0);
    nextButton.setEnabled(page < numPages() - 1);

    bool changed = false;
    for (int i = 0; i < nCells; ++i) {
        auto& c = cells[static_cast<size_t>(i)];
        const bool ok = processor.readScopeWindow(midiMode() ? i : page * pageSize + i, c.raw.data(), captureSamples);
        if (!ok) {
            if (c.valid) { c.valid = false; changed = true; }
            continue;
        }
        if (removeDc) { // 平均値ベースのDC除去（トリガー判定にも効く）
            double sum = 0.0;
            for (float v : c.raw) sum += v;
            const float mean = static_cast<float>(sum / captureSamples);
            for (float& v : c.raw) v -= mean;
        }
        size_t count = 0;
        const size_t start = findScopeTrigger(c.raw.data(), captureSamples, count);
        if (!c.valid || static_cast<int>(count) != c.shownCount
            || std::memcmp(c.shown.data(), c.raw.data() + start, count * sizeof(float)) != 0) {
            std::memcpy(c.shown.data(), c.raw.data() + start, count * sizeof(float));
            c.shownCount = static_cast<int>(count);
            changed = true;
        }
        c.valid = true;
    }
    if (changed) repaint(gridArea());
}

// 波形を ARGB イメージへ直接ラスタライズする（Path を使わない）
void ScopePanel::renderWaveforms(juce::Rectangle<int> px, float scale)
{
    if (image.getWidth() != px.getWidth() || image.getHeight() != px.getHeight())
        image = juce::Image(juce::Image::ARGB, px.getWidth(), px.getHeight(), true);
    else
        image.clear(image.getBounds());

    const int cols = columns(), nRows = rows();
    const int cw = px.getWidth() / cols, ch = px.getHeight() / nRows;
    const int thickness = scale >= 1.75f ? 2 : 1;
    Canvas canvas { juce::Image::BitmapData(image, juce::Image::BitmapData::readWrite), px.getWidth(), px.getHeight(), {} };

    for (int i = 0; i < cellCount(); ++i) {
        auto& cell = cells[static_cast<size_t>(i)];
        if (!cell.valid || cell.shownCount < 2) continue;
        const int col = i % cols, row = i / cols;
        const int x0 = col * cw, y0 = row * ch;
        const float centre = static_cast<float>(y0) + ch * 0.5f;

        float gain = gainSetting;
        if (gain <= 0.0f) { // Auto: 速く上がり、ゆっくり下がるピーク追従
            float peak = 0.0f;
            for (int s = 0; s < cell.shownCount; ++s) peak = juce::jmax(peak, std::abs(cell.shown[static_cast<size_t>(s)]));
            cell.autoPeak = juce::jmax(peak, cell.autoPeak * 0.95f, 1.0f / 256.0f);
            gain = 0.9f / cell.autoPeak;
        }
        const float amp = gain * ch * 0.45f;
        canvas.px = cellColours[static_cast<size_t>(i)].getPixelARGB();

        const float xScale = static_cast<float>(cw - 1) / static_cast<float>(cell.shownCount - 1);
        const int minY = y0, maxY = y0 + ch;
        int px0 = x0, py0 = juce::roundToInt(centre - cell.shown[0] * amp);
        for (int s = 1; s < cell.shownCount; ++s) {
            const int px1 = x0 + static_cast<int>(static_cast<float>(s) * xScale);
            const int py1 = juce::jlimit(minY, maxY - 1, juce::roundToInt(centre - cell.shown[static_cast<size_t>(s)] * amp));
            if (px1 != px0 || py1 != py0) canvas.line(px0, juce::jlimit(minY, maxY - 1, py0), px1, py1, thickness, minY, maxY);
            px0 = px1;
            py0 = py1;
        }
    }
}

void ScopePanel::paint(juce::Graphics& g)
{
    drawSection(g, getLocalBounds(), {});
    const auto grid = gridArea();
    g.setColour(juce::Colours::black);
    g.fillRect(grid);

    const int nCells = cellCount(), cols = columns(), nRows = rows();
    for (int i = 0; i < nCells; ++i) cellColours[static_cast<size_t>(i)] = cellColour(i);

    // 物理ピクセル解像度でラスタライズして、そのまま1回で転送する
    const float scale = juce::jmax(1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int pw = juce::roundToInt(grid.getWidth() * scale), ph = juce::roundToInt(grid.getHeight() * scale);
    if (pw > 0 && ph > 0) {
        renderWaveforms({ 0, 0, pw, ph }, scale);
        g.drawImage(image, grid.toFloat(), juce::RectanglePlacement::stretchToFit);
    }

    // グリッド線とラベル
    const float cw = grid.getWidth() / static_cast<float>(cols), chh = grid.getHeight() / static_cast<float>(nRows);
    g.setColour(colours::outline);
    for (int c = 1; c < cols; ++c)
        g.drawVerticalLine(grid.getX() + juce::roundToInt(c * cw), static_cast<float>(grid.getY()), static_cast<float>(grid.getBottom()));
    for (int r = 1; r < nRows; ++r)
        g.drawHorizontalLine(grid.getY() + juce::roundToInt(r * chh), static_cast<float>(grid.getX()), static_cast<float>(grid.getRight()));
    g.drawRect(grid, 1);

    for (int i = 0; i < nCells; ++i) {
        const int flat = page * pageSize + i;
        const juce::Rectangle<float> cell(grid.getX() + (i % cols) * cw, grid.getY() + (i / cols) * chh, cw, chh);
        const auto colour = cellColours[static_cast<size_t>(i)];
        g.setColour(colour);
        g.setFont(monoFont(type::body));
        g.drawText(midiMode() ? "CH" + juce::String(i + 1).paddedLeft('0', 2) : channelTitle(flat),
                   cell.reduced(6, 3).removeFromTop(17), juce::Justification::topLeft, false);
        g.setFont(monoFont(type::caption));
        g.setColour(colour.withAlpha(0.85f));
        g.drawText(midiMode() ? midiChannelSubtitle(i + 1) : channelSubtitle(flat), cell.reduced(6, 3).withTrimmedTop(17).removeFromTop(14), juce::Justification::topLeft, true);
    }
}
} // namespace ui
