// DrumPcmSampleLoader.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct DecodedWav {
    std::vector<float> pcm;
    uint32_t sampleRate;
};

class DrumPcmSampleLoader {
public:
    // PCM RAMにRaw Unsigned 8bitで書き込む
    bool loadSampleToRam(const std::string& filePath, uint32_t ramAddress, uint32_t& outSampleRate, uint32_t& outPcmSize);

    // 任意のPCMデータをRaw Unsigned 8bitに変換
    static std::vector<uint8_t> convertToRaw8bit(const std::vector<float>& input);

    // サポート: WAV/AIFF/16bit PCM等からのロード
    static DecodedWav loadAndDecode(const std::string& filePath);
};

// 一括ロード関数の宣言を追加
void loadAllDrumSamples(class DrumKeymapManager& keymap, uint32_t baseRamAddr = 0);