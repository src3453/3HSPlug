// Patchバンク外部定義例
#pragma once
#include <array>
#include <cstdint>
#include <string>

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
    std::array<uint8_t, 64> toRegValues(uint8_t midiVolume = 255);
};

// 例: 128個のパッチ定義（必要に応じて内容を編集）
extern std::array<Patch, PATCH_BANK_SIZE> PatchBank;
extern Patch defaultPatch; // デフォルトパッチ
extern bool volumeScalingMap[13][8]; // volumeScalingMapをexternで宣言
void initializePatchBank(const std::string& patchJsonPath = "patch_bank.json");
bool loadPatchBankFromYAML(const std::string& filePath);
bool loadPatchBankFromJSON(const std::string& filePath);
bool savePatchBankToYAML(const std::string& filePath);
bool savePatchBankToJSON(const std::string& filePath);
void exportCurrentPatchBankToJSON(); // 一時的な関数: 現在のPatchBankをJSONに書き出す
Patch& getPatchOrDefault(int programNumber);