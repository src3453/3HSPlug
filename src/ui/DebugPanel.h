#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"

namespace ui {
// Debug tab: Chip Register View. Hex dump of the chip register memory (0x400000-0x4003FF).
// Bytes that changed recently are highlighted and fade out.
class ChipRegisterGrid : public juce::Component
{
public:
    static constexpr int rowHeight = 18;
    static constexpr int rows = _3HSPlugAudioProcessor::chipRegisterBytes / 16;
    ChipRegisterGrid() { setSize(100, rows * rowHeight + 4); }
    // Returns true if anything changed (so the owner can repaint).
    bool setData(const std::array<uint8_t, _3HSPlugAudioProcessor::chipRegisterBytes>& data, bool resetHistory);
    void paint(juce::Graphics&) override;

private:
    static juce::String rowLabel(int row);
    std::array<uint8_t, _3HSPlugAudioProcessor::chipRegisterBytes> values {};
    std::array<float, _3HSPlugAudioProcessor::chipRegisterBytes> flash {};
};

class DebugPanel : public juce::Component
{
public:
    explicit DebugPanel(_3HSPlugAudioProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
    void update(); // called from the UI timer while visible

private:
    _3HSPlugAudioProcessor& processor;
    juce::Label chipLabel;
    juce::ComboBox chipBox;
    juce::Viewport viewport;
    ChipRegisterGrid grid;
    int shownChips = 0;
    bool needsReset = true;
};
} // namespace ui
