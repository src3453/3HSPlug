// Patchバンク外部定義例
#pragma once
#include <array>
#include <cstdint>

#define PATCH_BANK_SIZE 128



// オペレーター 構造体
struct Operator {
    uint16_t frequency = 0; // OP1では使用しない
    uint8_t attack = 0;
    uint8_t decay = 0;
    uint8_t sustain = 0;
    uint8_t release = 0;
    uint8_t volume = 0;
    uint8_t waveform = 0; // 4bit 
};

// パッチ（MIDIプログラム番号ごとのレジスタ設定）構造体
struct Patch {
    // 意味のあるパラメータ
    bool defined = false; // パッチが定義されているか
    uint8_t modmode = 0; // モジュレーションモード
    uint8_t feedback = 128; // フィードバック (デフォルトは128)
    int8_t keyShift = 0; // キーシフト (デフォルトは0)
    std::array<Operator, 8> operators; // 各オペレーターのパラメータ
    std::array<uint8_t, 64> regValues{}; // 既存コード互換用

    // ...他に必要なパラメータを追加

    // レジスタ値配列へ変換
    std::array<uint8_t, 64> toRegValues() {
        std::array<uint8_t, 64> regs{0};
        regs[0x1F] = feedback; // フィードバック
        regs[0x1C] = modmode; // モジュレーションモード
        for (size_t i = 0; i < 8; ++i) {
            const Operator& op = operators[i];
            // すべてのOPでfrequency/volumeを反映
            regs[i * 2 + 0] = op.frequency >> 8; // 周波数倍率 上位ビット
            regs[i * 2 + 1] = op.frequency & 0xFF; // 周波数倍率 下位ビット
            regs[0x10 + i] = op.volume; // 音量
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
};

// 例: 128個のパッチ定義（必要に応じて内容を編集）
extern std::array<Patch, PATCH_BANK_SIZE> PatchBank;
extern Patch defaultPatch; // デフォルトパッチ
void initializePatchBank();
const Patch& getPatchOrDefault(int programNumber);