#include "Panels.h"

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__ " " __TIME__
#endif

namespace ui {

namespace {
constexpr int64_t kGsDisplayTicks = 120000; // GS表示を出しておく時間（tick）

juce::String noteName(int note)
{
    if (note < 0 || note > 127)
        return "--";
    return juce::MidiMessage::getMidiNoteName(note, true, true, 4);
}
} // namespace

//==============================================================================
TitleBar::TitleBar()
{
    panicButton.setTooltip("Stop all sounding notes");
    panicButton.onClick = [this] { if (onPanic) onPanic(); };
    settingsButton.onClick = [this] { if (onSettings) onSettings(); };
    addAndMakeVisible(panicButton);
    addAndMakeVisible(settingsButton);
}

void TitleBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setGradientFill(juce::ColourGradient(colours::panelRaised, 0.0f, 0.0f,
                                           colours::panel, 0.0f, static_cast<float>(bounds.getHeight()), false));
    g.fillRect(bounds);
    g.setColour(colours::accent);
    g.fillRect(bounds.removeFromBottom(2));

    auto text = getLocalBounds().reduced(14, 0);
    g.setColour(colours::text);
    g.setFont(font(26.0f, true));
    const juce::String logo = "3HSPlug";
    const int logoW = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), logo) + 4;
    g.drawText(logo, text.removeFromLeft(logoW), juce::Justification::centredLeft);
    text.removeFromLeft(12);

    auto sub = text.removeFromLeft(420);
    g.setColour(colours::accent);
    g.setFont(font(13.0f, true));
    g.drawText("3HS88PWN4 Harmonic MIDI Synthesizer", sub.removeFromTop(sub.getHeight() / 2).withTrimmedTop(4),
               juce::Justification::bottomLeft);
    g.setColour(colours::textDim);
    g.setFont(font(11.0f));
    g.drawText("by src3453  |  build " BUILD_DATE, sub.withTrimmedBottom(4), juce::Justification::topLeft);
}

void TitleBar::resized()
{
    auto area = getLocalBounds().withTrimmedBottom(2).reduced(10, 9);
    settingsButton.setBounds(area.removeFromRight(90));
    area.removeFromRight(8);
    panicButton.setBounds(area.removeFromRight(70));
}

//==============================================================================
ChannelActivityPanel::ChannelActivityPanel(_3HSPlugAudioProcessor& p) : processor(p)
{
    setRepaintsOnMouseActivity(false);
}

void ChannelActivityPanel::update()
{
    const auto& slots = processor.getVoiceSlots();
    const int totalVoices = std::min(static_cast<int>(slots.size()),
                                     processor.getNumChips() * _3HSPlugAudioProcessor::numVoices);
    const uint64_t now = processor.getCurrentTick();

    for (auto& level : levels)
        level = juce::jmax(0.0f, level - 0.02f);

    for (int i = 0; i < totalVoices; ++i) {
        const auto& v = slots[static_cast<size_t>(i)];
        if (v.inUse && v.midiChannel >= 1 && v.midiChannel <= 16)
            levels[v.midiChannel - 1] = juce::jmax(levels[v.midiChannel - 1], v.volume / 255.0f);
    }
    for (const auto& d : processor.drumPcmChannelStates) {
        if (d.inUse && d.midiChannel >= 1 && d.midiChannel <= 16 && now - d.lastUsedTick < 2100)
            levels[d.midiChannel - 1] = juce::jmax(levels[d.midiChannel - 1], d.volume / 255.0f);
    }

    // GSドットマトリクス（16行 × 16列。データは5ビットずつ並ぶ）
    if (processor.gsDotMatrixUpdated.exchange(false)) {
        const auto& data = processor.gsDotMatrixData;
        for (int row = 0; row < 16; ++row) {
            int col = 0;
            for (int block = 0; block < 3; ++block)
                for (int bit = 4; bit >= 0; --bit)
                    gsDots[row][col++] = ((data[static_cast<size_t>(row + block * 16)] >> bit) & 1) != 0;
            gsDots[row][col] = ((data[static_cast<size_t>(row + 48)] >> 4) & 1) != 0;
        }
    }
    if (processor.gsTextUpdated.exchange(false))
        gsText = juce::String::fromUTF8(processor.TextDisplayData.c_str()).trim();

    // 一度も受け取っていなければ lastGS*UpdateTick は int64 の最小値
    auto recent = [now](int64_t last) {
        return last >= 0 && now >= static_cast<uint64_t>(last) && now - static_cast<uint64_t>(last) < kGsDisplayTicks;
    };
    showGsDots = recent(processor.lastGSDotUpdateTick);
    showGsText = gsText.isNotEmpty() && recent(processor.lastGSTextUpdateTick);
    repaint();
}

