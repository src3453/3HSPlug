// DrumPcmSampleLoader.cpp
#include "DrumPcmSampleLoader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>



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
