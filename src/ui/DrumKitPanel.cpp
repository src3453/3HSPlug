#include "DrumKitPanel.h"

namespace ui {

namespace {

// GM/GSドラムの名前（27～87）
const char* gmDrumName(int note)
{
    static const char* const names[] = {
        "High Q", "Slap", "Scratch Push", "Scratch Pull", "Sticks", "Square Click", "Metronome Click",
        "Metronome Bell", "Acoustic Bass Drum", "Bass Drum 1", "Side Stick", "Acoustic Snare", "Hand Clap",
        "Electric Snare", "Low Floor Tom", "Closed Hi-Hat", "High Floor Tom", "Pedal Hi-Hat", "Low Tom",
        "Open Hi-Hat", "Low-Mid Tom", "Hi-Mid Tom", "Crash Cymbal 1", "High Tom", "Ride Cymbal 1",
        "Chinese Cymbal", "Ride Bell", "Tambourine", "Splash Cymbal", "Cowbell", "Crash Cymbal 2", "Vibraslap",
        "Ride Cymbal 2", "Hi Bongo", "Low Bongo", "Mute Hi Conga", "Open Hi Conga", "Low Conga",
        "High Timbale", "Low Timbale", "High Agogo", "Low Agogo", "Cabasa", "Maracas", "Short Whistle",
        "Long Whistle", "Short Guiro", "Long Guiro", "Claves", "Hi Wood Block", "Low Wood Block", "Mute Cuica",
        "Open Cuica", "Mute Triangle", "Open Triangle", "Shaker", "Jingle Bell", "Bell Tree", "Castanets",
        "Mute Surdo", "Open Surdo",
    };
    return (note >= 27 && note <= 87) ? names[note - 27] : "";
}

juce::String noteLabel(int note)
{
    return juce::String(note) + " " + juce::MidiMessage::getMidiNoteName(note, true, true, 4);
}

juce::String bytesText(uint64_t bytes)
{
    if (bytes >= 1024 * 1024)
        return juce::String(bytes / (1024.0 * 1024.0), 2) + " MB";
    return juce::String(bytes / 1024.0, 1) + " KB";
}

const juce::String kAudioWildcard = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3";

bool isAudioFile(const juce::String& path)
{
    return juce::File(path).hasFileExtension("wav;aif;aiff;flac;ogg;mp3");
}

} // namespace

bool DrumKitPanel::hasCustomAssignments() const
{
    for (const auto& s : slots)
        if (!s.isDefault)
            return true;
    return false;
}

void DrumKitPanel::confirm(const juce::String& title, const juce::String& text, const juce::String& action,
                           std::function<void()> onConfirm)
{
    const juce::Component::SafePointer<DrumKitPanel> safe(this);
    juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                     .withIconType(juce::MessageBoxIconType::WarningIcon)
                                     .withTitle(title)
                                     .withMessage(text)
                                     .withButton(action)
                                     .withButton("Cancel")
                                     .withAssociatedComponent(this),
                                 [safe, onConfirm](int result) {
                                     if (safe != nullptr && result == 1)
                                         onConfirm();
                                 });
}

void DrumKitPanel::requestClear(int note)
{
    const auto& s = slots[static_cast<size_t>(note)];
    if (!s.assigned && !s.missing)
        return;
    confirm("Clear " + noteLabel(note) + "?",
            "The note stops playing a sample. Assign the file again to undo this.", "Clear",
            [this, note] {
                processor.clearDrumSample(note);
                showMessage("Cleared " + noteLabel(note), false);
            });
}

void DrumKitPanel::loadKit()
{
    chooser = std::make_unique<juce::FileChooser>("Load drum kit", processor.getDrumSlot(36).file.getParentDirectory(), "*.json");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this](const juce::FileChooser& fc) {
                             const auto file = fc.getResult();
                             if (file == juce::File())
                                 return;
                             const auto result = processor.loadDrumKitFile(file);
                             showMessage(result.wasOk() ? "Loaded " + file.getFileName() : result.getErrorMessage(), result.failed());
                         });
}