juce::Rectangle<int> ChannelActivityPanel::columnsArea() const
{
    return getLocalBounds().withTrimmedTop(22).reduced(8, 4).withTrimmedBottom(2);
}

int ChannelActivityPanel::channelAt(juce::Point<int> pos) const
{
    const auto area = columnsArea();
    if (!area.contains(pos))
        return 0;
    return juce::jlimit(1, 16, 1 + (pos.x - area.getX()) * 16 / juce::jmax(1, area.getWidth()));
}

void ChannelActivityPanel::mouseDown(const juce::MouseEvent& e)
{
    if (const int ch = channelAt(e.getPosition()); ch > 0) {
        selectedChannel = ch;
        if (onChannelSelected)
            onChannelSelected(ch);
        repaint();
    }
}

void ChannelActivityPanel::paint(juce::Graphics& g)
{
    drawSection(g, getLocalBounds(), "Channel Activity");

    // 見出しの右にGSテキストを出す
    if (showGsText) {
        auto header = getLocalBounds().removeFromTop(22).reduced(10, 3);
        auto lcd = header.removeFromRight(juce::jmin(260, header.getWidth() / 2));
        g.setColour(colours::lcdBack);
        g.fillRoundedRectangle(lcd.toFloat(), 3.0f);
        g.setColour(colours::lcdOn);
        g.setFont(monoFont(12.0f));
        g.drawText(gsText, lcd.reduced(6, 0), juce::Justification::centredLeft, true);
    }

    const auto area = columnsArea();
    const float colW = area.getWidth() / 16.0f;
    const int labelH = 14;
    const int progH = 13;
    auto barsTop = area.getY() + labelH + 2;
    auto barsBottom = area.getBottom() - progH - 2;
    const int barsH = juce::jmax(16, barsBottom - barsTop);

    if (showGsDots) {
        auto lcd = juce::Rectangle<int>(area.getX(), barsTop, area.getWidth(), barsH);
        g.setColour(colours::lcdBack);
        g.fillRoundedRectangle(lcd.toFloat().expanded(2.0f), 3.0f);
    }

    for (int ch = 0; ch < 16; ++ch) {
        const int x0 = area.getX() + juce::roundToInt(ch * colW);
        const int x1 = area.getX() + juce::roundToInt((ch + 1) * colW);
        auto col = juce::Rectangle<int>(x0, area.getY(), x1 - x0, area.getHeight()).reduced(2, 0);
        const bool selected = (ch + 1) == selectedChannel;
        const bool drum = processor.isDrumChannel(ch + 1);
        const auto chColour = colours::forChannel(ch);

        if (selected) {
            g.setColour(colours::accent.withAlpha(0.12f));
            g.fillRoundedRectangle(col.toFloat().expanded(1.0f, 0.0f), 4.0f);
            g.setColour(colours::accent);
            g.drawRoundedRectangle(col.toFloat().expanded(1.0f, 0.0f), 4.0f, 1.2f);
        }

        g.setColour(selected ? colours::accent : colours::text);
        g.setFont(font(11.5f, true));
        g.drawText(juce::String(ch + 1) + (drum ? " D" : ""), col.withHeight(labelH), juce::Justification::centred);

        auto bars = juce::Rectangle<int>(col.getX() + 3, barsTop, col.getWidth() - 6, barsH);
        if (showGsDots) {
            const float dotH = barsH / 16.0f;
            for (int row = 0; row < 16; ++row) {
                auto dot = juce::Rectangle<float>(static_cast<float>(bars.getX()), barsTop + row * dotH,
                                                  static_cast<float>(bars.getWidth()), dotH).reduced(0.5f, 0.8f);
                g.setColour(gsDots[row][ch] ? colours::lcdOn : colours::lcdOff);
                g.fillRect(dot);
            }
        } else {
            const int segments = 16;
            const float segH = barsH / static_cast<float>(segments);
            const int lit = juce::roundToInt(levels[ch] * segments);
            for (int s = 0; s < segments; ++s) {
                auto seg = juce::Rectangle<float>(static_cast<float>(bars.getX()), bars.getBottom() - (s + 1) * segH,
                                                  static_cast<float>(bars.getWidth()), segH).reduced(0.0f, 0.9f);
                g.setColour(s < lit ? chColour.withMultipliedBrightness(0.65f + 0.35f * s / segments)
                                    : colours::panelRaised);
                g.fillRoundedRectangle(seg, 1.0f);
            }
        }

        const auto loc = processor.getChannelPatchLocation(ch + 1);
        g.setFont(monoFont(9.5f));
        g.setColour(loc.availability == 2 ? colours::textDim
                    : loc.availability == 1 ? colours::meterYellow : colours::meterRed);
        if (drum)
            g.setColour(colours::textDim);
        juce::String prog = drum ? "PCM" : juce::String(loc.program + 1).paddedLeft('0', 3);
        if (loc.modified && !drum)
            prog += "*";
        g.drawText(prog, col.withTop(area.getBottom() - progH), juce::Justification::centred);
    }
}

