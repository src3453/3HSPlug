#include "DrumKit.h"
#include "DrumPcmSampleLoader.h"

namespace {
juce::String sizeText(uint64_t bytes)
{
    return juce::String(bytes / 1024.0, 1) + " KB";
}
}

DrumKit::Decoded DrumKit::decode(const juce::File& file)
{
    Decoded result;
    if (!file.existsAsFile()) {
        result.error = "File not found";
        return result;
    }
    const auto decoded = DrumPcmSampleLoader::loadAndDecode(file.getFullPathName().toStdString());
    if (decoded.pcm.empty() || decoded.sampleRate == 0) {
        result.error = "Unsupported or empty audio file";
        return result;
    }
    result.data = DrumPcmSampleLoader::convertToRaw8bit(decoded.pcm);
    result.sampleRate = decoded.sampleRate;
    return result;
}

void DrumKit::repack()
{
    uint32_t address = 0;
    for (auto& s : slots) {
        if (!s.isAssigned()) {
            s.start = 0;
            continue;
        }
        s.start = address;
        std::copy(s.data.begin(), s.data.end(), pcm.begin() + address);
        address += static_cast<uint32_t>(s.data.size());
    }
    std::fill(pcm.begin() + address, pcm.begin() + juce::jmax(address, used), uint8_t { 0x80 });
    used = address;
}

void DrumKit::loadDefaults(const juce::File& pcmDirectory)
{
    defaultDirectory = pcmDirectory;
    for (auto& s : slots)
        s = Slot {};
    uint32_t total = 0;
    for (int note = 0; note < kNumNotes; ++note) {
        const auto file = pcmDirectory.getChildFile(juce::String(note) + ".wav");
        if (!file.existsAsFile())
            continue;
        auto decoded = decode(file);
        auto& s = slots[static_cast<size_t>(note)];
        s.file = file;
        s.fromDefault = true;
        if (decoded.data.empty()) {
            s.missing = true;
            s.error = decoded.error;
            continue;
        }
        if (total + decoded.data.size() > kPcmMemorySize) {
            s.missing = true;
            s.error = "Not enough PCM memory";
            continue;
        }
        total += static_cast<uint32_t>(decoded.data.size());
        s.data = std::move(decoded.data);
        s.sampleRate = decoded.sampleRate;
    }
    repack();
}

void DrumKit::resetToDefaults()
{
    loadDefaults(defaultDirectory);
}

void DrumKit::setMissing(int note, const juce::File& file, const juce::String& error)
{
    auto& s = slots[static_cast<size_t>(note)];
    s = Slot {};
    s.file = file;
    s.missing = true;
    s.error = error;
    s.fromDefault = file == defaultDirectory.getChildFile(juce::String(note) + ".wav");
    repack();
}

juce::Result DrumKit::assign(int note, const juce::File& file)
{
    if (note < 0 || note >= kNumNotes)
        return juce::Result::fail("Invalid note");
    auto decoded = decode(file);
    if (decoded.data.empty())
        return juce::Result::fail(decoded.error);

    const auto& current = slots[static_cast<size_t>(note)];
    const uint64_t needed = static_cast<uint64_t>(used) - current.data.size() + decoded.data.size();
    if (needed > kPcmMemorySize)
        return juce::Result::fail("Not enough PCM memory: needs " + sizeText(decoded.data.size()) + ", "
                                  + sizeText(kPcmMemorySize - (used - current.data.size())) + " free");

    auto& s = slots[static_cast<size_t>(note)];
    s = Slot {};
    s.file = file;
    s.fromDefault = file == defaultDirectory.getChildFile(juce::String(note) + ".wav");
    s.data = std::move(decoded.data);
    s.sampleRate = decoded.sampleRate;
    repack();
    return juce::Result::ok();
}

void DrumKit::clear(int note)
{
    if (note < 0 || note >= kNumNotes)
        return;
    slots[static_cast<size_t>(note)] = Slot {};
    repack();
}

