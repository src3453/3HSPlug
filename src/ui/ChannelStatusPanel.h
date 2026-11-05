#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"

namespace ui {
// Read-only MIDI monitor. Keys show input notes and sustain, rather than audio tails.
class ChannelStatusPanel : public juce::Component
{
public:
    explicit ChannelStatusPanel(_3HSPlugAudioProcessor& p) : processor(p)
    {
        setWantsKeyboardFocus(true);
        setComponentID("ownFocusRing"); // 選択中の行に自分でリングを描く
        setTitle("MIDI channel status");
        setDescription("Up and down select a channel; Enter opens its patch editor");
    }
    void update();
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void focusGained(FocusChangeType) override;
    void focusLost(FocusChangeType) override;
    void setSelectedChannel(int ch) { selectedChannel = ch; repaint(); }
    std::function<void(int)> onChannelSelected, onEditChannel;

private:
    juce::Rectangle<int> rowsArea() const;
    int channelAt(juce::Point<int>) const;
    void choose(int ch);
    void setHoverChannel(int ch);
    _3HSPlugAudioProcessor& processor;
    std::array<_3HSPlugAudioProcessor::ChannelStatus, 16> status {};
    int selectedChannel = 1;
    int hoverChannel = 0;
    bool showFocusRing = false;
};
} // namespace ui
