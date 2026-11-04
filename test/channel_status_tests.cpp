#include "PluginEditor.h"
#include "ui/UIScale.h"
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
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
        check(tabs != nullptr && tabs->getNumTabs() == 2, "Editor tabs");
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
                auto image = editor.createComponentSnapshot(editor.getLocalBounds());
                auto output = directory.getChildFile("channels-" + juce::String(size.x) + ".png").createOutputStream();
                check(output != nullptr && juce::PNGImageFormat().writeImageToStream(image, *output), "Screenshot export");
            }
        }
        processor.releaseResources();
        std::cout << "Channel status tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
