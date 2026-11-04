#include "PatchEditorPanel.h"
#include "../VolumeScalingMap.h"
#include "../s3hs_core/3hs/3hs816/src/generated_tables.hpp"

namespace ui {

namespace {

const char* const kWaveNames[16] = {
    "Sine", "Abs. Sine", "Squished Sine", "Square", "Triangle", "Sawtooth", "Abs. Square", "Abs. Triangle",
    "Abs. Sawtooth", "Noise", "User A (silent)", "User B (silent)", "User C (silent)", "User D (silent)",
    "Abs. AC Sine", "Alternating Sine",
};

const char* const kModeNames[13] = {
    "Additive", "4x2OP FM", "4x2 RingMod", "2x4OP FM", "8OP FM", "4OP FM x2", "2OP FM x4",
    "4OP FMxRM x2", "2x4 RingMod", "2OP FMxRM x4", "2OP DirectPhase", "4OP DirectPhase", "8OP DirectPhase",
};

const char* const kGmNames[128] = {
    "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano",
    "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavi", "Celesta", "Glockenspiel", "Music Box",
    "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer", "Drawbar Organ", "Percussive Organ",
    "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
    "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
    "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics", "Acoustic Bass",
    "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2",
    "Synth Bass 1", "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings",
    "Pizzicato Strings", "Orchestral Harp", "Timpani", "String Ensemble 1", "String Ensemble 2",
    "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit", "Trumpet",
    "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2",
    "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
    "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
    "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)", "Lead 5 (charang)",
    "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)", "Pad 1 (new age)", "Pad 2 (warm)",
    "Pad 3 (polysynth)", "Pad 4 (choir)", "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
    "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)", "FX 5 (brightness)",
    "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)", "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba",
    "Bag pipe", "Fiddle", "Shanai", "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum",
    "Melodic Tom", "Synth Drum", "Reverse Cymbal", "Guitar Fret Noise", "Breath Noise", "Seashore",
    "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot",
};

// エンベロープの時間（秒）。3HS816のエンベロープ（envelope.cpp）と同じ式。
double attackSeconds(int a) { return a * 0.33 / 64.0; }
double decaySeconds(int d) { return (d == 0 ? 0.5 : d) * 1.5 / 64.0; }
double releaseSeconds(int r) { return r * 1.5 / 64.0; }

juce::String formatSeconds(double s)
{
    return s < 1.0 ? juce::String(juce::roundToInt(s * 1000.0)) + "ms" : juce::String(s, 2) + "s";
}

void setupByteKnob(Knob& knob)
{
    knob.slider.setRange(0, 255, 1);
    knob.slider.setDoubleClickReturnValue(false, 0);
}

} // namespace

//==============================================================================
Knob::Knob(const juce::String& n) : name(n)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
    slider.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
    slider.setMouseDragSensitivity(220);
    slider.setVelocityModeParameters(0.6, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
    addAndMakeVisible(slider);
}

void Knob::paint(juce::Graphics& g)
{
    g.setColour(isEnabled() ? colours::textDim : colours::textDim.withAlpha(0.4f));
    g.setFont(font(10.5f, true));
    g.drawText(name, getLocalBounds().removeFromTop(13), juce::Justification::centred);
}

void Knob::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(13);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, juce::jmin(64, area.getWidth()), 15);
    slider.setBounds(area);
}

