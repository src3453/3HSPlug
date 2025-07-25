// DrumKeymapManager.cpp
#include "DrumKeymapManager.h"
    
void DrumKeymapManager::assignNoteToSample(uint8_t midiChannel, uint8_t noteNumber, int32_t pcmIndex, int32_t sampleRate, int32_t pcmLength) {
    keymapTable[midiChannel][noteNumber] = DrumSampleInfo{pcmIndex, sampleRate, pcmLength};
    printf("[DrumKeymapManager] Assigned note %d on channel %d to PCM index 0x%X, sample rate %d, length %d\n", noteNumber, midiChannel, pcmIndex, sampleRate, pcmLength);
}

DrumSampleInfo DrumKeymapManager::getSampleInfo(uint8_t midiChannel, uint8_t noteNumber) const {
    auto chIt = keymapTable.find(midiChannel);
    if (chIt == keymapTable.end()) return DrumSampleInfo{-1, -1, -1};
    auto noteIt = chIt->second.find(noteNumber);
    if (noteIt == chIt->second.end()) return DrumSampleInfo{-1, -1, -1};
    return noteIt->second;
}

// ドラムサンプル再生トリガー
void DrumKeymapManager::triggerDrumSample(uint8_t midiChannel, uint8_t noteNumber, uint8_t velocity) {
    DrumSampleInfo info = getSampleInfo(midiChannel, noteNumber);
    if (info.pcmIndex >= 0) {
        // 実際のPCM再生処理をここに記述（例: DrumPcmSampleLoader等を呼び出し）
        printf("[DrumKeymapManager] CH%d Note%d → PCM idx=0x%X rate=%d len=%d vel=%d\n",
            midiChannel, noteNumber, info.pcmIndex, info.sampleRate, info.pcmLength, velocity);
        // TODO: DrumPcmSampleLoader等の再生関数呼び出し
    } else {
        printf("[DrumKeymapManager] CH%d Note%d 未割当\n", midiChannel, noteNumber);
    }
}

void DrumKeymapManager::initializeGM() {
    // GMドラムノート番号の例（35:Acoustic Bass Drum, 36: Bass Drum 1, ...）
    // 必要に応じて拡張
    // 自動割り当てに変更
}