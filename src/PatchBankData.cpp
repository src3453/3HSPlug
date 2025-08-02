

#include "PatchBankData.h"

#define y true
#define _ false
// モジュレーションモードに応じて、音量に応じてボリュームレジスタをスケーリングすべきかを定義する (true: スケーリングする, false: しない)
// 可読性のために、y: true, _: false としている
bool volumeScalingMap[13][8] = {
    {y,y,y,y,y,y,y,y}, //  0: Additive
    {y,y,y,y,_,_,_,_}, //  1: 4x2OP FM
    {y,y,y,y,_,_,_,_}, //  2: 4x2 RingMod
    {y,y,_,_,_,_,_,_}, //  3: 2x4OP FM
    {y,_,_,_,_,_,_,_}, //  4: 8OP FM
    {y,y,_,_,y,y,_,_}, //  5: 4OP FM x2
    {y,_,y,_,y,_,y,_}, //  6: 2OP FM x4
    {y,_,_,_,_,_,_,_}, //  7: 4OP FMxRM x2
    {y,y,_,_,_,_,_,_}, //  8: 2x4 RingMod
    {y,_,_,_,_,_,_,_}, //  9: 2OP FMxRM x4
    {y,y,y,y,_,_,_,_}, // 10: 2OP DirectPhase
    {y,y,_,_,_,_,_,_}, // 11: 4OP DirectPhase
    {y,_,_,_,_,_,_,_}, // 12: 8OP DirectPhase
};
#undef y // 一時的なマクロ定義を削除
#undef _ // 一時的なマクロ定義を削除

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

    // 10: Music Box
    PatchBank[10].defined = true;
    PatchBank[10].modmode = 1;
    PatchBank[10].feedback = 0x80;
    PatchBank[10].keyShift = 0;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[10].operators[0] = { 0x0000, 0, 64, 0, 0, 255, 0 }; 
    PatchBank[10].operators[1] = { 0x4000, 0, 50, 0, 4, 128, 0 };
    PatchBank[10].operators[2] = { 0x1000, 0,  2, 0, 4,  32, 0 };
    PatchBank[10].operators[4] = { 0x8000, 0, 32, 0, 0,   4, 0 };


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

    // 38: Synth Bass 1
    PatchBank[38].defined = true;
    PatchBank[38].modmode = 4;
    PatchBank[38].feedback = 0x88;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[38].operators[0] = { 0, 0, 64, 64, 2, 255, 0 };

    // 39: Synth Bass 2
    PatchBank[39].defined = true;
    PatchBank[39].modmode = 4;
    PatchBank[39].feedback = 0x88;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[39].operators[0] = { 0, 0, 64, 64, 2, 255, 0 };

    // 46: Orchestral Harp
    PatchBank[46].defined = true;
    PatchBank[46].modmode = 4;
    PatchBank[46].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[46].operators[0] = { 0, 0, 64, 0, 32, 255, 0 };
    PatchBank[46].operators[1] = { 0x1010, 0, 0, 255, 255, 16, 0 };
    PatchBank[46].operators[2] = { 0x0FF3, 0, 0, 255, 255, 16, 0 };

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

    // 87: Lead 8 (bass + lead)
    PatchBank[87].defined = true;
    PatchBank[87].modmode = 4;
    PatchBank[87].feedback = 0x88;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[87].operators[0] = { 0, 0, 16, 128, 4, 255, 0 };

    // 100: FX 5 (brightness)
    PatchBank[100].defined = true;
    PatchBank[100].modmode = 4;
    PatchBank[100].feedback = 0x84;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[100].operators[0] = { 0, 1, 64, 64, 4, 255, 0 };
    PatchBank[100].operators[1] = { 0x0ff1, 1, 64, 64, 4, 16, 15 };
    PatchBank[100].operators[2] = { 0x1012, 1, 64, 64, 4, 16, 15 };

    // 必要に応じて他のパッチも追加


    // デフォルトパッチを初期化
    defaultPatch.defined = false; // デフォルトは未定義扱い
    defaultPatch.modmode = 4;
    defaultPatch.feedback = 0x80;
    defaultPatch.keyShift = 0;
    // frequency, attack, decay, sustain, release, volume, waveform
    defaultPatch.operators[0] = { 0,      0, 64, 1, 32, 255, 3 };
    defaultPatch.operators[1] = { 0x1000, 0, 0, 255, 255,  17, 6 };
    //defaultPatch.operators[2] = { 0x0FF3, 0, 0, 255, 255, 16, 0 };
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

// PatchクラスのtoRegValuesメソッドの実装
std::array<uint8_t, 64> Patch::toRegValues(uint8_t midiVolume) {
    std::array<uint8_t, 64> regs{0};
    regs[0x1F] = feedback; // フィードバック
    regs[0x1C] = modmode; // モジュレーションモード
    for (size_t i = 0; i < 8; ++i) {
        const Operator& op = operators[i];
        // すべてのOPでfrequency/volumeを反映
        regs[i * 2 + 0] = op.frequency >> 8; // 周波数倍率 上位ビット
        regs[i * 2 + 1] = op.frequency & 0xFF; // 周波数倍率 下位ビット
        // volumeScalingMapに応じて音量をスケーリング
        uint8_t finalVolume = op.volume;
        if (modmode < 13 && volumeScalingMap[modmode][i]) {
            // スケーリングが必要な場合、MIDIボリュームで調整
            finalVolume = static_cast<uint8_t>((static_cast<uint16_t>(op.volume) * midiVolume) / 255);
        }
        regs[0x10 + i] = finalVolume; // 音量
        regs[0x20 + i * 4 + 0] = op.attack; // Attack
        regs[0x20 + i * 4 + 1] = op.decay; // Decay
        regs[0x20 + i * 4 + 2] = op.sustain; // Sustain
        regs[0x20 + i * 4 + 3] = op.release; // Release
        // 波形は4bitなので、オペレーター番号に応じてMSB, LSBを切り替える (奇数番目はMSB, 偶数番目はLSB)
        if (i % 2 == 0) {
            regs[0x18 + i / 2] = (op.waveform & 0x0F) << 4; // MSBを設定
        } else {
            regs[0x18 + i / 2] |= (op.waveform & 0x0F); // LSBを設定
        }
    }
    // ...他のパラメータも必要に応じてマッピング
    regValues = regs; // メンバにも保存
    //printf("Patch toRegValues: ");
    //for (int i = 0; i < 64; ++i) {
    //    printf("%02X ", regValues[i]);
    //}
    //printf("\n");
    return regs;
}