//==============================================================================
OperatorPanel::OperatorPanel(int i) : index(i)
{
    for (int w = 0; w < 16; ++w)
        waveBox.addItem(juce::String::toHexString(w).toUpperCase() + "  " + kWaveNames[w], w + 1);
    waveBox.onChange = [this] { repaint(); if (onEdit) onEdit(); };
    addAndMakeVisible(waveBox);

    // 周波数比はQ12（4096 = 1.0倍）。OP1はノートの周波数で動くので比は使われない。
    ratio.slider.setRange(0, 65535, 1);
    ratio.slider.setMouseDragSensitivity(1200);
    ratio.slider.setDoubleClickReturnValue(true, 4096);
    ratio.slider.textFromValueFunction = [](double v) { return "x" + juce::String(v / 4096.0, 3); };
    ratio.slider.valueFromTextFunction = [](const juce::String& text) {
        return juce::jlimit(0.0, 65535.0, std::round(text.retainCharacters("0123456789.").getDoubleValue() * 4096.0));
    };
    ratio.slider.setTooltip("Frequency ratio (Q12, 4096 = x1). Type a ratio such as 2.5. Shift-drag for fine steps.");
    if (index == 0) {
        ratio.setEnabled(false);
        ratio.slider.setTooltip("OP1 always runs at the note frequency");
    }

    for (auto* k : { &level, &attack, &decay, &sustain, &release })
        setupByteKnob(*k);
    attack.slider.setTooltip("0-255: 0 to 1.31 s, linear");
    decay.slider.setTooltip("0-255: 23 ms to 5.98 s, exponential");
    sustain.slider.setTooltip("Sustain level, 0-255");
    release.slider.setTooltip("0-255: 0 to 5.98 s, exponential");

    for (auto* k : { &ratio, &level, &attack, &decay, &sustain, &release }) {
        k->slider.onValueChange = [this] { repaint(graphArea()); if (onEdit) onEdit(); };
        addAndMakeVisible(*k);
    }
}

void OperatorPanel::setOperator(const Operator& op)
{
    auto set = [](Knob& k, int v) {
        if (!k.slider.isMouseButtonDown())
            k.slider.setValue(v, juce::dontSendNotification);
    };
    set(ratio, op.frequency);
    set(level, op.volume);
    set(attack, op.attack);
    set(decay, op.decay);
    set(sustain, op.sustain);
    set(release, op.release);
    waveBox.setSelectedId((op.waveform & 0x0F) + 1, juce::dontSendNotification);
    repaint();
}

void OperatorPanel::writeTo(Operator& op) const
{
    if (index != 0)
        op.frequency = static_cast<uint16_t>(ratio.slider.getValue());
    op.volume = static_cast<uint8_t>(level.slider.getValue());
    op.attack = static_cast<uint8_t>(attack.slider.getValue());
    op.decay = static_cast<uint8_t>(decay.slider.getValue());
    op.sustain = static_cast<uint8_t>(sustain.slider.getValue());
    op.release = static_cast<uint8_t>(release.slider.getValue());
    op.waveform = static_cast<uint8_t>(juce::jlimit(0, 15, waveBox.getSelectedId() - 1));
}

void OperatorPanel::setRole(bool isCarrier, bool isFeedbackTarget)
{
    if (carrier == isCarrier && feedbackTarget == isFeedbackTarget)
        return;
    carrier = isCarrier;
    feedbackTarget = isFeedbackTarget;
    const auto c = carrier ? colours::carrier : colours::modulator;
    for (auto* k : { &ratio, &level, &attack, &decay, &sustain, &release })
        k->slider.setColour(juce::Slider::rotarySliderFillColourId, c);
    repaint();
}

juce::Rectangle<int> OperatorPanel::graphArea() const
{
    // ノブ2段（1段62px）を優先し、残りが少なければ波形/エンベロープ表示を省く
    const int spare = getHeight() - 30 - 2 * 62 - 12;
    const int h = spare < 24 ? 0 : juce::jmin(56, spare);
    return getLocalBounds().reduced(8, 0).withTrimmedTop(30).withHeight(h);
}

void OperatorPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto roleColour = carrier ? colours::carrier : colours::modulator;
    g.setColour(colours::panelRaised.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(roleColour.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);
    g.setColour(roleColour);
    g.fillRoundedRectangle(bounds.withHeight(3.0f).reduced(4.0f, 0.0f), 1.5f);

    auto header = getLocalBounds().reduced(8, 0).withTrimmedTop(5).withHeight(22);
    g.setColour(colours::text);
    g.setFont(font(15.0f, true));
    g.drawText("OP" + juce::String(index + 1), header.removeFromLeft(36), juce::Justification::centredLeft);
    g.setColour(roleColour);
    g.setFont(font(10.0f, true));
    juce::String role = carrier ? "CARRIER" : "MOD";
    if (feedbackTarget)
        role << "  FB";
    g.drawText(role, header.removeFromLeft(70), juce::Justification::centredLeft);

    auto graph = graphArea().toFloat();
    if (graph.isEmpty())
        return;
    auto waveRect = graph.removeFromLeft(juce::jmin(64.0f, graph.getWidth() * 0.3f));
    graph.removeFromLeft(6.0f);
    drawWave(g, waveRect);
    drawEnvelope(g, graph);
}