DrumKitPanel::DrumKitPanel(_3HSPlugAudioProcessor& p) : processor(p)
{
    setWantsKeyboardFocus(true);
    setComponentID("ownFocusRing"); // 選択中のセルに自分でリングを描く
    setTitle("Drum kit notes");
    setDescription("Arrow keys choose a note, Space plays it, Enter assigns a sample, Delete clears it");
    loadKitButton.setTooltip("Replace the kit with a drum kit JSON file");
    loadKitButton.onClick = [this] {
        if (!hasCustomAssignments()) {
            loadKit();
            return;
        }
        confirm("Replace the drum kit?",
                "Loading a kit file replaces every note assignment you have made. Save the kit first to keep them.",
                "Choose kit file", [this] { loadKit(); });
    };
    saveKitButton.setTooltip("Write the current note assignments to a drum kit JSON file");
    saveKitButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser>("Save drum kit", juce::File(), "*.json");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [this](const juce::FileChooser& fc) {
                                 auto file = fc.getResult();
                                 if (file == juce::File())
                                     return;
                                 if (!file.hasFileExtension("json"))
                                     file = file.withFileExtension("json");
                                 const auto result = processor.saveDrumKitFile(file);
                                 showMessage(result.wasOk() ? "Saved " + file.getFullPathName() : result.getErrorMessage(), result.failed());
                             });
    };
    resetButton.setTooltip("Go back to the <note>.wav files in the PCM folder");
    resetButton.onClick = [this] {
        const auto doReset = [this] {
            processor.resetDrumKit();
            showMessage("Kit reset to the PCM folder", false);
        };
        if (!hasCustomAssignments()) {
            doReset();
            return;
        }
        confirm("Reset the drum kit?",
                "Every note goes back to the <note>.wav file in the PCM folder, and your assignments are lost. "
                "Save the kit first to keep them.",
                "Reset", doReset);
    };
    assignButton.onClick = [this] { chooseSample(selectedNote); };
    clearButton.onClick = [this] { requestClear(selectedNote); };
    playButton.onClick = [this] { processor.auditionDrum(selectedNote); };

    for (auto* b : { &loadKitButton, &saveKitButton, &resetButton, &assignButton, &clearButton, &playButton })
        addAndMakeVisible(*b);
    reloadSlots();
}

void DrumKitPanel::reloadSlots()
{
    for (int n = 0; n < 128; ++n)
        slots[static_cast<size_t>(n)] = processor.getDrumSlot(n);
    memoryUsed = processor.getDrumMemoryUsed();
    loadedRevision = processor.getDrumKitRevision();
    const bool has = slots[static_cast<size_t>(selectedNote)].assigned;
    clearButton.setEnabled(has || slots[static_cast<size_t>(selectedNote)].missing);
    playButton.setEnabled(has);
    repaint();
}

void DrumKitPanel::update()
{
    if (!isVisible())
        return;
    if (processor.getDrumKitRevision() != loadedRevision)
        reloadSlots();

    for (auto& f : flash)
        f = juce::jmax(0.0f, f - 0.06f);
    const uint64_t now = processor.getCurrentTick();
    for (const auto& d : processor.drumPcmChannelStates)
        if (d.inUse && d.noteNumber >= 0 && d.noteNumber < 128 && now - d.lastUsedTick < 2400)
            flash[static_cast<size_t>(d.noteNumber)] = 1.0f;
    if (messageFrames > 0)
        --messageFrames;
    repaint(gridArea().expanded(2));
    if (messageFrames == 0 && message.isNotEmpty()) {
        message.clear();
        repaint(detailArea());
    }
}

juce::Rectangle<int> DrumKitPanel::memoryArea() const
{
    return getLocalBounds().removeFromTop(space::control);
}

juce::Rectangle<int> DrumKitPanel::detailArea() const
{
    return getLocalBounds().removeFromBottom(64);
}

juce::Rectangle<int> DrumKitPanel::gridArea() const
{
    return getLocalBounds().withTrimmedTop(space::control + space::sm).withTrimmedBottom(64 + space::sm);
}

juce::Rectangle<float> DrumKitPanel::cellBounds(int note) const
{
    const auto area = gridArea().toFloat();
    const float w = area.getWidth() / kColumns;
    const float h = area.getHeight() / kRows;
    const int c = note % kColumns, r = note / kColumns;
    return { area.getX() + c * w, area.getY() + r * h, w, h };
}

int DrumKitPanel::noteAt(juce::Point<int> pos) const
{
    const auto area = gridArea();
    if (!area.contains(pos))
        return -1;
    const int c = juce::jlimit(0, kColumns - 1, (pos.x - area.getX()) * kColumns / juce::jmax(1, area.getWidth()));
    const int r = juce::jlimit(0, kRows - 1, (pos.y - area.getY()) * kRows / juce::jmax(1, area.getHeight()));
    return r * kColumns + c;
}