bool DrumKit::isDefaultAssignment(int note) const
{
    const auto defaultFile = defaultDirectory.getChildFile(juce::String(note) + ".wav");
    const auto& s = slot(note);
    if (!defaultFile.existsAsFile())
        return s.file == juce::File() && !s.missing;
    return s.file == defaultFile;
}

void DrumKit::applyToKeymap(DrumKeymapManager& keymap, uint8_t midiChannel) const
{
    keymap.clear();
    for (int note = 0; note < kNumNotes; ++note) {
        const auto& s = slots[static_cast<size_t>(note)];
        if (s.isAssigned())
            keymap.assignNoteToSample(midiChannel, static_cast<uint8_t>(note), static_cast<int32_t>(s.start),
                                      static_cast<int32_t>(s.sampleRate), static_cast<int32_t>(s.data.size()));
    }
}

juce::ValueTree DrumKit::toState() const
{
    juce::ValueTree state("DrumKit");
    for (int note = 0; note < kNumNotes; ++note) {
        if (isDefaultAssignment(note))
            continue;
        juce::ValueTree slotTree("Slot");
        slotTree.setProperty("note", note, nullptr);
        slotTree.setProperty("file", slot(note).file.getFullPathName(), nullptr); // 空 = 割り当てなし
        state.appendChild(slotTree, nullptr);
    }
    return state;
}

void DrumKit::fromState(const juce::ValueTree& state)
{
    resetToDefaults();
    for (const auto& slotTree : state) {
        const int note = slotTree.getProperty("note", -1);
        if (note < 0 || note >= kNumNotes)
            continue;
        const juce::String path = slotTree.getProperty("file", juce::String());
        if (path.isEmpty()) {
            clear(note);
            continue;
        }
        const juce::File file(path);
        if (auto result = assign(note, file); result.failed())
            setMissing(note, file, result.getErrorMessage());
    }
}

juce::Result DrumKit::saveKitFile(const juce::File& file) const
{
    const auto base = file.getParentDirectory();
    juce::Array<juce::var> drums;
    for (int note = 0; note < kNumNotes; ++note) {
        const auto& s = slot(note);
        if (s.file == juce::File())
            continue;
        auto* entry = new juce::DynamicObject();
        entry->setProperty("note", note);
        entry->setProperty("file", s.file.isAChildOf(base) ? s.file.getRelativePathFrom(base).replaceCharacter('\\', '/')
                                                          : s.file.getFullPathName());
        drums.add(juce::var(entry));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty("version", 1);
    root->setProperty("drums", drums);
    if (!file.replaceWithText(juce::JSON::toString(juce::var(root))))
        return juce::Result::fail("Could not write " + file.getFullPathName());
    return juce::Result::ok();
}

juce::Result DrumKit::loadKitFile(const juce::File& file)
{
    const auto json = juce::JSON::parse(file.loadFileAsString());
    const auto* drums = json.getProperty("drums", juce::var()).getArray();
    if (drums == nullptr)
        return juce::Result::fail("Not a drum kit file (no \"drums\" array)");

    for (auto& s : slots)
        s = Slot {};
    repack();
    const auto base = file.getParentDirectory();
    int failed = 0;
    for (const auto& entry : *drums) {
        const int note = entry.getProperty("note", -1);
        const juce::String path = entry.getProperty("file", juce::String());
        if (note < 0 || note >= kNumNotes || path.isEmpty())
            continue;
        const auto sample = juce::File::isAbsolutePath(path) ? juce::File(path) : base.getChildFile(path);
        if (auto result = assign(note, sample); result.failed()) {
            setMissing(note, sample, result.getErrorMessage());
            ++failed;
        }
    }
    if (failed > 0)
        return juce::Result::fail(juce::String(failed) + " sample(s) could not be loaded");
    return juce::Result::ok();
}