//==============================================================================
VoiceStatusPanel::VoiceStatusPanel(_3HSPlugAudioProcessor& p) : processor(p) {}

int VoiceStatusPanel::getPreferredHeight() const
{
    const int chips = processor.getNumChips();
    const int rowH = chips <= 4 ? 18 : chips <= 8 ? 13 : 6;
    return 22 + 8 + chips * rowH;
}

void VoiceStatusPanel::paint(juce::Graphics& g)
{
    auto area = drawSection(g, getLocalBounds(), "Voices");
    const int chips = processor.getNumChips();
    const auto& slots = processor.getVoiceSlots();
    const auto& drums = processor.drumPcmChannelStates;
    const uint64_t now = processor.getCurrentTick();
    if (chips <= 0)
        return;

    const float rowH = area.getHeight() / static_cast<float>(chips);
    const bool showText = rowH >= 13.0f;
    const int labelW = 52;
    const int gap = 10;
    const float cellW = (area.getWidth() - labelW - gap) / 12.0f;

    for (int chip = 0; chip < chips; ++chip) {
        const float y = area.getY() + chip * rowH;
        g.setColour(colours::textDim);
        if (rowH >= 9.0f) {
            g.setFont(font(juce::jmin(11.0f, rowH - 2.0f), true));
            g.drawText("CHIP " + juce::String(chip + 1), juce::Rectangle<float>(static_cast<float>(area.getX()), y, labelW, rowH),
                       juce::Justification::centredLeft);
        } else if (chip % 4 == 0) {
            g.setFont(font(9.0f, true));
            g.drawText("CHIP " + juce::String(chip + 1), juce::Rectangle<float>(static_cast<float>(area.getX()), y, labelW, 10.0f),
                       juce::Justification::topLeft);
        }

        for (int cell = 0; cell < 12; ++cell) {
            const bool pcm = cell >= 8;
            float x = area.getX() + labelW + cell * cellW + (pcm ? gap : 0);
            auto r = juce::Rectangle<float>(x, y, cellW, rowH).reduced(1.5f, juce::jmin(1.5f, rowH * 0.1f));

            bool active = false;
            int midiCh = 0, note = -1;
            float level = 0.0f;
            if (!pcm) {
                const size_t idx = static_cast<size_t>(chip * _3HSPlugAudioProcessor::numVoices + cell);
                if (idx < slots.size()) {
                    const auto& v = slots[idx];
                    active = v.inUse;
                    midiCh = v.midiChannel;
                    note = v.noteNumber;
                    level = v.volume / 255.0f;
                }
            } else {
                const size_t idx = static_cast<size_t>(chip * 4 + (cell - 8));
                if (idx < drums.size()) {
                    const auto& d = drums[idx];
                    const uint64_t age = now - d.lastUsedTick;
                    active = d.inUse && age < 24000;
                    midiCh = d.midiChannel;
                    note = d.noteNumber;
                    level = active ? (d.velocity / 127.0f) * (1.0f - static_cast<float>(age) / 24000.0f) : 0.0f;
                }
            }

            g.setColour(pcm ? colours::panelRaised.darker(0.15f) : colours::panelRaised);
            g.fillRoundedRectangle(r, 2.0f);
            if (active && midiCh >= 1 && midiCh <= 16) {
                const auto c = colours::forChannel(midiCh - 1);
                g.setColour(c.withAlpha(0.25f + 0.5f * level));
                g.fillRoundedRectangle(r.withWidth(juce::jmax(2.0f, r.getWidth() * level)), 2.0f);
                if (showText) {
                    g.setColour(colours::text);
                    g.setFont(monoFont(juce::jmin(11.0f, rowH - 4.0f)));
                    g.drawText(juce::String(midiCh) + " " + noteName(note), r.reduced(3.0f, 0.0f),
                               juce::Justification::centredLeft, false);
                }
            }
        }
    }

    // 列の見出し（最上段の上に小さく）
    g.setColour(colours::textDim);
    g.setFont(font(10.0f));
    auto header = getLocalBounds().removeFromTop(22).reduced(10, 0);
    header.removeFromLeft(labelW + 70);
    g.drawText("FM 1-8   |   PCM 1-4", header, juce::Justification::centredRight);
}

