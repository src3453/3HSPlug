// ドラムキットの編集（ノートごとのサンプル割り当て、試聴、PCMメモリ使用量）
#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Theme.h"

namespace ui {

class DrumKitPanel : public juce::Component,
                     public juce::FileDragAndDropTarget,
                     public juce::SettableTooltipClient
{
public:
    explicit DrumKitPanel(_3HSPlugAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void update(); // タイマーから呼ぶ

    void mouseDown(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void focusGained(FocusChangeType) override;
    void focusLost(FocusChangeType) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    juce::String getTooltipForNote(int note) const;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    static constexpr int kColumns = 16;
    static constexpr int kRows = 8;

    juce::Rectangle<int> memoryArea() const;
    juce::Rectangle<int> gridArea() const;
    juce::Rectangle<int> detailArea() const;
    juce::Rectangle<float> cellBounds(int note) const;
    int noteAt(juce::Point<int>) const;

    void reloadSlots();
    void select(int note, bool play);
    void chooseSample(int note);
    void assign(int note, const juce::File& file);
    void showMessage(const juce::String& text, bool error);
    void showMenu(int note);
    void layoutDetailButtons();
    bool hasCustomAssignments() const;
    void confirm(const juce::String& title, const juce::String& text, const juce::String& action, std::function<void()> onConfirm);
    void requestClear(int note);
    void loadKit();

    _3HSPlugAudioProcessor& processor;
    std::array<_3HSPlugAudioProcessor::DrumSlotView, 128> slots;
    uint32_t loadedRevision = ~0u;
    uint32_t memoryUsed = 0;
    int selectedNote = 36;
    int hoverNote = -1;
    int dropNote = -1;
    bool showFocusRing = false;
    std::array<float, 128> flash {};

    juce::String message;
    bool messageIsError = false;
    int messageFrames = 0;

    juce::TextButton loadKitButton { "Load Kit..." };
    juce::TextButton saveKitButton { "Save Kit..." };
    juce::TextButton resetButton { "Reset to Default" };
    juce::TextButton assignButton { "Assign..." };
    juce::TextButton clearButton { "Clear" };
    juce::TextButton playButton { "Play" };
    std::unique_ptr<juce::FileChooser> chooser;
};

} // namespace ui
