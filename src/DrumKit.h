// ドラムキット: ノートごとのPCMサンプル割り当てと、PCMメモリへの配置
// プラグインインスタンスごとに1つ持つ（_3HSPlugAudioProcessorが所有する）。
#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <vector>
#include "DrumKeymapManager.h"

class DrumKit
{
public:
    static constexpr uint32_t kPcmMemorySize = 0x400000; // 3HS88PWN4のPCMメモリ（4 MB）
    static constexpr int kNumNotes = 128;

    struct Slot {
        juce::File file;             // 割り当てたファイル（未割り当てなら空）
        bool fromDefault = false;    // PCMフォルダの「ノート番号.wav」から来たもの
        bool missing = false;        // ファイルが見つからない/読めない/入りきらない
        juce::String error;          // missingの理由
        std::vector<uint8_t> data;   // 8bit unsigned PCM
        uint32_t sampleRate = 0;
        uint32_t start = 0;          // PCMメモリ上の開始アドレス

        bool isAssigned() const noexcept { return !data.empty(); }
    };

    // PCMフォルダの「ノート番号.wav」をすべて読み込み、キットを置き換える
    void loadDefaults(const juce::File& pcmDirectory);
    void resetToDefaults();

    juce::Result assign(int note, const juce::File& file);
    void clear(int note);

    const Slot& slot(int note) const { return slots[static_cast<size_t>(juce::jlimit(0, kNumNotes - 1, note))]; }
    bool isDefaultAssignment(int note) const;
    uint32_t usedBytes() const noexcept { return used; }
    const std::vector<uint8_t>& memory() const noexcept { return pcm; }
    const juce::File& getDefaultDirectory() const noexcept { return defaultDirectory; }

    void applyToKeymap(DrumKeymapManager& keymap, uint8_t midiChannel = 10) const;

    // DAWの状態: PCMフォルダの既定の割り当てと異なるノートだけを保存する
    juce::ValueTree toState() const;
    void fromState(const juce::ValueTree& state);

    // キットファイル（JSON）。キットファイルのフォルダ内のサンプルは相対パスで書く。
    juce::Result saveKitFile(const juce::File& file) const;
    juce::Result loadKitFile(const juce::File& file);

private:
    struct Decoded { std::vector<uint8_t> data; uint32_t sampleRate = 0; juce::String error; };
    static Decoded decode(const juce::File& file);
    void setMissing(int note, const juce::File& file, const juce::String& error);
    void repack();

    std::array<Slot, kNumNotes> slots;
    juce::File defaultDirectory;
    std::vector<uint8_t> pcm = std::vector<uint8_t>(kPcmMemorySize, 0x80);
    uint32_t used = 0;
};