//==============================================================================
MasterSection::MasterSection(_3HSPlugAudioProcessor& p) : processor(p)
{
    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setDoubleClickReturnValue(true, 0.0);
    fader.setTooltip("Master volume (double-click: 0 dB)");
    addAndMakeVisible(fader);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getParameters(), _3HSPlugAudioProcessor::masterVolumeParamId, fader);

    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setFont(monoFont(12.0f));
    valueLabel.setEditable(false, true);
    valueLabel.onTextChange = [this] {
        fader.setValue(valueLabel.getText().retainCharacters("-0123456789.").getFloatValue(), juce::sendNotificationSync);
    };
    addAndMakeVisible(valueLabel);
    fader.onValueChange = [this] {
        const double v = fader.getValue();
        valueLabel.setText(v <= _3HSPlugAudioProcessor::masterVolumeMinDb ? "-inf dB" : juce::String(v, 1) + " dB",
                           juce::dontSendNotification);
    };
    fader.onValueChange();
}

void MasterSection::resized()
{
    auto area = getLocalBounds().withTrimmedTop(22).reduced(6, 6);
    valueLabel.setBounds(area.removeFromBottom(20));
    area.removeFromBottom(4);
    area.removeFromTop(16); // クリップ表示
    fader.setBounds(area.removeFromLeft(area.getWidth() / 2).reduced(0, 0));
}

juce::Rectangle<int> MasterSection::meterArea() const
{
    auto area = getLocalBounds().withTrimmedTop(22).reduced(6, 6);
    area.removeFromBottom(24);
    area.removeFromTop(16);
    area.removeFromLeft(area.getWidth() / 2);
    return area.reduced(2, 6);
}

void MasterSection::update()
{
    const auto reading = processor.consumeMeterReading();
    for (int c = 0; c < 2; ++c) {
        peak[c] = juce::jmax(reading.peak[c], peak[c] * 0.86f);
        rms[c] = juce::jmax(reading.rms[c], rms[c] * 0.92f);
        if (reading.peak[c] >= hold[c]) {
            hold[c] = reading.peak[c];
            holdFrames[c] = 90;
        } else if (--holdFrames[c] <= 0) {
            hold[c] *= 0.9f;
        }
        if (reading.peak[c] >= 1.0f)
            clipped = true;
    }
    repaint(meterArea().expanded(4).withTop(0));
}

void MasterSection::mouseDown(const juce::MouseEvent&)
{
    clipped = false; // クリップ表示をクリア
    repaint();
}

