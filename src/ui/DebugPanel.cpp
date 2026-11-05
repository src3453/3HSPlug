#include "DebugPanel.h"

namespace ui {
namespace {
constexpr int addrW = 78, nameW = 150, cellW = 30;
}

juce::String ChipRegisterGrid::rowLabel(int row)
{
    const int addr = row * 16;
    if (addr < 0x200) {
        const int ch = addr / 0x40 + 1, off = addr % 0x40;
        static const char* parts[] = { "Freq/Vol/Wave/Alg/Pan/Gate/FB", "", "Env OP1-4 (A D S R)", "Env OP5-8 (A D S R)" };
        const juce::String p = parts[off / 16];
        return "CH" + juce::String(ch) + " FM  " + (p.isEmpty() ? "" : p);
    }
    if (addr < 0x2C0) {
        const int ch = (addr - 0x200) / 0x30 + 9, off = (addr - 0x200) % 0x30;
        return "CH" + juce::String(ch) + " PCM " + (off == 0 ? "Freq/Vol/Mode/Filt" : off == 16 ? "Addr/Wave" : "Wave");
    }
    if (addr == 0x2C0) return "Comp / EQ";
    if (addr == 0x2D0) return "Channel mute";
    if (addr == 0x3C0) return "FB destination";
    return {};
}

bool ChipRegisterGrid::setData(const std::array<uint8_t, _3HSPlugAudioProcessor::chipRegisterBytes>& data, bool resetHistory)
{
    bool dirty = false;
    for (size_t i = 0; i < values.size(); ++i) {
        if (resetHistory) {
            flash[i] = 0.0f;
            dirty = true;
        } else if (values[i] != data[i]) {
            flash[i] = 1.0f;
            dirty = true;
        } else if (flash[i] > 0.0f) {
            flash[i] = juce::jmax(0.0f, flash[i] - 0.03f);
            dirty = true;
        }
        values[i] = data[i];
    }
    return dirty;
}

void ChipRegisterGrid::paint(juce::Graphics& g)
{
    const auto clip = g.getClipBounds();
    const int first = juce::jmax(0, (clip.getY() - 2) / rowHeight);
    const int last = juce::jmin(rows - 1, (clip.getBottom() - 2) / rowHeight);
    const auto mono = monoFont(type::small);
    for (int row = first; row <= last; ++row) {
        const int y = 2 + row * rowHeight;
        if (row % 2 == 1) {
            g.setColour(colours::panelRaised.withAlpha(0.35f));
            g.fillRect(0, y, getWidth(), rowHeight);
        }
        g.setFont(mono);
        g.setColour(colours::textDim);
        g.drawText("0x" + juce::String::toHexString(0x400000 + row * 16).toUpperCase().paddedLeft('0', 6),
                   space::sm, y, addrW, rowHeight, juce::Justification::centredLeft);
        g.setColour(colours::textDim);
        g.drawText(rowLabel(row), space::sm + addrW, y, nameW + 60, rowHeight, juce::Justification::centredLeft);
        for (int col = 0; col < 16; ++col) {
            const size_t i = static_cast<size_t>(row * 16 + col);
            const int x = space::sm + addrW + nameW + 60 + col * cellW;
            if (flash[i] > 0.0f) {
                g.setColour(colours::accent.withAlpha(0.65f * flash[i]));
                g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), cellW - 2.0f, rowHeight - 2.0f, 3.0f);
            }
            g.setColour(values[i] == 0 ? colours::textDim.withAlpha(0.6f) : colours::text);
            g.drawText(juce::String::toHexString(values[i]).toUpperCase().paddedLeft('0', 2),
                       x, y, cellW - 2, rowHeight, juce::Justification::centred);
        }
    }
}

DebugPanel::DebugPanel(_3HSPlugAudioProcessor& p) : processor(p)
{
    setTitle("Debug");
    chipLabel.setText("Chip register view  -  chip", juce::dontSendNotification);
    setLabelFont(chipLabel, type::body);
    addAndMakeVisible(chipLabel);
    chipBox.onChange = [this] { needsReset = true; update(); };
    addAndMakeVisible(chipBox);
    viewport.setViewedComponent(&grid, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);
}

void DebugPanel::paint(juce::Graphics& g)
{
    g.fillAll(colours::panel);
}

void DebugPanel::resized()
{
    auto area = getLocalBounds().reduced(space::sm);
    auto row = area.removeFromTop(space::control);
    chipLabel.setBounds(row.removeFromLeft(200));
    chipBox.setBounds(row.removeFromLeft(120));
    area.removeFromTop(space::sm);
    viewport.setBounds(area);
    grid.setSize(juce::jmax(area.getWidth() - viewport.getScrollBarThickness(),
                            space::sm + addrW + nameW + 60 + 16 * cellW), grid.getHeight());
}

void DebugPanel::update()
{
    const int chips = processor.getNumChips();
    if (chips != shownChips) {
        const int selected = chipBox.getSelectedId();
        chipBox.clear(juce::dontSendNotification);
        for (int i = 1; i <= chips; ++i)
            chipBox.addItem("Chip " + juce::String(i), i);
        chipBox.setSelectedId(juce::jlimit(1, juce::jmax(1, chips), selected > 0 ? selected : 1), juce::dontSendNotification);
        shownChips = chips;
        needsReset = true;
    }
    std::array<uint8_t, _3HSPlugAudioProcessor::chipRegisterBytes> data {};
    if (!processor.getChipRegisters(chipBox.getSelectedId() - 1, data))
        return;
    if (grid.setData(data, needsReset))
        grid.repaint();
    needsReset = false;
}
} // namespace ui
