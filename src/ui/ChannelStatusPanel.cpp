#include "ChannelStatusPanel.h"

namespace ui {
namespace {
constexpr int channelWidth = 32, patchWidth = 174;
constexpr int ccNumbers[] { -1, 7, 11, 10, -2, 1, 64, 74, 71, 91, 93, 94 };
constexpr const char* labels[] { "VEL", "VOL", "EXP", "PAN", "BEND", "MOD", "HOLD", "CUT", "RES", "REV", "CHO", "DLY" };
int cellWidth(int i) { return i == 4 ? 48 : 34; }
bool blackKey(int n) { const int p = n % 12; return p == 1 || p == 3 || p == 6 || p == 8 || p == 10; }

void drawKeyboard(juce::Graphics& g, juce::Rectangle<int> bounds,
                  const std::array<uint8_t, 128>& notes, juce::Colour colour)
{
    // MIDI 0..127 has 75 white keys. Draw whites before overlapping black keys.
    const float keyWidth = bounds.getWidth() / 75.0f;
    const float top = static_cast<float>(bounds.getY());
    const float height = static_cast<float>(bounds.getHeight());
    for (int pass = 0; pass < 2; ++pass) {
        int white = 0;
        for (int n = 0; n < 128; ++n) {
            const bool black = blackKey(n);
            const float x = bounds.getX() + (black ? white - 0.32f : static_cast<float>(white)) * keyWidth;
            if (black == (pass == 1)) {
                juce::Rectangle<float> key(x, top, keyWidth * (black ? 0.64f : 1.0f), height * (black ? 0.62f : 1.0f));
                g.setColour(notes[static_cast<size_t>(n)] != 0 ? colour : (black ? colours::keyBlack : colours::keyWhite));
                g.fillRect(key);
                g.setColour(colours::fieldEdge);
                g.drawRect(key, 0.5f);
            }
            if (!black) ++white;
        }
    }
}
} // namespace

juce::Rectangle<int> ChannelStatusPanel::rowsArea() const
{
    return getLocalBounds().reduced(space::sm).withTrimmedTop(2 * space::sectionHeader).withTrimmedBottom(space::sectionHeader);
}

void ChannelStatusPanel::update()
{
    if (processor.readChannelStatus(status)) repaint();
}

void ChannelStatusPanel::paint(juce::Graphics& g)
{
    drawSection(g, getLocalBounds(), "MIDI channel status");
    auto header = getLocalBounds().reduced(space::sm).withTrimmedTop(space::sectionHeader).removeFromTop(space::sectionHeader);
    g.setFont(font(type::caption, true));
    g.setColour(colours::textDim);
    g.drawText("CH", header.removeFromLeft(channelWidth), juce::Justification::centred);
    g.drawText("Program / Bank MSB : LSB", header.removeFromLeft(patchWidth), juce::Justification::centredLeft);
    for (int i = 0; i < 12; ++i)
        g.drawText(labels[i], header.removeFromLeft(cellWidth(i)), juce::Justification::centred);
    g.drawText("Notes  C-1 ... G9", header, juce::Justification::centred);

    const auto rows = rowsArea();
    for (int i = 0; i < 16; ++i) {
        const auto& s = status[static_cast<size_t>(i)];
        const int y = rows.getY() + rows.getHeight() * i / 16;
        auto row = juce::Rectangle<int>(rows.getX(), y, rows.getWidth(), rows.getY() + rows.getHeight() * (i + 1) / 16 - y);
        const auto colour = colours::forChannel(i);
        if (selectedChannel == i + 1) {
            drawSelection(g, row.toFloat());
            if (showFocusRing)
                drawFocusRing(g, row.toFloat());
        } else {
            if (i % 2 == 0) {
                g.setColour(colours::panelRaised);
                g.fillRect(row);
            }
            if (hoverChannel == i + 1) {
                g.setColour(colours::fieldEdge);
                g.drawRoundedRectangle(row.toFloat().reduced(0.5f), shape::r(radius::control), stroke::hairline);
            }
        }
        auto content = row.reduced(0, space::xs);
        g.setFont(monoFont(type::body));
        g.setColour(colour);
        g.drawText(juce::String(i + 1).paddedLeft('0', 2), content.removeFromLeft(channelWidth), juce::Justification::centred);
        auto patch = content.removeFromLeft(patchWidth).reduced(4, 0);
        g.setColour(colours::text);
        g.setFont(font(type::small, true));
        const auto name = s.drum ? juce::String("Drum kit") : juce::String(juce::MidiMessage::getGMInstrumentName(s.program));
        g.drawText(juce::String(s.program + 1).paddedLeft('0', 3) + " " + name,
                   patch.removeFromTop(patch.getHeight() / 2), juce::Justification::centredLeft);
        g.setColour(colours::textDim);
        g.setFont(monoFont(type::caption));
        g.drawText(juce::String(s.bank).paddedLeft('0', 3) + " : " + juce::String(s.bankLsb).paddedLeft('0', 3)
                   + (s.drum ? "  PCM" : "  GM name"), patch, juce::Justification::centredLeft);
        for (int c = 0; c < 12; ++c) {
            auto cell = content.removeFromLeft(cellWidth(c)).reduced(3, 0);
            const int value = c == 0 ? s.velocity : c == 4 ? s.pitchBend : s.controllers[static_cast<size_t>(ccNumbers[c])];
            auto bar = cell.removeFromBottom(3).toFloat();
            g.setColour(colours::outline);
            g.fillRect(bar);
            g.setColour(colour.withAlpha(0.75f));
            if (c == 4) {
                const float middle = bar.getCentreX();
                const float end = middle + (value / 8192.0f) * bar.getWidth() * 0.5f;
                g.fillRect(juce::Rectangle<float>(juce::jmin(middle, end), bar.getY(), std::abs(end - middle), bar.getHeight()));
            } else g.fillRect(bar.withWidth(bar.getWidth() * value / 127.0f));
            g.setColour(colours::text);
            g.setFont(monoFont(type::caption));
            g.drawText(juce::String(value), cell, juce::Justification::centred);
        }
        // 単色のテーマでは濃淡が鍵盤に埋もれるので、発音中のノートはアクセントで示す
        drawKeyboard(g, content.reduced(5, 2), s.notes, shape::monochromeChannels ? colours::accent : colour);
    }
    g.setColour(colours::textDim);
    g.setFont(font(type::caption));
    g.drawText("Keys show MIDI notes and sustain. CC values are as received. Double-click or press Enter to edit a channel.",
               getLocalBounds().reduced(space::md, space::xs).removeFromBottom(20), juce::Justification::centredLeft);
}

int ChannelStatusPanel::channelAt(juce::Point<int> p) const
{
    const auto rows = rowsArea();
    return rows.contains(p) ? juce::jlimit(1, 16, (p.y - rows.getY()) * 16 / juce::jmax(1, rows.getHeight()) + 1) : 0;
}
void ChannelStatusPanel::choose(int ch)
{
    selectedChannel = ch;
    if (onChannelSelected) onChannelSelected(ch);
    repaint();
}
void ChannelStatusPanel::focusGained(FocusChangeType cause)
{
    showFocusRing = cause != focusChangedByMouseClick;
    repaint();
}
void ChannelStatusPanel::focusLost(FocusChangeType)
{
    showFocusRing = false;
    repaint();
}
void ChannelStatusPanel::mouseDown(const juce::MouseEvent& e)
{
    showFocusRing = false;
    if (const int ch = channelAt(e.getPosition()); ch != 0) choose(ch);
}
void ChannelStatusPanel::mouseMove(const juce::MouseEvent& e)
{
    setHoverChannel(channelAt(e.getPosition()));
}
void ChannelStatusPanel::mouseExit(const juce::MouseEvent&)
{
    setHoverChannel(0);
}
void ChannelStatusPanel::setHoverChannel(int ch)
{
    if (ch != hoverChannel) {
        hoverChannel = ch;
        repaint();
    }
}
void ChannelStatusPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const int ch = channelAt(e.getPosition()); ch != 0) {
        choose(ch);
        if (onEditChannel) onEditChannel(ch);
    }
}
bool ChannelStatusPanel::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::upKey) choose(juce::jmax(1, selectedChannel - 1));
    else if (key == juce::KeyPress::downKey) choose(juce::jmin(16, selectedChannel + 1));
    else if (key == juce::KeyPress::homeKey) choose(1);
    else if (key == juce::KeyPress::endKey) choose(16);
    else if (key == juce::KeyPress::returnKey) { if (onEditChannel) onEditChannel(selectedChannel); }
    else return false;
    showFocusRing = true;
    repaint();
    return true;
}
} // namespace ui