void MasterSection::paint(juce::Graphics& g)
{
    drawSection(g, getLocalBounds(), "Master");

    auto meter = meterArea();
    auto clip = getLocalBounds().withTrimmedTop(22).reduced(6, 6).removeFromTop(14);
    g.setColour(clipped ? colours::meterRed : colours::panelRaised);
    g.fillRoundedRectangle(clip.toFloat(), 3.0f);
    g.setColour(clipped ? colours::background : colours::textDim);
    g.setFont(font(10.0f, true));
    g.drawText("CLIP", clip, juce::Justification::centred);

    constexpr float minDb = -60.0f, maxDb = 6.0f;
    auto yFor = [&](float gain) {
        const float db = juce::jlimit(minDb, maxDb, juce::Decibels::gainToDecibels(gain, minDb));
        return juce::jmap(db, minDb, maxDb, static_cast<float>(meter.getBottom()), static_cast<float>(meter.getY()));
    };

    const int barW = (meter.getWidth() - 16) / 2;
    for (int c = 0; c < 2; ++c) {
        auto bar = juce::Rectangle<int>(meter.getX() + c * (barW + 2), meter.getY(), barW, meter.getHeight()).toFloat();
        g.setColour(colours::background);
        g.fillRect(bar);
        juce::ColourGradient grad(colours::meterRed, 0.0f, yFor(1.0f), colours::meterGreen, 0.0f, yFor(juce::Decibels::decibelsToGain(-18.0f)), false);
        grad.addColour(0.35, colours::meterYellow);
        g.setGradientFill(grad);
        g.fillRect(bar.withTop(yFor(peak[c])));
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.fillRect(bar.withTop(yFor(rms[c])).reduced(barW * 0.3f, 0.0f));
        if (hold[c] > 0.001f) {
            g.setColour(hold[c] >= 1.0f ? colours::meterRed : colours::text);
            g.fillRect(bar.getX(), yFor(hold[c]) - 1.0f, bar.getWidth(), 2.0f);
        }
    }

    // 目盛り
    g.setFont(font(9.0f));
    const int scaleX = meter.getX() + 2 * barW + 4;
    for (float db : { 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -36.0f, -48.0f }) {
        const float y = yFor(juce::Decibels::decibelsToGain(db));
        g.setColour(db == 0.0f ? colours::accent : colours::textDim);
        g.drawText(juce::String(juce::roundToInt(std::abs(db))), scaleX, juce::roundToInt(y) - 5, 16, 10,
                   juce::Justification::centredLeft);
    }
}

//==============================================================================
SettingsOverlay::SettingsOverlay(_3HSPlugAudioProcessor& p) : processor(p)
{
    setWantsKeyboardFocus(true);
    closeButton.onClick = [this] { close(); };
    addAndMakeVisible(closeButton);

    numChipsLabel.setText("Number of chips", juce::dontSendNotification);
    numChipsLabel.setFont(font(13.0f));
    addAndMakeVisible(numChipsLabel);
    for (int i = 1; i <= 16; ++i)
        numChipsBox.addItem(juce::String(i) + (i == 1 ? " chip" : " chips") + "  (" + juce::String(i * 8) + " FM voices)", i);
    numChipsBox.onChange = [this] { processor.setNumChips(numChipsBox.getSelectedId()); };
    addAndMakeVisible(numChipsBox);

    pcOverrideToggle.onClick = [this] { processor.setPcOverrideEnabled(pcOverrideToggle.getToggleState()); };
    addAndMakeVisible(pcOverrideToggle);

    auto setupNumber = [this](juce::Slider& s, juce::Label& label, const juce::String& name, int maxValue) {
        label.setText(name, juce::dontSendNotification);
        label.setFont(font(13.0f));
        addAndMakeVisible(label);
        s.setSliderStyle(juce::Slider::IncDecButtons);
        s.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 24);
        s.setRange(0, maxValue, 1);
        addAndMakeVisible(s);
    };
    setupNumber(pcBankSlider, pcBankLabel, "Bank", MAX_BANKS - 1);
    setupNumber(pcProgramSlider, pcProgramLabel, "Program", PATCH_BANK_SIZE - 1);
    pcBankSlider.onValueChange = [this] { processor.setPcOverrideBank(static_cast<int>(pcBankSlider.getValue())); };
    pcProgramSlider.onValueChange = [this] { processor.setPcOverrideProgram(static_cast<int>(pcProgramSlider.getValue())); };

    gmResetButton.setTooltip("Reset all MIDI channels and revert SysEx patch overrides");
    gmResetButton.onClick = [this] { processor.resetGM(); };
    panicButton.onClick = [this] { processor.allNotesOff(); };
    addAndMakeVisible(gmResetButton);
    addAndMakeVisible(panicButton);

    infoLabel.setFont(monoFont(12.0f));
    infoLabel.setJustificationType(juce::Justification::topLeft);
    infoLabel.setColour(juce::Label::textColourId, colours::textDim);
    addAndMakeVisible(infoLabel);
}

void SettingsOverlay::open()
{
    numChipsBox.setSelectedId(processor.getNumChips(), juce::dontSendNotification);
    pcOverrideToggle.setToggleState(processor.isPcOverrideEnabled(), juce::dontSendNotification);
    pcBankSlider.setValue(processor.getPcOverrideBank(), juce::dontSendNotification);
    pcProgramSlider.setValue(processor.getPcOverrideProgram(), juce::dontSendNotification);
    update();
    setVisible(true);
    toFront(true);
    grabKeyboardFocus();
}

