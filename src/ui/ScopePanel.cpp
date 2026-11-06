#include "ScopePanel.h"
#include "ScopeTrigger.h"
#include "UserSettings.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>

namespace ui {
namespace {
constexpr int headerHeight = space::control + space::sm;
constexpr int fpsChoices[] { 10, 15, 20, 30, 60, 120 };
constexpr const char* kFpsKey = "scopeFps";
constexpr const char* kColourKey = "scopeColourMode"; // 1 = 単色（テーマの文字色）, 2 = ボイスごと, 3 = MIDIチャンネルごと
constexpr const char* kViewKey = "scopeViewMode"; // 1 = ボイスごと, 2 = MIDIチャンネルごと(ミックス)
constexpr const char* kDcKey = "scopeDcRemoval"; // 0 = なし, 1 = 平均値ベース, 2 = ハイパス, 3 = Max-Min
constexpr const char* kDcSmoothKey = "scopeDcSmoothing"; // 0 = なし, 1 = 弱, 2 = 強
constexpr const char* kGainKey = "scopeGainId"; // ComboBox の ID（1 = Auto, 2.. = x1, x2, ...）
constexpr const char* kWindowKey = "scopeWindowMs"; // 表示窓の時間幅(ms)
constexpr int windowChoices[] { 5, 10, 20, 40, 80 };

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

// Options 用のウィジェットパネル。ラベル + 選択ボックスを縦に並べる（テーマは LookAndFeel から適用される）。
class OptionsPanel : public juce::Component
{
public:
    struct Row {
        const char* label;
        juce::StringArray items;
        int selectedId; // 1始まり
        std::function<void(int)> onSelect;
    };
    explicit OptionsPanel(std::vector<Row> r) : rows(std::move(r))
    {
        for (auto& row : rows) {
            auto& e = widgets.emplace_back();
            e.label.setText(row.label, juce::dontSendNotification);
            setLabelFont(e.label, type::small, false, false, true);
            e.label.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(e.label);
            for (int i = 0; i < row.items.size(); ++i) e.box.addItem(row.items[i], i + 1);
            e.box.setSelectedId(row.selectedId, juce::dontSendNotification);
            auto cb = row.onSelect;
            auto* boxPtr = &e.box;
            e.box.onChange = [cb, boxPtr] { cb(boxPtr->getSelectedId()); };
            addAndMakeVisible(e.box);
        }
        setSize(260, space::md * 2 + static_cast<int>(rows.size()) * (space::control + space::xs));
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced(space::md);
        for (auto& e : widgets) {
            auto line = area.removeFromTop(space::control);
            area.removeFromTop(space::xs);
            e.label.setBounds(line.removeFromLeft(90));
            e.box.setBounds(line);
        }
    }
private:
    struct Widgets { juce::Label label; juce::ComboBox box; };
    std::vector<Row> rows;
    std::deque<Widgets> widgets; // 要素のアドレスを安定させる
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

    addAndMakeVisible(optionsButton);
    optionsButton.setTooltip("Scope options (View, Window, Gain, FPS, DC removal, DC smoothing, Colour)");
    optionsButton.setWantsKeyboardFocus(true);
    optionsButton.onClick = [this] { showOptionsMenu(); };

