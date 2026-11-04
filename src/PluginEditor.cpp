#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
_3HSPlugAudioProcessorEditor::_3HSPlugAudioProcessorEditor(_3HSPlugAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      channelActivity(p),
      patchEditor(p),
      voiceStatus(p),
      master(p),
      settings(p)
{
    setLookAndFeel(&lookAndFeel);

    titleBar.onPanic = [this] { audioProcessor.allNotesOff(); };
    titleBar.onSettings = [this] { settings.open(); focusRing.toFront(false); };
    channelActivity.onChannelSelected = [this](int ch) { selectChannel(ch); };
    patchEditor.onChannelChanged = [this](int ch) { channelActivity.setSelectedChannel(ch); };

    addAndMakeVisible(titleBar);
    addAndMakeVisible(channelActivity);
    addAndMakeVisible(patchEditor);
    addAndMakeVisible(voiceStatus);
    addAndMakeVisible(master);
    addChildComponent(settings);
    addAndMakeVisible(focusRing); // いちばん上（設定画面の上にも描く）

    laidOutChips = audioProcessor.getNumChips();
    setResizable(true, true);
    setResizeLimits(1000, 760, 2560, 1600);
    setSize(1200, 880);
    startTimerHz(60);
}

_3HSPlugAudioProcessorEditor::~_3HSPlugAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void _3HSPlugAudioProcessorEditor::selectChannel(int midiChannel)
{
    patchEditor.setChannel(midiChannel);
    channelActivity.setSelectedChannel(midiChannel);
}

void _3HSPlugAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ui::colours::background);
}

void _3HSPlugAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    titleBar.setBounds(area.removeFromTop(52));
    settings.setBounds(getLocalBounds());
    focusRing.setBounds(getLocalBounds());

    area.reduce(ui::space::sm, ui::space::sm);
    master.setBounds(area.removeFromRight(104));
    area.removeFromRight(ui::space::sm);

    channelActivity.setBounds(area.removeFromTop(area.getHeight() < 760 ? 112 : 152));
    area.removeFromTop(ui::space::sm);
    voiceStatus.setBounds(area.removeFromBottom(voiceStatus.getPreferredHeight()));
    area.removeFromBottom(ui::space::sm);
    patchEditor.setBounds(area);
}

void _3HSPlugAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.getNumChips() != laidOutChips) {
        laidOutChips = audioProcessor.getNumChips();
        resized();
    }
    channelActivity.update();
    voiceStatus.repaint();
    master.update();
    patchEditor.refresh();
    settings.update();
    focusRing.refresh();
}
