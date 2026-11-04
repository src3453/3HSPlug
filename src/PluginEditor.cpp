#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
_3HSPlugAudioProcessorEditor::_3HSPlugAudioProcessorEditor(_3HSPlugAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      channelStatus(p),
      channelActivity(p),
      patchEditor(p),
      voiceStatus(p),
      master(p),
      settings(p)
{
    ui::applySavedTheme();
    lookAndFeel.applyTheme();
    setLookAndFeel(&lookAndFeel);
    ui::appearanceChanged().addChangeListener(this);

    titleBar.onPanic = [this] { audioProcessor.allNotesOff(); };
    titleBar.onSettings = [this] { settings.open(); focusRing.toFront(false); };
    channelActivity.onChannelSelected = [this](int ch) { selectChannel(ch); };
    patchEditor.onChannelChanged = [this](int ch) {
        channelActivity.setSelectedChannel(ch);
        channelStatus.setSelectedChannel(ch);
    };
    channelStatus.onChannelSelected = [this](int ch) { selectChannel(ch); };
    channelStatus.onEditChannel = [this](int ch) {
        selectChannel(ch);
        tabs.setCurrentTabIndex(0);
    };

    addAndMakeVisible(titleBar);
    synthPage.addAndMakeVisible(channelActivity);
    synthPage.addAndMakeVisible(patchEditor);
    synthPage.addAndMakeVisible(voiceStatus);
    tabs.setTabBarDepth(32);
    tabs.setOutline(0);
    tabs.addTab("Synth", ui::colours::panel, &synthPage, false);
    tabs.addTab("Channels", ui::colours::panel, &channelStatus, false);
    addAndMakeVisible(tabs);
    addAndMakeVisible(master);
    addChildComponent(settings);
    addAndMakeVisible(focusRing); // いちばん上（設定画面の上にも描く）

    laidOutChips = audioProcessor.getNumChips();
    setResizable(true, true);
    setResizeLimits(1000, 800, 2560, 1600);
    setSize(1200, 920);
    startTimerHz(60);
}

_3HSPlugAudioProcessorEditor::~_3HSPlugAudioProcessorEditor()
{
    stopTimer();
    ui::appearanceChanged().removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void _3HSPlugAudioProcessorEditor::selectChannel(int midiChannel)
{
    patchEditor.setChannel(midiChannel);
    channelActivity.setSelectedChannel(midiChannel);
    channelStatus.setSelectedChannel(midiChannel);
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
    tabs.setBounds(area);
    area = synthPage.getLocalBounds().withTrimmedTop(ui::space::sm);

    channelActivity.setBounds(area.removeFromTop(area.getHeight() < 760 ? 112 : 152));
    area.removeFromTop(ui::space::sm);
    voiceStatus.setBounds(area.removeFromBottom(voiceStatus.getPreferredHeight()));
    area.removeFromBottom(ui::space::sm);
    patchEditor.setBounds(area);
}

void _3HSPlugAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    ui::refreshFonts(*this);
    // JUCE detaches the inactive tab page from the component tree.
    if (tabs.getCurrentTabIndex() != 0)
        ui::refreshFonts(synthPage);
    ui::refreshTheme(*this);
    for (int i = 0; i < tabs.getNumTabs(); ++i)
        tabs.setTabBackgroundColour(i, ui::colours::panel);
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
    if (channelStatus.isShowing()) channelStatus.update();
    settings.update();
    focusRing.refresh();
}