void OperatorPanel::drawWave(juce::Graphics& g, juce::Rectangle<float> r) const
{
    g.setColour(colours::background);
    g.fillRoundedRectangle(r, 3.0f);
    const int wave = juce::jlimit(0, 15, waveBox.getSelectedId() - 1);
    const auto inner = r.reduced(3.0f, 4.0f);
    g.setColour(colours::outline);
    g.drawHorizontalLine(juce::roundToInt(inner.getCentreY()), inner.getX(), inner.getRight());
    juce::Path path;
    for (int i = 0; i < 256; i += 2) {
        const float x = inner.getX() + inner.getWidth() * i / 255.0f;
        const float y = inner.getCentreY() - inner.getHeight() * 0.5f * hs816::tables::kWaves[wave][i] / 128.0f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour((carrier ? colours::carrier : colours::modulator).withAlpha(isEnabled() ? 1.0f : 0.4f));
    g.strokePath(path, juce::PathStrokeType(1.3f));
}

void OperatorPanel::drawEnvelope(juce::Graphics& g, juce::Rectangle<float> r) const
{
    g.setColour(colours::background);
    g.fillRoundedRectangle(r, 3.0f);
    const int a = static_cast<int>(attack.slider.getValue());
    const int d = static_cast<int>(decay.slider.getValue());
    const int s = static_cast<int>(sustain.slider.getValue());
    const int rl = static_cast<int>(release.slider.getValue());
    const float peakLevel = static_cast<float>(level.slider.getValue()) / 255.0f;

    // 区間ごとの幅は対数で圧縮する（短い値も見えるように）
    auto squash = [](double secs, double maxSecs) {
        return static_cast<float>(std::log1p(secs * 20.0) / std::log1p(maxSecs * 20.0));
    };
    auto inner = r.reduced(4.0f, 4.0f);
    const float w = inner.getWidth();
    const float wa = w * 0.22f * squash(attackSeconds(a), attackSeconds(255));
    const float wd = w * 0.32f * squash(decaySeconds(d), decaySeconds(255));
    const float ws = w * 0.14f;
    const float wr = w * 0.32f * squash(releaseSeconds(rl), releaseSeconds(255));
    const float sus = s / 255.0f;

    auto yFor = [&](float lvl) { return inner.getBottom() - inner.getHeight() * lvl * juce::jmax(0.15f, peakLevel); };
    juce::Path path;
    float x = inner.getX();
    path.startNewSubPath(x, inner.getBottom());
    x += wa;
    path.lineTo(x, yFor(1.0f));
    const int steps = 24;
    for (int i = 1; i <= steps; ++i) { // Decay: 1 - 2^(-10t)
        const float t = static_cast<float>(i) / steps;
        const float shape = 1.0f - std::pow(2.0f, -10.0f * t);
        path.lineTo(x + wd * t, yFor(1.0f + (sus - 1.0f) * shape));
    }
    x += wd;
    path.lineTo(x + ws, yFor(sus));
    x += ws;
    for (int i = 1; i <= steps; ++i) { // Release
        const float t = static_cast<float>(i) / steps;
        const float shape = 1.0f - std::pow(2.0f, -10.0f * t);
        path.lineTo(x + wr * t, yFor(sus * (1.0f - shape)));
    }
    if (rl == 0)
        path.lineTo(x, inner.getBottom());

    const auto c = (carrier ? colours::carrier : colours::modulator).withAlpha(isEnabled() ? 1.0f : 0.4f);
    juce::Path fill(path);
    fill.lineTo(path.getCurrentPosition().x, inner.getBottom());
    fill.closeSubPath();
    g.setColour(c.withAlpha(0.15f));
    g.fillPath(fill);
    g.setColour(c);
    g.strokePath(path, juce::PathStrokeType(1.4f));

    g.setColour(colours::textDim);
    g.setFont(font(9.0f));
    g.drawText("A " + formatSeconds(attackSeconds(a)) + "  D " + formatSeconds(decaySeconds(d))
                   + "  R " + formatSeconds(releaseSeconds(rl)),
               r.reduced(4.0f, 1.0f), juce::Justification::topRight, false);
}

void OperatorPanel::resized()
{
    auto header = getLocalBounds().reduced(8, 0).withTrimmedTop(5).withHeight(22);
    waveBox.setBounds(header.removeFromRight(juce::jmin(140, header.getWidth() - 100)));

    auto area = getLocalBounds().reduced(6, 4);
    area.removeFromTop(juce::jmax(graphArea().getBottom(), 28) - area.getY() + 4);
    const int rowH = area.getHeight() / 2;
    auto row1 = area.removeFromTop(rowH);
    auto row2 = area;
    const int w1 = row1.getWidth() / 3;
    for (auto* k : { &ratio, &level, &attack })
        k->setBounds(row1.removeFromLeft(w1).reduced(1));
    const int w2 = row2.getWidth() / 3;
    for (auto* k : { &decay, &sustain, &release })
        k->setBounds(row2.removeFromLeft(w2).reduced(1));
}

//==============================================================================
PatchEditorPanel::PatchEditorPanel(_3HSPlugAudioProcessor& p) : processor(p)
{
    for (int ch = 1; ch <= 16; ++ch)
        channelBox.addItem("CH " + juce::String(ch), ch);
    channelBox.setSelectedId(channel, juce::dontSendNotification);
    channelBox.onChange = [this] {
        setChannel(channelBox.getSelectedId());
        if (onChannelChanged)
            onChannelChanged(channel);
    };
    addAndMakeVisible(channelBox);

    for (int m = 0; m < 13; ++m)
        modeBox.addItem(juce::String(m) + ": " + kModeNames[m], m + 1);
    modeBox.onChange = [this] { updateRoles(); pushEdit(); };
    modeBox.setTooltip("Modulation mode (algorithm)");
    addAndMakeVisible(modeBox);

    for (int op = 0; op < 8; ++op)
        fbDestBox.addItem("FB -> OP" + juce::String(op + 1), op + 1);
    fbDestBox.onChange = [this] { updateRoles(); pushEdit(); };
    fbDestBox.setTooltip("Feedback destination operator");
    addAndMakeVisible(fbDestBox);

    feedback.slider.setRange(0, 255, 1);
    feedback.slider.setDoubleClickReturnValue(true, 128);
    feedback.slider.onValueChange = [this] { pushEdit(); };
    addAndMakeVisible(feedback);

    keyShift.slider.setRange(-48, 48, 1);
    keyShift.slider.setDoubleClickReturnValue(true, 0);
    keyShift.slider.textFromValueFunction = [](double v) {
        const int n = juce::roundToInt(v);
        return (n > 0 ? "+" : "") + juce::String(n);
    };
    keyShift.slider.onValueChange = [this] { pushEdit(); };
    addAndMakeVisible(keyShift);

    revertButton.setTooltip("Revert this patch to the contents of the JSON file");
    revertButton.onClick = [this] {
        processor.revertChannelPatch(channel);
        loadedRevision = ~0u;
        refresh();
    };
    addAndMakeVisible(revertButton);

    saveButton.setTooltip("Write this patch's bank to <patches>/<bank>.json");
    saveButton.onClick = [this] {
        processor.saveChannelPatchBank(channel, saveMessage);
        saveMessageFrames = 60 * 4;
        updateStatus();
    };
    addAndMakeVisible(saveButton);

    programLabel.setFont(font(16.0f, true));
    addAndMakeVisible(programLabel);
    statusLabel.setFont(font(11.5f));
    statusLabel.setColour(juce::Label::textColourId, colours::textDim);
    addAndMakeVisible(statusLabel);

    drumNotice.setText("This channel plays drum PCM samples. Select another channel to edit an FM patch.",
                       juce::dontSendNotification);
    drumNotice.setJustificationType(juce::Justification::centred);
    drumNotice.setFont(font(15.0f, true));
    drumNotice.setColour(juce::Label::backgroundColourId, colours::panel.withAlpha(0.85f));

    for (int i = 0; i < 8; ++i) {
        operators[static_cast<size_t>(i)] = std::make_unique<OperatorPanel>(i);
        operators[static_cast<size_t>(i)]->onEdit = [this] { pushEdit(); };
        addAndMakeVisible(*operators[static_cast<size_t>(i)]);
    }
    addChildComponent(drumNotice); // オペレーターの上に重ねる
    refresh();
}

void PatchEditorPanel::setChannel(int midiChannel)
{
    midiChannel = juce::jlimit(1, 16, midiChannel);
    channelBox.setSelectedId(midiChannel, juce::dontSendNotification);
    if (midiChannel == channel)
        return;
    channel = midiChannel;
    loadedRevision = ~0u;
    refresh();
}

void PatchEditorPanel::refresh()
{
    const auto location = processor.getChannelPatchLocation(channel);
    const bool drum = processor.isDrumChannel(channel);
    const auto revision = processor.getPatchRevision();
    if (revision != loadedRevision || location.bank != loadedBank || location.program != loadedProgram
        || drum != drumChannel) {
        loadedRevision = revision;
        loadedBank = location.bank;
        loadedProgram = location.program;
        drumChannel = drum;
        loadFromProcessor();
    }
    if (saveMessageFrames > 0 && --saveMessageFrames == 0)
        updateStatus();
}

void PatchEditorPanel::loadFromProcessor()
{
    const juce::ScopedValueSetter<bool> guard(loading, true);
    patch = processor.getChannelPatch(channel);

    modeBox.setSelectedId(juce::jlimit(0, 12, static_cast<int>(patch.modmode)) + 1, juce::dontSendNotification);
    fbDestBox.setSelectedId(juce::jlimit(0, 7, static_cast<int>(patch.fbDest)) + 1, juce::dontSendNotification);
    if (!feedback.slider.isMouseButtonDown())
        feedback.slider.setValue(patch.feedback, juce::dontSendNotification);
    if (!keyShift.slider.isMouseButtonDown())
        keyShift.slider.setValue(patch.keyShift, juce::dontSendNotification);
    for (size_t i = 0; i < operators.size(); ++i)
        operators[i]->setOperator(patch.operators[i]);
    updateRoles();

    for (auto* c : std::initializer_list<juce::Component*> { &modeBox, &fbDestBox, &feedback, &keyShift, &revertButton, &saveButton })
        c->setEnabled(!drumChannel);
    for (auto& op : operators)
        op->setEnabled(!drumChannel);
    drumNotice.setVisible(drumChannel);
    updateStatus();
}

void PatchEditorPanel::pushEdit()
{
    if (loading || drumChannel)
        return;
    patch.modmode = static_cast<uint8_t>(juce::jlimit(0, 12, modeBox.getSelectedId() - 1));
    patch.fbDest = static_cast<uint8_t>(juce::jlimit(0, 7, fbDestBox.getSelectedId() - 1));
    patch.feedback = static_cast<uint8_t>(feedback.slider.getValue());
    patch.keyShift = static_cast<int8_t>(keyShift.slider.getValue());
    for (size_t i = 0; i < operators.size(); ++i)
        operators[i]->writeTo(patch.operators[i]);
    processor.editChannelPatch(channel, patch);
    // 自分の編集は読み込み直さない（ドラッグ中の値を上書きしないため）
    loadedRevision = processor.getPatchRevision();
    updateStatus();
}

void PatchEditorPanel::updateRoles()
{
    const int mode = juce::jlimit(0, 12, modeBox.getSelectedId() - 1);
    const int fbDest = juce::jlimit(0, 7, fbDestBox.getSelectedId() - 1);
    for (int i = 0; i < 8; ++i)
        operators[static_cast<size_t>(i)]->setRole(volumeScalingMap[mode][i], i == fbDest);
}

void PatchEditorPanel::updateStatus()
{
    const auto location = processor.getChannelPatchLocation(channel);
    const int prog = location.program;
    programLabel.setText(drumChannel ? "Drum Kit"
                                     : juce::String(location.bank).paddedLeft('0', 3) + ":"
                                           + juce::String(prog + 1).paddedLeft('0', 3) + "  "
                                           + (prog >= 0 && prog < 128 ? kGmNames[prog] : ""),
                         juce::dontSendNotification);

    juce::String status;
    juce::Colour colour = colours::textDim;
    if (saveMessageFrames > 0) {
        status = saveMessage;
        colour = colours::accent;
    } else if (drumChannel) {
        status = "PCM drum channel";
    } else if (location.availability == 2) {
        status = location.modified ? "Edited (not saved to JSON; kept in the DAW project)" : "Patch from JSON";
        colour = location.modified ? colours::accent : colours::textDim;
    } else if (location.availability == 1) {
        status = "Not in bank " + juce::String(location.bank) + "; using bank 0. Editing creates it in bank "
                 + juce::String(location.bank) + ".";
        colour = colours::meterYellow;
    } else {
        status = "Undefined program; using the default patch. Editing creates it.";
        colour = colours::meterRed;
    }
    statusLabel.setText(status, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, colour);
    revertButton.setEnabled(!drumChannel && location.modified);
}

juce::Rectangle<int> PatchEditorPanel::globalArea() const
{
    return getLocalBounds().withTrimmedTop(22).reduced(8, 2).removeFromTop(64);
}

juce::Rectangle<int> PatchEditorPanel::operatorsArea() const
{
    auto area = getLocalBounds().withTrimmedTop(22).reduced(8, 2);
    area.removeFromTop(64 + 6);
    return area.withTrimmedBottom(6);
}

void PatchEditorPanel::paint(juce::Graphics& g)
{
    drawSection(g, getLocalBounds(), "Patch Editor");
    g.setColour(colours::outline);
    const auto ops = operatorsArea();
    g.drawHorizontalLine(ops.getY() - 4, static_cast<float>(ops.getX()), static_cast<float>(ops.getRight()));
}

void PatchEditorPanel::resized()
{
    auto global = globalArea();
    auto left = global.removeFromLeft(juce::jmin(420, global.getWidth() / 2));
    auto top = left.removeFromTop(30);
    channelBox.setBounds(top.removeFromLeft(80).reduced(0, 3));
    top.removeFromLeft(10);
    programLabel.setBounds(top);
    statusLabel.setBounds(left.removeFromTop(20));

    auto buttons = global.removeFromRight(100);
    revertButton.setBounds(buttons.removeFromTop(30).reduced(0, 3));
    saveButton.setBounds(buttons.removeFromTop(30).reduced(0, 3));
    global.removeFromRight(10);

    keyShift.setBounds(global.removeFromRight(64));
    feedback.setBounds(global.removeFromRight(64));
    global.removeFromRight(8);
    auto combos = global.removeFromRight(juce::jmin(190, global.getWidth()));
    modeBox.setBounds(combos.removeFromTop(30).reduced(0, 3));
    fbDestBox.setBounds(combos.removeFromTop(30).reduced(0, 3));

    auto ops = operatorsArea();
    drumNotice.setBounds(ops);
    const int cols = 4;
    const int gap = 6;
    const int w = (ops.getWidth() - gap * (cols - 1)) / cols;
    const int h = (ops.getHeight() - gap) / 2;
    for (int i = 0; i < 8; ++i) {
        const int c = i % cols, r = i / cols;
        operators[static_cast<size_t>(i)]->setBounds(ops.getX() + c * (w + gap), ops.getY() + r * (h + gap), w, h);
    }
}

} // namespace ui
