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
    // 任意のPCMデータをRaw Unsigned 8bitに変換
    static std::vector<uint8_t> convertToRaw8bit(const std::vector<float>& input);

    // サポート: WAV/AIFF/16bit PCM等からのロード
    static DecodedWav loadAndDecode(const std::string& filePath);
};