void DrumKitPanel::paint(juce::Graphics& g)
{
    // PCMメモリ使用量
    auto mem = memoryArea().withTrimmedRight(340); // 右はキットのボタン
    g.setColour(colours::textDim);
    g.setFont(font(type::small, true));
    g.drawText("PCM memory", mem.removeFromLeft(90), juce::Justification::centredLeft);
    auto bar = mem.removeFromLeft(juce::jlimit(60, 360, mem.getWidth() - 200)).reduced(0, 8).toFloat();
    mem = mem.removeFromLeft(200);
    const float ratio = static_cast<float>(memoryUsed) / _3HSPlugAudioProcessor::drumMemoryCapacity;
    g.setColour(colours::background);
    g.fillRoundedRectangle(bar, shape::r(3.0f));
    g.setColour(ratio > 0.9f ? colours::meterRed : ratio > 0.7f ? colours::meterYellow : colours::accent);
    g.fillRoundedRectangle(bar.withWidth(juce::jmax(2.0f, bar.getWidth() * ratio)), shape::r(3.0f));
    g.setColour(colours::text);
    g.setFont(monoFont(type::small));
    g.drawText(bytesText(memoryUsed) + " / 4.00 MB (" + juce::String(ratio * 100.0f, 1) + "%)",
               mem.withTrimmedLeft(8), juce::Justification::centredLeft);

    // ノートのグリッド
    for (int note = 0; note < 128; ++note) {
        const auto& s = slots[static_cast<size_t>(note)];
        auto cell = cellBounds(note).reduced(1.5f);
        const bool selected = note == selectedNote;
        const bool isC = note % 12 == 0;

        auto fill = s.assigned ? colours::panelRaised : colours::panel.darker(0.25f);
        g.setColour(fill);
        g.fillRoundedRectangle(cell, shape::r(3.0f));
        if (flash[static_cast<size_t>(note)] > 0.0f) {
            g.setColour(colours::accent.withAlpha(0.55f * flash[static_cast<size_t>(note)]));
            g.fillRoundedRectangle(cell, shape::r(3.0f));
        }

        auto text = cell.reduced(6.0f, 2.0f);
        const float lineH = juce::jmin(13.0f, text.getHeight() / 2.0f);
        g.setColour(s.assigned ? colours::text : colours::textDim);
        g.setFont(font(type::caption, isC || s.assigned));
        g.drawText(noteLabel(note), text.removeFromTop(lineH), juce::Justification::centredLeft, false);
        juce::String second;
        if (s.missing)
            second = "! " + s.file.getFileNameWithoutExtension();
        else if (s.assigned)
            second = s.isDefault && *gmDrumName(note) ? juce::String(gmDrumName(note)) : s.file.getFileNameWithoutExtension();
        else
            second = gmDrumName(note);
        // 既定（PCMフォルダ）は控えめ、自分で割り当てたファイルはアクセント（状態）で示す
        g.setColour(s.missing ? colours::meterRed : (s.assigned && !s.isDefault) ? colours::accent : colours::textDim);
        g.setFont(font(type::caption));
        g.drawText(second, text.removeFromTop(lineH), juce::Justification::centredLeft, true);

        if (s.missing) {
            g.setColour(colours::meterRed);
            g.drawRoundedRectangle(cell, shape::r(3.0f), 1.0f);
        }
        if (selected && showFocusRing) {
            drawFocusRing(g, cell.expanded(1.5f), 3.0f);
        } else if (note == dropNote) {
            g.setColour(colours::meterGreen);
            g.drawRoundedRectangle(cell, shape::r(3.0f), 2.0f);
        } else if (selected) {
            g.setColour(colours::accent);
            g.drawRoundedRectangle(cell, shape::r(3.0f), 1.6f);
        } else if (note == hoverNote) {
            g.setColour(colours::outline.brighter(0.4f));
            g.drawRoundedRectangle(cell, shape::r(3.0f), 1.0f);
        }
    }

    // 選択中のノートの詳細
    auto detail = detailArea();
    g.setColour(colours::panelRaised.withAlpha(0.5f));
    g.fillRoundedRectangle(detail.toFloat(), shape::r(5.0f));
    detail.reduce(10, 6);
    detail.removeFromRight(300); // ボタン
    const auto& s = slots[static_cast<size_t>(selectedNote)];
    const auto noteName = juce::MidiMessage::getMidiNoteName(selectedNote, true, true, 4);
    juce::String title = *gmDrumName(selectedNote)
                             ? juce::String(gmDrumName(selectedNote)) + ", note " + juce::String(selectedNote) + " (" + noteName + ")"
                             : "Note " + juce::String(selectedNote) + " (" + noteName + ")";
    g.setColour(colours::text);
    g.setFont(font(type::title, true));
    g.drawText(title, detail.removeFromTop(18), juce::Justification::centredLeft);

    g.setFont(font(type::small));
    if (message.isNotEmpty()) {
        g.setColour(messageIsError ? colours::meterRed : colours::accent);
        g.drawText(message, detail.removeFromTop(16), juce::Justification::centredLeft, true);
    } else if (s.missing) {
        g.setColour(colours::meterRed);
        g.drawText(s.error + ": " + s.file.getFullPathName(), detail.removeFromTop(16), juce::Justification::centredLeft, true);
    } else if (s.assigned) {
        g.setColour(colours::textDim);
        g.drawText(s.file.getFullPathName() + (s.isDefault ? "  (PCM folder)" : ""), detail.removeFromTop(16),
                   juce::Justification::centredLeft, true);
    } else {
        g.setColour(colours::textDim);
        g.drawText("No sample. Drop an audio file on a note or press Assign.", detail.removeFromTop(16),
                   juce::Justification::centredLeft, true);
    }
    if (s.assigned) {
        g.setColour(colours::textDim);
        g.setFont(monoFont(type::caption));
        const double secs = s.sampleRate > 0 ? static_cast<double>(s.length) / s.sampleRate : 0.0;
        g.drawText(juce::String(s.sampleRate) + " Hz   " + bytesText(s.length) + "   " + juce::String(secs, 2) + " s   8-bit mono",
                   detail.removeFromTop(16), juce::Justification::centredLeft, true);
    }
}