void SettingsOverlay::close()
{
    setVisible(false);
}

void SettingsOverlay::update()
{
    if (!isVisible())
        return;
    const double sr = processor.getSampleRate();
    const int block = processor.getBlockSize();
    const double bufferMs = sr > 0.0 ? block * 1000.0 / sr : 0.0;
    auto pct = [bufferMs](double ms) { return bufferMs > 0.0 ? ms / bufferMs * 100.0 : 0.0; };
    juce::String info;
    info << "Sample rate   " << juce::String(sr, 0) << " Hz (core 48000 Hz)\n"
         << "Buffer        " << block << " samples (" << juce::String(bufferMs, 2) << " ms)\n"
         << "CPU           " << juce::String(processor.getCpuUsagePercent(), 1) << " %\n"
         << "  MIDI        " << juce::String(processor.getMidiProcessingTimeMs(), 3) << " ms ("
         << juce::String(pct(processor.getMidiProcessingTimeMs()), 1) << " %)\n"
         << "  Synth       " << juce::String(processor.getSynthProcessingTimeMs(), 3) << " ms ("
         << juce::String(pct(processor.getSynthProcessingTimeMs()), 1) << " %)\n"
         << "Patches       " << juce::String(processor.getPatchJsonPath()) << "\n"
         << "PCM           " << juce::String(processor.getPcmPath());
    infoLabel.setText(info, juce::dontSendNotification);
}

juce::Rectangle<int> SettingsOverlay::panelBounds() const
{
    return getLocalBounds().withSizeKeepingCentre(juce::jmin(560, getWidth() - 40), juce::jmin(470, getHeight() - 40));
}

void SettingsOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));
    auto panel = panelBounds();
    g.setColour(colours::panel);
    g.fillRoundedRectangle(panel.toFloat(), 8.0f);
    g.setColour(colours::accent);
    g.drawRoundedRectangle(panel.toFloat().reduced(0.5f), 8.0f, 1.0f);

    auto area = panel.reduced(18, 14);
    g.setColour(colours::text);
    g.setFont(font(18.0f, true));
    g.drawText("Settings", area.removeFromTop(28), juce::Justification::centredLeft);

    auto heading = [&](int y, const juce::String& text) {
        g.setColour(colours::accent);
        g.setFont(font(11.5f, true));
        g.drawText(text.toUpperCase(), area.getX(), y, area.getWidth(), 16, juce::Justification::centredLeft);
        g.setColour(colours::outline);
        g.drawHorizontalLine(y + 17, static_cast<float>(area.getX()), static_cast<float>(area.getRight()));
    };
    heading(numChipsLabel.getY() - 24, "Engine");
    heading(pcOverrideToggle.getY() - 24, "Program Change");
    heading(gmResetButton.getY() - 24, "Reset");
    heading(infoLabel.getY() - 22, "Status");
}

void SettingsOverlay::resized()
{
    auto area = panelBounds().reduced(18, 14);
    closeButton.setBounds(area.removeFromTop(28).removeFromRight(80).reduced(0, 2));

    area.removeFromTop(30);
    auto row = area.removeFromTop(26);
    numChipsLabel.setBounds(row.removeFromLeft(140));
    numChipsBox.setBounds(row.removeFromLeft(240));

    area.removeFromTop(32);
    pcOverrideToggle.setBounds(area.removeFromTop(24).removeFromLeft(260));
    area.removeFromTop(4);
    row = area.removeFromTop(26);
    pcBankLabel.setBounds(row.removeFromLeft(50));
    pcBankSlider.setBounds(row.removeFromLeft(130));
    row.removeFromLeft(20);
    pcProgramLabel.setBounds(row.removeFromLeft(64));
    pcProgramSlider.setBounds(row.removeFromLeft(130));

    area.removeFromTop(32);
    row = area.removeFromTop(26);
    gmResetButton.setBounds(row.removeFromLeft(110));
    row.removeFromLeft(10);
    panicButton.setBounds(row.removeFromLeft(120));

    area.removeFromTop(30);
    infoLabel.setBounds(area);
}

void SettingsOverlay::mouseDown(const juce::MouseEvent& e)
{
    if (!panelBounds().contains(e.getPosition()))
        close();
}

bool SettingsOverlay::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) {
        close();
        return true;
    }
    return false;
}

} // namespace ui