    {
        auto& s = ui::userSettings();
        gainId = juce::jlimit(1, 7, s.getIntValue(kGainKey, 1));
        gainSetting = gainId <= 1 ? 0.0f : static_cast<float>(1 << (gainId - 2));

        const int savedWin = s.getIntValue(kWindowKey, 20);
        windowId = 3;
        for (int i = 0; i < static_cast<int>(std::size(windowChoices)); ++i)
            if (windowChoices[i] == savedWin) windowId = i + 1;
        windowMs = windowChoices[windowId - 1];

        viewMode = static_cast<ViewMode>(juce::jlimit(1, 2, s.getIntValue(kViewKey, 1)));
        colourMode = static_cast<ColourMode>(juce::jlimit(1, 3, s.getIntValue(kColourKey, 1)));
        dcMode = static_cast<DcMode>(juce::jlimit(0, 3, s.getIntValue(kDcKey, 0)));
        dcSmoothId = juce::jlimit(1, 3, s.getIntValue(kDcSmoothKey, 3));

        const int savedFps = s.getIntValue(kFpsKey, 30);
        fpsId = 4; // 30
        for (int i = 0; i < static_cast<int>(std::size(fpsChoices)); ++i)
            if (fpsChoices[i] == savedFps) fpsId = i + 1;
    }

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
    for (auto& c : cells) { c.valid = false; c.dcFilter.reset(); }
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

void ScopePanel::setGainId(int id)
{
    gainId = juce::jlimit(1, 7, id);
    gainSetting = gainId <= 1 ? 0.0f : static_cast<float>(1 << (gainId - 2));
    ui::userSettings().setValue(kGainKey, gainId);
    ui::userSettings().saveIfNeeded();
    repaint();
}

void ScopePanel::setWindowId(int id)
{
    windowId = juce::jlimit(1, static_cast<int>(std::size(windowChoices)), id);
    windowMs = windowChoices[windowId - 1];
    ui::userSettings().setValue(kWindowKey, windowMs);
    ui::userSettings().saveIfNeeded();
    for (auto& c : cells) { c.valid = false; c.shownCount = 0; }
    repaint();
}

void ScopePanel::setViewMode(ViewMode m)
{
    viewMode = m;
    ui::userSettings().setValue(kViewKey, static_cast<int>(viewMode));
    ui::userSettings().saveIfNeeded();
    for (auto& c : cells) { c.valid = false; c.autoPeak = 0.05f; }
    if (wasShowing) updateRange(true);
    resized();
    repaint();
}

void ScopePanel::setColourMode(ColourMode m)
{
    colourMode = m;
    ui::userSettings().setValue(kColourKey, static_cast<int>(colourMode));
    ui::userSettings().saveIfNeeded();
    repaint();
}

void ScopePanel::setFpsId(int id)
{
    fpsId = juce::jlimit(1, static_cast<int>(std::size(fpsChoices)), id);
    const int fps = fpsChoices[fpsId - 1];
    startTimerHz(fps);
    ui::userSettings().setValue(kFpsKey, fps);
    ui::userSettings().saveIfNeeded();
}

void ScopePanel::setDcMode(DcMode m)
{
    dcMode = m;
    for (auto& c : cells) { c.valid = false; c.dcFilter.reset(); c.dcSmoothedValid = false; } // 次の取得で作り直す
    ui::userSettings().setValue(kDcKey, static_cast<int>(dcMode));
    ui::userSettings().saveIfNeeded();
}

void ScopePanel::setDcSmoothId(int id)
{
    dcSmoothId = juce::jlimit(1, 3, id);
    for (auto& c : cells) c.dcSmoothedValid = false;
    ui::userSettings().setValue(kDcSmoothKey, dcSmoothId);
    ui::userSettings().saveIfNeeded();
}

// ページ切替は頻繁に触るのでヘッダーに残し、設定系の項目だけをOptionsパネルにまとめる。
void ScopePanel::showOptionsMenu()
{
    juce::StringArray win, gain, fps;
    for (int w : windowChoices) win.add(juce::String(w) + " ms");
    gain.add("Auto");
    for (int g : { 1, 2, 4, 8, 16, 32 }) gain.add("x" + juce::String(g));
    for (int f : fpsChoices) fps.add(juce::String(f));

    juce::Component::SafePointer<ScopePanel> s(this);
    std::vector<OptionsPanel::Row> rows;
    rows.push_back({ "View", { "Voice", "MIDI channel" }, static_cast<int>(viewMode),
        [s](int id) { if (s) s->setViewMode(static_cast<ViewMode>(id)); } });
    rows.push_back({ "Window", win, windowId, [s](int id) { if (s) s->setWindowId(id); } });
    rows.push_back({ "Gain", gain, gainId, [s](int id) { if (s) s->setGainId(id); } });
    rows.push_back({ "FPS", fps, fpsId, [s](int id) { if (s) s->setFpsId(id); } });
    rows.push_back({ "DC removal", { "Off", "Mean", "High-pass", "Max-Min" }, static_cast<int>(dcMode) + 1,
        [s](int id) { if (s) s->setDcMode(static_cast<DcMode>(id - 1)); } });
    rows.push_back({ "DC smoothing", { "Off", "Weak", "Strong" }, dcSmoothId,
        [s](int id) { if (s) s->setDcSmoothId(id); } });
    rows.push_back({ "Colour", { "Single colour", "Per voice", "Per MIDI channel" }, static_cast<int>(colourMode),
        [s](int id) { if (s) s->setColourMode(static_cast<ColourMode>(id)); } });

    // 画面の中の子として開く（親なしだとUIスケールが効かない）
    auto& root = uiRoot(*this);
    juce::CallOutBox::launchAsynchronously(std::make_unique<OptionsPanel>(std::move(rows)),
        root.getLocalArea(&optionsButton, optionsButton.getLocalBounds()), &root);
}

void ScopePanel::resized()
{
    auto area = getLocalBounds().reduced(space::sm);
    auto bar = area.removeFromTop(headerHeight - space::sm);
    optionsButton.setBounds(bar.removeFromRight(96));
    prevButton.setBounds(bar.removeFromLeft(space::control));
    pageLabel.setBounds(bar.removeFromLeft(190));
    nextButton.setBounds(bar.removeFromLeft(space::control));
    for (auto* b : { &prevButton, &nextButton }) b->setVisible(!midiMode());
}

// 取得するサンプル数。表示サンプル数(窓)の2倍（トリガー探索に窓と同じ長さの余裕が要る）
int ScopePanel::captureCount() const
{
    double sr = processor.getSampleRate();
    if (sr <= 0.0) sr = 48000.0;
    int shown = juce::roundToInt(windowMs * 0.001 * sr);
    shown = juce::jlimit(64, captureSamples / 2, shown) & ~1; // 偶数
    return shown * 2;
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
        case ColourMode::single: break;
    }
    return colours::text;
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
        for (auto& c : cells) { c.valid = false; c.dcFilter.reset(); }
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
    const int capture = captureCount();
    for (int i = 0; i < nCells; ++i) {
        auto& c = cells[static_cast<size_t>(i)];
        const bool ok = processor.readScopeWindow(midiMode() ? i : page * pageSize + i, c.raw.data(), capture);
        if (!ok) {
            if (c.valid) { c.valid = false; changed = true; }
            continue;
        }
        if (dcMode == DcMode::mean || dcMode == DcMode::maxMin) { // 平均値/Max-MinベースのDC除去（トリガー判定にも効く）
            float offset;
            if (dcMode == DcMode::mean) {
                double sum = 0.0;
                for (int s = 0; s < capture; ++s) sum += c.raw[static_cast<size_t>(s)];
                offset = static_cast<float>(sum / capture);
            } else { // 最大値と最小値の平均
                const auto mm = std::minmax_element(c.raw.begin(), c.raw.begin() + capture);
                offset = (*mm.first + *mm.second) * 0.5f;
            }
            // 補正値をフレーム間で指数移動平均し、縦方向の揺れを抑える（Offなら平滑化なし）
            const float alpha = dcSmoothId == 1 ? 1.0f : dcSmoothId == 2 ? 0.3f : 0.1f;
            c.dcSmoothed = c.dcSmoothedValid ? c.dcSmoothed + alpha * (offset - c.dcSmoothed) : offset;
            c.dcSmoothedValid = true;
            for (int s = 0; s < capture; ++s) c.raw[static_cast<size_t>(s)] -= c.dcSmoothed;
        } else if (dcMode == DcMode::highPass) { // 出力段と同じハイパス（トリガー判定にも効く）
            // 取得窓は連続ストリームではないので、毎回先頭サンプルの定常状態で状態を作り直す
            // （DC入力に対する出力は0）。先頭の過渡応答が表示に出ないようにする。
            double sr = processor.getSampleRate();
            if (sr <= 0.0) sr = 48000.0;
            c.dcFilter.setCoefficients(_3HSPlugAudioProcessor::makeDcHighPassCoefficients(sr));
            c.dcFilter.reset();
            float primed = c.raw[0];
            const int primeSamples = juce::roundToInt(sr * 0.1); // 20Hzの時定数の十分な倍数
            for (int s = 0; s < primeSamples; ++s) { float v = primed; c.dcFilter.processSamples(&v, 1); }
            c.dcFilter.processSamples(c.raw.data(), capture);
        }
        size_t count = 0;
        const size_t start = findScopeTrigger(c.raw.data(), static_cast<size_t>(capture), count);
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
    g.setColour(colours::background);
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
        g.drawText(midiMode() ? midiChannelSubtitle(i + 1) : channelSubtitle(flat), cell.reduced(6, 3).withTrimmedTop(17).removeFromTop(14), juce::Justification::topLeft, true);
    }
}
} // namespace ui