void DrumKitPanel::resized()
{
    auto top = memoryArea();
    resetButton.setBounds(top.removeFromRight(124));
    top.removeFromRight(6);
    saveKitButton.setBounds(top.removeFromRight(100));
    top.removeFromRight(6);
    loadKitButton.setBounds(top.removeFromRight(100));
    layoutDetailButtons();
}

void DrumKitPanel::layoutDetailButtons()
{
    auto buttons = detailArea().reduced(space::md, 0).withSizeKeepingCentre(detailArea().getWidth() - 2 * space::md, space::control)
                       .removeFromRight(290);
    playButton.setBounds(buttons.removeFromRight(80));
    buttons.removeFromRight(8);
    clearButton.setBounds(buttons.removeFromRight(80));
    buttons.removeFromRight(8);
    assignButton.setBounds(buttons.removeFromRight(100));
}

void DrumKitPanel::select(int note, bool play)
{
    selectedNote = juce::jlimit(0, 127, note);
    message.clear();
    messageFrames = 0;
    const auto& s = slots[static_cast<size_t>(selectedNote)];
    clearButton.setEnabled(s.assigned || s.missing);
    playButton.setEnabled(s.assigned);
    if (play && s.assigned)
        processor.auditionDrum(selectedNote);
    repaint();
}

bool DrumKitPanel::keyPressed(const juce::KeyPress& key)
{
    int note = selectedNote;
    if (key == juce::KeyPress::leftKey) note -= 1;
    else if (key == juce::KeyPress::rightKey) note += 1;
    else if (key == juce::KeyPress::upKey) note -= kColumns;
    else if (key == juce::KeyPress::downKey) note += kColumns;
    else if (key == juce::KeyPress::spaceKey) { processor.auditionDrum(selectedNote); return true; }
    else if (key == juce::KeyPress::returnKey) { chooseSample(selectedNote); return true; }
    else if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) { requestClear(selectedNote); return true; }
    else return false;
    showFocusRing = true;
    select(juce::jlimit(0, 127, note), false);
    return true;
}

void DrumKitPanel::focusGained(FocusChangeType cause)
{
    showFocusRing = cause != focusChangedByMouseClick;
    repaint();
}

void DrumKitPanel::focusLost(FocusChangeType)
{
    showFocusRing = false;
    repaint();
}

