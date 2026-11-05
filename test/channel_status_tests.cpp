#include "PluginEditor.h"
#include "ui/UIScale.h"
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

template <typename T>
T* findChildOfType(juce::Component& parent)
{
    for (auto* child : parent.getChildren()) {
        if (auto* found = dynamic_cast<T*>(child)) return found;
        if (auto* found = findChildOfType<T>(*child)) return found;
    }
    return nullptr;
}

juce::Label* findLabel(juce::Component& parent, const juce::String& text)
{
    for (auto* child : parent.getChildren()) {
        if (auto* label = dynamic_cast<juce::Label*>(child); label != nullptr && label->getText() == text) return label;
        if (auto* found = findLabel(*child, text)) return found;
    }
    return nullptr;
}

// 閉じたポップアップの後始末は非同期なので、メッセージループのないテストでは直接呼ぶ
void runModalCleanup()
{
    struct Access : juce::ModalComponentManager { using juce::ModalComponentManager::handleAsyncUpdate; };
    (juce::ModalComponentManager::getInstance()->*&Access::handleAsyncUpdate)();
}

void closePopups()
{
    juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
    runModalCleanup();
}

void writeSnapshot(juce::Component& component, const juce::File& file)
{
    file.deleteFile();
    auto image = component.createComponentSnapshot(component.getLocalBounds());
    auto output = file.createOutputStream();
    check(output != nullptr && juce::PNGImageFormat().writeImageToStream(image, *output), "Screenshot export");
}

