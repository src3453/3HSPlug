// DrumPcmSampleLoader.cpp
#include "DrumPcmSampleLoader.h"
#include "DrumKeymapManager.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

extern uint8_t* g_pcmRam;
extern size_t g_pcmRamSize;

namespace fs = std::filesystem;

DecodedWav DrumPcmSampleLoader::loadAndDecode(const std::string& filePath) {
    // JUCEのAudioFormatManagerを使ってWAV/AIFF/FLAC/MP3等を読み込む
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    juce::File juceFile(filePath);
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(juceFile));
    if (!reader) return {};

    std::vector<float> pcm;
    int64_t numSamples = reader->lengthInSamples;
    int numChannels = reader->numChannels;
    pcm.reserve(static_cast<size_t>(numSamples));

    juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(numSamples));
    reader->read(&buffer, 0, static_cast<int>(numSamples), 0, true, true);

    // モノラル化（複数chの場合は平均）
    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            sum += buffer.getSample(ch, i);
        }
        pcm.push_back(sum / numChannels);
    }

    uint32_t sampleRate = static_cast<uint32_t>(reader->sampleRate);
    return DecodedWav{pcm, sampleRate};
}

std::vector<uint8_t> DrumPcmSampleLoader::convertToRaw8bit(const std::vector<float>& input) {
    std::vector<uint8_t> out;
    out.reserve(input.size());
    for (float v : input) {
        // -1.0～1.0 → 0～255 にスケール
        float clamped = std::min(std::max(v, -1.0f), 1.0f);
        uint8_t val = static_cast<uint8_t>((clamped + 1.0f) * 127.5f); // -1→0, 0→127, 1→255
        out.push_back(val);
    }
    return out;
}

bool DrumPcmSampleLoader::loadSampleToRam(const std::string& filePath, uint32_t ramAddress, uint32_t& outSampleRate, uint32_t& outPcmSize) {
    auto decoded = loadAndDecode(filePath);
    if (decoded.pcm.empty()) return false;
    auto raw8 = convertToRaw8bit(decoded.pcm);
    if (!g_pcmRam || ramAddress + raw8.size() > g_pcmRamSize) return false;
    std::memcpy(g_pcmRam + ramAddress, raw8.data(), raw8.size());
    outSampleRate = decoded.sampleRate;
    outPcmSize = static_cast<uint32_t>(raw8.size());
    return true;
}

// 一括ロード: 指定ディレクトリ内の[ノート番号].wavを全てロードし、キーマップに登録
void loadAllDrumSamples(DrumKeymapManager& keymap, uint32_t baseRamAddr, const std::string& pcmPath) {
    uint32_t ramPtr = baseRamAddr;
    printf("[DrumPcmSampleLoader] Loading drum samples from: %s\n", pcmPath.c_str());
    
    for (int note = 0; note < 128; ++note) {
        std::string wavPath = pcmPath + std::to_string(note) + ".wav";
        if (!fs::exists(wavPath)) {
            continue;
        }

        DrumPcmSampleLoader loader;
        uint32_t sampleRate = 44100;
        uint32_t pcmSize = 0;
        if (loader.loadSampleToRam(wavPath, ramPtr, sampleRate, pcmSize)) {
            keymap.assignNoteToSample(10, note, ramPtr, sampleRate, pcmSize); // MIDI ch10
            ramPtr += pcmSize; // PCMデータ長分だけポインタを進める
            printf("[DrumPcmSampleLoader] Loaded note %d from %s\n", note, wavPath.c_str());
        }
    }
    printf("[DrumPcmSampleLoader] Loaded all drum samples from %s, %d bytes used (%d bytes free, %f%%)\n", pcmPath.c_str(), ramPtr - baseRamAddr, g_pcmRamSize - (ramPtr - baseRamAddr), (ramPtr - baseRamAddr) * 100.0f / g_pcmRamSize);
}