void DrumKitPanel::mouseDown(const juce::MouseEvent& e)
{
    showFocusRing = false;
    const int note = noteAt(e.getPosition());
    if (note < 0)
        return;
    if (e.mods.isPopupMenu()) {
        select(note, false);
        showMenu(note);
        return;
    }
    select(note, true); // クリックで試聴
}

void DrumKitPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const int note = noteAt(e.getPosition()); note >= 0)
        chooseSample(note);
}

void DrumKitPanel::mouseMove(const juce::MouseEvent& e)
{
    const int note = noteAt(e.getPosition());
    if (note != hoverNote) {
        hoverNote = note;
        setTooltip(note >= 0 ? getTooltipForNote(note) : juce::String());
        repaint(gridArea().expanded(2));
    }
}

void DrumKitPanel::mouseExit(const juce::MouseEvent&)
{
    hoverNote = -1;
    repaint(gridArea().expanded(2));
}

juce::String DrumKitPanel::getTooltipForNote(int note) const
{
    const auto& s = slots[static_cast<size_t>(note)];
    juce::String tip = noteLabel(note);
    if (*gmDrumName(note))
        tip << " (" << gmDrumName(note) << ")";
    if (s.assigned || s.missing)
        tip << "\n" << s.file.getFullPathName();
    tip << "\nClick: play   Double-click: assign   Right-click: menu";
    return tip;
}

void DrumKitPanel::showMenu(int note)
{
    juce::PopupMenu menu;
    const auto& s = slots[static_cast<size_t>(note)];
    menu.addSectionHeader(noteLabel(note));
    menu.addItem(1, "Assign sample...");
    menu.addItem(2, "Play", s.assigned);
    menu.addItem(3, "Clear", s.assigned || s.missing);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(),
                       [this, note](int result) {
                           if (result == 1) chooseSample(note);
                           if (result == 2) processor.auditionDrum(note);
                           if (result == 3) requestClear(note);
                       });
}

void DrumKitPanel::chooseSample(int note)
{
    select(note, false);
    const auto& s = slots[static_cast<size_t>(note)];
    const auto start = s.file != juce::File() ? s.file.getParentDirectory() : juce::File();
    chooser = std::make_unique<juce::FileChooser>("Assign sample to " + noteLabel(note), start, kAudioWildcard);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this, note](const juce::FileChooser& fc) {
                             if (const auto file = fc.getResult(); file != juce::File())
                                 assign(note, file);
                         });
}

void DrumKitPanel::assign(int note, const juce::File& file)
{
    const auto result = processor.assignDrumSample(note, file);
    if (result.wasOk()) {
        reloadSlots();
        select(note, true);
        showMessage("Assigned " + file.getFileName() + " to " + noteLabel(note), false);
    } else {
        showMessage(file.getFileName() + ": " + result.getErrorMessage(), true);
    }
}

void DrumKitPanel::showMessage(const juce::String& text, bool error)
{
    message = text;
    messageIsError = error;
    messageFrames = 60 * 5;
    repaint(detailArea());
}

bool DrumKitPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (isAudioFile(f))
            return true;
    return false;
}

void DrumKitPanel::fileDragMove(const juce::StringArray&, int x, int y)
{
    const int note = noteAt({ x, y });
    if (note != dropNote) {
        dropNote = note;
        repaint(gridArea().expanded(2));
    }
}

void DrumKitPanel::fileDragExit(const juce::StringArray&)
{
    dropNote = -1;
    repaint(gridArea().expanded(2));
}

void DrumKitPanel::filesDropped(const juce::StringArray& files, int x, int y)
{
    dropNote = -1;
    int note = noteAt({ x, y });
    if (note < 0) {
        repaint();
        return;
    }
    // 複数ファイルは、落としたノートから順に割り当てる
    const int first = note;
    int assigned = 0;
    juce::String lastError;
    for (const auto& path : files) {
        if (!isAudioFile(path) || note > 127)
            continue;
        const auto result = processor.assignDrumSample(note, juce::File(path));
        if (result.wasOk())
            ++assigned;
        else
            lastError = juce::File(path).getFileName() + ": " + result.getErrorMessage();
        ++note;
    }
    reloadSlots();
    select(first, assigned == 1);
    if (lastError.isNotEmpty())
        showMessage(lastError, true);
    else
        showMessage("Assigned " + juce::String(assigned) + " sample(s) from " + noteLabel(first), false);
}

} // namespace ui