// 同梱テーマごとに、すべてのタブ・ドラムキット・設定画面を書き出す（テーマからのずれを目で確かめる）
void writeThemeGallery(juce::Component& editor, juce::TabbedComponent& tabs, const juce::File& directory)
{
    directory.createDirectory();
    auto* content = editor.findChildWithID("mainUI");
    auto* settings = findChildOfType<ui::SettingsOverlay>(*content);
    check(settings != nullptr, "Settings overlay");

    juce::ToggleButton* debugToggle = nullptr;
    for (auto* child : settings->getChildren())
        if (auto* toggle = dynamic_cast<juce::ToggleButton*>(child); toggle != nullptr && toggle->getButtonText() == "Show Debug tab")
            debugToggle = toggle;
    check(debugToggle != nullptr, "Debug toggle");
    const bool debugWasShown = ui::showDebugTab();
    debugToggle->setToggleState(true, juce::dontSendNotification);
    debugToggle->onClick();

    const auto themeBefore = ui::currentThemeName();
    tabs.setCurrentTabIndex(0);
    auto* activity = findChildOfType<ui::ChannelActivityPanel>(*content);
    auto* patchEditor = findChildOfType<ui::PatchEditorPanel>(*content);
    check(activity != nullptr && patchEditor != nullptr, "Synth page");

    for (const auto& theme : ui::availableThemes()) {
        if (theme.file != juce::File()) continue;
        ui::selectTheme(theme.name);
        ui::appearanceChanged().dispatchPendingMessages();
        const auto slug = theme.name.toLowerCase().replaceCharacter(' ', '-');
        for (int i = 0; i < tabs.getNumTabs(); ++i) {
            tabs.setCurrentTabIndex(i);
            writeSnapshot(editor, directory.getChildFile(slug + "-" + tabs.getTabNames()[i].toLowerCase() + ".png"));
        }
        tabs.setCurrentTabIndex(0);
        activity->keyPressed(juce::KeyPress(juce::KeyPress::endKey)); // 一度端へ送り、ドラムのチャンネルへ戻す
        for (int i = 0; i < 6; ++i) activity->keyPressed(juce::KeyPress(juce::KeyPress::leftKey));
        patchEditor->refresh();
        writeSnapshot(editor, directory.getChildFile(slug + "-drums.png"));
        activity->keyPressed(juce::KeyPress(juce::KeyPress::homeKey));
        patchEditor->refresh();
        settings->open();
        writeSnapshot(editor, directory.getChildFile(slug + "-settings.png"));
        settings->close();

        ui::showConfirm(*content, "Revert patch 0:001?",
                        "Your edits to this patch are discarded and it goes back to the JSON file. "
                        "Notes that are sounding change at once.", "Revert", [] {});
        check(findChildOfType<juce::AlertWindow>(*content) != nullptr, "Dialog inside the scaled UI");
        writeSnapshot(editor, directory.getChildFile(slug + "-dialog.png"));
        closePopups();

        tabs.setCurrentTabIndex(2);
        juce::TextButton* optionsButton = nullptr;
        for (auto* child : tabs.getCurrentContentComponent()->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child); button != nullptr && button->getButtonText() == "Options")
                optionsButton = button;
        check(optionsButton != nullptr, "Scope options button");
        optionsButton->onClick();
        check(findChildOfType<juce::CallOutBox>(*content) != nullptr, "Options inside the scaled UI");
        writeSnapshot(editor, directory.getChildFile(slug + "-options.png"));
        closePopups();
        tabs.setCurrentTabIndex(0);
    }
    ui::selectTheme(themeBefore);
    ui::appearanceChanged().dispatchPendingMessages();
    debugToggle->setToggleState(debugWasShown, juce::dontSendNotification);
    debugToggle->onClick();
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    struct RestoreScale {
        int percent = ui::currentUIScalePercent();
        ~RestoreScale() { ui::setUIScalePercent(percent); ui::appearanceChanged().dispatchPendingMessages(); }
    } restoreScale;
    try {
        _3HSPlugAudioProcessor processor;
        processor.setPlayConfigDetails(0, 2, 48000, 64);
        processor.prepareToPlay(48000, 64);
        std::array<_3HSPlugAudioProcessor::ChannelStatus, 16> state;
        auto send = [&](juce::MidiMessage message) {
            juce::AudioBuffer<float> audio(2, 64);
            audio.clear();
            juce::MidiBuffer midi;
            midi.addEvent(message, 0);
            processor.processBlock(audio, midi);
            check(processor.readChannelStatus(state), "Snapshot failed");
        };
        send(juce::MidiMessage::controllerEvent(1, 7, 83));
        check(state[0].controllers[7] == 83 && state[1].controllers[7] == 127, "CC isolation");
        send(juce::MidiMessage::controllerEvent(1, 0, 8));
        send(juce::MidiMessage::controllerEvent(1, 32, 2));
        send(juce::MidiMessage::programChange(1, 40));
        send(juce::MidiMessage::pitchWheel(1, 4096));
        check(state[0].bank == 8 && state[0].bankLsb == 2 && state[0].program == 40 && state[0].pitchBend == -4096, "Program/bank/bend");
        send(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(99)));
        check(state[0].notes[60] == 99 && state[0].velocity == 99, "Note on");
        send(juce::MidiMessage::controllerEvent(1, 64, 127));
        send(juce::MidiMessage::noteOff(1, 60));
        check(state[0].notes[60] == 99, "Sustain hold");
        send(juce::MidiMessage::noteOn(1, 64, static_cast<juce::uint8>(80)));
        send(juce::MidiMessage::controllerEvent(1, 64, 0));
        check(state[0].notes[60] == 0 && state[0].notes[64] == 80, "Sustain release preserves pressed keys");
        send(juce::MidiMessage::noteOn(1, 64, static_cast<juce::uint8>(0)));
        check(state[0].notes[64] == 0, "Velocity-zero note off");
        send(juce::MidiMessage::noteOn(10, 36, static_cast<juce::uint8>(110)));
        check(state[9].drum && state[9].notes[36] == 110, "Drum note");
        send(juce::MidiMessage::controllerEvent(10, 120, 0));
        check(state[9].notes[36] == 0, "All sound off");
        send(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(90)));
        send(juce::MidiMessage::controllerEvent(1, 123, 0));
        check(state[0].notes[60] == 0, "All notes off");
        send(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(90)));
        processor.allNotesOff();
        check(processor.readChannelStatus(state) && state[0].notes[60] == 0, "Panic");
        send(juce::MidiMessage::noteOn(1, 64, static_cast<juce::uint8>(90)));
        send(juce::MidiMessage::midiMachineControlCommand(juce::MidiMessage::mmc_stop));
        processor.resetGM();
        check(processor.readChannelStatus(state) && state[0].notes[64] == 0 && state[0].bank == 0 && state[0].pitchBend == 0, "GM reset");

        _3HSPlugAudioProcessorEditor editor(processor);
        juce::TabbedComponent* tabs = nullptr;
        auto* content = editor.findChildWithID("mainUI");
        check(content != nullptr, "Scalable content");
        const auto logicalSize = content->getLocalBounds();
        for (int percent : ui::scalePercentages) {
            ui::setUIScalePercent(percent);
            ui::appearanceChanged().dispatchPendingMessages();
            const float scale = percent / 100.0f;
            check(std::abs(editor.getWidth() - juce::roundToInt(logicalSize.getWidth() * scale)) <= 1, "Scaled window width");
            check(std::abs(content->getWidth() - logicalSize.getWidth()) <= 1, "Stable logical layout");
            check(juce::approximatelyEqual(content->getTransform().getScaleFactor(), scale), "Content scale transform");
            check(editor.getConstrainer()->getMinimumWidth() == juce::roundToInt(1000 * scale), "Scaled resize limits");
        }
        {
            _3HSPlugAudioProcessorEditor reopened(processor);
            auto* reopenedContent = reopened.findChildWithID("mainUI");
            check(reopenedContent != nullptr && juce::approximatelyEqual(reopenedContent->getTransform().getScaleFactor(), 2.0f), "Scale on reopening");
        }
        ui::setUIScalePercent(100);
        ui::appearanceChanged().dispatchPendingMessages();
        for (auto* child : content->getChildren())
            if (auto* t = dynamic_cast<juce::TabbedComponent*>(child)) tabs = t;
        check(tabs != nullptr && tabs->getNumTabs() >= 3, "Editor tabs");
        {
            // 確認ダイアログ: 画面の中に開き、確定でだけ実行する
            for (const int result : { 0, 1 }) {
                bool confirmed = false;
                ui::showConfirm(*content, "Title", "Message", "Do it", [&confirmed] { confirmed = true; });
                auto* alert = findChildOfType<juce::AlertWindow>(*content);
                check(alert != nullptr && alert->isCurrentlyModal() && alert->getNumButtons() == 2, "Confirm dialog opens");
                alert->exitModalState(result);
                runModalCleanup();
                check(confirmed == (result == 1), "Confirm dialog result");
                check(findChildOfType<juce::AlertWindow>(*content) == nullptr, "Confirm dialog closes");
            }
        }
        {
            // 同梱テーマの文字と操作部品のコントラスト、ボイスのセルの上の文字
            const auto themeBefore = ui::currentThemeName();
            for (const auto& theme : ui::availableThemes()) {
                if (theme.file != juce::File()) continue;
                ui::selectTheme(theme.name);
                namespace c = ui::colours;
                for (const auto surface : { c::background, c::panel, c::panelRaised }) {
                    for (const auto ink : { c::text, c::textDim, c::accent, c::carrier, c::modulator, c::warning, c::error }) {
                        if (ui::contrastRatio(ink, surface) < 4.5f) std::cerr << theme.name << " ink " << ink.toDisplayString(false) << " on " << surface.toDisplayString(false) << '\n';
                        check(ui::contrastRatio(ink, surface) >= 4.5f, "Text contrast");
                    }
                    for (int ch = 0; ch < 16; ++ch)
                        check(ui::contrastRatio(c::forChannel(ch), surface) >= 4.5f, "Channel colour contrast");
                }
                check(ui::contrastRatio(c::fieldEdge, c::panel) >= 3.0f && ui::contrastRatio(c::focusRing, c::panel) >= 3.0f, "Control edge contrast");
                for (int ch = 0; ch < 16; ++ch) {
                    const auto cell = c::panelRaised.overlaidWith(c::forChannel(ch).withAlpha(ui::alpha::voiceFill));
                    if (ui::contrastRatio(c::text, cell) < 4.5f) std::cerr << theme.name << " voice cell ch " << ch + 1 << ' ' << ui::contrastRatio(c::text, cell) << '\n';
                    check(ui::contrastRatio(c::text, cell) >= 4.5f, "Voice cell text contrast");
                }
            }
            ui::selectTheme(themeBefore);
            ui::appearanceChanged().dispatchPendingMessages();
        }
        {
            // ノブの名前の略は 11px のまま、最小サイズのノブの幅に入る
            const auto fontBefore = ui::currentFontChoice();
            for (const auto choice : { ui::FontChoice::system, ui::FontChoice::azurZulu, ui::FontChoice::future6x8 }) {
                ui::setFontChoice(choice);
                for (const char* name : { "Coarse", "Fine", "Level", "Attack", "Decay", "Sustain", "Release" }) {
                    const int width = juce::GlyphArrangement::getStringWidthInt(ui::font(ui::type::caption, true), juce::String(name).substring(0, 3) + ".");
                    if (width > 46) std::cerr << static_cast<int>(choice) << ' ' << name << ' ' << width << '\n';
                    check(width <= 46, "Knob name fits");
                }
            }
            ui::setFontChoice(fontBefore);
            ui::appearanceChanged().dispatchPendingMessages();
        }
        {
            // テーマを変えても、ラベルと入力欄に前のテーマの色が残らない
            const auto themeBefore = ui::currentThemeName();
            ui::selectTheme("Daylight");
            ui::appearanceChanged().dispatchPendingMessages();
            auto* caption = findLabel(*content, "Bank");
            check(caption != nullptr && caption->findColour(juce::Label::textColourId) == ui::colours::textDim, "Label follows the theme");
            ui::selectTheme(themeBefore);
            ui::appearanceChanged().dispatchPendingMessages();
            check(caption->findColour(juce::Label::textColourId) == ui::colours::textDim, "Label follows the theme back");
        }
        tabs->setCurrentTabIndex(1);
        auto* panel = dynamic_cast<ui::ChannelStatusPanel*>(tabs->getCurrentContentComponent());
        check(panel != nullptr, "Channel panel");
        send(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(99)));
        send(juce::MidiMessage::noteOn(1, 64, static_cast<juce::uint8>(90)));
        send(juce::MidiMessage::noteOn(1, 67, static_cast<juce::uint8>(85)));
        panel->update();
        check(panel->keyPressed(juce::KeyPress(juce::KeyPress::downKey)), "Keyboard selection");
        panel->keyPressed(juce::KeyPress(juce::KeyPress::returnKey));
        check(tabs->getCurrentTabIndex() == 0, "Open selected editor");
        tabs->setCurrentTabIndex(1);
        if (argc > 1) {
            const juce::File directory = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            directory.createDirectory();
            for (auto size : { juce::Point<int>(1200, 920), juce::Point<int>(1000, 800) }) {
                editor.setSize(size.x, size.y);
                writeSnapshot(editor, directory.getChildFile("channels-" + juce::String(size.x) + ".png"));
            }
            editor.setSize(1000, 800);
            writeThemeGallery(editor, *tabs, directory.getChildFile("min-size"));
            editor.setSize(1200, 920);
            writeThemeGallery(editor, *tabs, directory);
            ui::setUIScalePercent(150);
            ui::appearanceChanged().dispatchPendingMessages();
            writeThemeGallery(editor, *tabs, directory.getChildFile("scale-150"));
        }
        processor.releaseResources();
        std::cout << "Channel status tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
