// DrumKeymapManager.h
#pragma once
#include <unordered_map>
#include <vector>
#include <cstdint>

struct DrumSampleInfo {
    int32_t pcmIndex;
    int32_t sampleRate;
    int32_t pcmLength;
};

class DrumKeymapManager {
public:
    // ドラムチャンネルごとにノート→PCM情報を管理
    void assignNoteToSample(uint8_t midiChannel, uint8_t noteNumber, int32_t pcmIndex, int32_t sampleRate, int32_t pcmLength);
    DrumSampleInfo getSampleInfo(uint8_t midiChannel, uint8_t noteNumber) const;
    // ドラムサンプル再生トリガー
    void triggerDrumSample(uint8_t midiChannel, uint8_t noteNumber, uint8_t velocity);

    // GMドラム初期化
    void initializeGM();

private:
    // [midiChannel][noteNumber] = DrumSampleInfo
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, DrumSampleInfo>> keymapTable;
};