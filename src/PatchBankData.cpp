

#include "PatchBankData.h"

/*
PatchBankの初期化と、プログラム番号に該当しない場合のフォールバック取得
パッチ番号はMIDIプログラム番号によって決定される。GM準拠。
*/

std::array<Patch, PATCH_BANK_SIZE> PatchBank;
Patch defaultPatch; // デフォルトパッチ

void initializePatchBank() {
    // 0: Acoustic Grand Piano
    PatchBank[0].defined = true;
    PatchBank[0].modmode = 4;
    PatchBank[0].feedback = 0x83;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[0].operators[0] = { 0, 0, 64, 0, 16, 255, 0 };
    PatchBank[0].operators[1] = { 0x1000, 0, 10, 125, 16, 16, 2 };

    // 1: Bright Acoustic Piano
    PatchBank[1].defined = true;
    PatchBank[1].modmode = 4;
    PatchBank[1].feedback = 0x86;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[1].operators[0] = { 0, 0, 64, 0, 16, 255, 0 };
    PatchBank[1].operators[1] = { 0x1000, 0, 10, 125, 16, 32, 2 };

    // 8: Celesta
    PatchBank[8].defined = true;
    PatchBank[8].modmode = 4;
    PatchBank[8].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[8].operators[0] = { 0, 0, 64, 0, 32, 255, 0 };
    PatchBank[8].operators[1] = { 0x5000, 0, 4, 64, 32, 8, 0 };

    // 11: Vibraphone
    PatchBank[11].defined = true;
    PatchBank[11].modmode = 4;
    PatchBank[11].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[11].operators[0] = { 0, 0, 64, 0, 1, 255, 0 };
    PatchBank[11].operators[1] = { 0x9000, 0, 64, 255, 1, 6, 0 };

    // 12: Malimba
    PatchBank[12].defined = true;
    PatchBank[12].modmode = 4;
    PatchBank[12].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[12].operators[0] = { 0, 0, 64, 0, 1, 255, 0 };
    PatchBank[12].operators[1] = { 0x7000, 0, 64, 255, 1, 4, 0 };

    // 13: Xylophone
    PatchBank[13].defined = true;
    PatchBank[13].modmode = 4;
    PatchBank[13].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[13].operators[0] = { 0, 0, 64, 0, 32, 255, 0 };
    PatchBank[13].operators[1] = { 0x5000, 0, 4, 64, 32, 12, 0 };

    // 24: Nylon Acoustic Guitar
    PatchBank[24].defined = true;
    PatchBank[24].modmode = 4;
    PatchBank[24].feedback = 0x80;
    PatchBank[24].keyShift = 0;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[24].operators[0] = { 0, 0, 128, 0, 1, 255, 0 };
    PatchBank[24].operators[1] = { 0x1000, 0, 0, 255, 1, 8, 0 };
    PatchBank[24].operators[2] = { 0x3000, 0, 64, 0, 1, 32, 1 };

    // 25: Steel Acoustic Guitar
    PatchBank[25].defined = true;
    PatchBank[25].modmode = 4;
    PatchBank[25].feedback = 0x80;
    PatchBank[25].keyShift = 0;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[25].operators[0] = { 0, 0, 128, 0, 1, 255, 0 };
    PatchBank[25].operators[1] = { 0x1000, 0, 0, 255, 1, 8, 0 };
    PatchBank[25].operators[2] = { 0x3000, 0, 64, 0, 1, 48, 1 };

    // 36: Slap Bass 1
    PatchBank[36].defined = true;
    PatchBank[36].modmode = 4;
    PatchBank[36].feedback = 0x80;
    PatchBank[36].keyShift = 12;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[36].operators[0] = { 0, 0, 96, 0, 4, 255, 0 };
    PatchBank[36].operators[1] = { 0x0800, 0, 64, 0, 4, 46, 0 };
    PatchBank[36].operators[2] = { 0x9000, 0, 8, 128, 4, 3, 1 };

    // 37: Slap Bass 2
    PatchBank[37].defined = true;
    PatchBank[37].modmode = 4;
    PatchBank[37].feedback = 0x80;
    PatchBank[37].keyShift = 12;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[37].operators[0] = { 0, 0, 96, 0, 4, 255, 0 };
    PatchBank[37].operators[1] = { 0x0800, 0, 64, 0, 4, 46, 0 };
    PatchBank[37].operators[2] = { 0x9000, 0, 8, 128, 4, 3, 1 };

    // 79: Ocarina
    PatchBank[79].defined = true;
    PatchBank[79].modmode = 0;
    PatchBank[79].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[79].operators[0] = { 0, 0, 16, 192, 1, 255, 0 };
    PatchBank[79].operators[1] = { 0x2000, 0, 0, 255, 1, 16, 0 };

    // 80: Square Wave
    PatchBank[80].defined = true;
    PatchBank[80].modmode = 0;
    PatchBank[80].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[80].operators[0] = { 0, 0, 1, 144, 0, 255, 3 };

    // 81: Saw Wave
    PatchBank[81].defined = true;
    PatchBank[81].modmode = 0;
    PatchBank[81].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[81].operators[0] = { 0, 0, 1, 144, 0, 255, 5 };

    // 必要に応じて他のパッチも追加


    // デフォルトパッチを初期化
    defaultPatch.defined = false; // デフォルトは未定義扱い
    defaultPatch.modmode = 4;
    defaultPatch.feedback = 0x80;
    defaultPatch.keyShift = 0;
    // frequency, attack, decay, sustain, release, volume, waveform
    defaultPatch.operators[0] = { 0, 0, 64, 0, 1, 255, 3 };
    defaultPatch.operators[1] = { 0x1000, 0, 0, 255, 255, 17, 6 };
}



// プログラム番号に該当しない場合は0番パッチを返す
const Patch& getPatchOrDefault(int programNumber) {
    //printf("getPatchOrDefault: programNumber=%d, defined=%d\n", programNumber, PatchBank[programNumber].defined);
    if (programNumber >= 0 && programNumber < PATCH_BANK_SIZE && PatchBank[programNumber].defined) { // 定義済みのパッチがある場合
        return PatchBank[programNumber]; // 定義済みのパッチを返す
    }
    //printf("[Warning::Patch] Patch %d not defined, returning default patch 0\n", programNumber);
    return defaultPatch; // デフォルトは0番パッチ
}