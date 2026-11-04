// Patchバンク外部定義例
#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include "VolumeScalingMap.h"

#define PATCH_BANK_SIZE 128
#define MAX_BANKS 128  // 最大バンク数（GSバンク対応）

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

struct Mutation {
    float attackTime = 0.0f;
    float decayTime = 0.0f;
    float sustainLevel = 0.0f;
    float releaseTime = 0.0f;
    float LPFCutoff = 0.0f;
    float LPFResonance = 0.0f;
    float CarrierFreqShift = 0.0f; // 1/4096単位で周波数倍率に加算する
    float ModulatorFreqShift = 0.0f; // 1/4096単位で周波数倍率に加算する
};

// パッチ（MIDIプログラム番号ごとのレジスタ設定）構造体
struct Patch {
    // 意味のあるパラメータ
    bool defined = false; // パッチが定義されているか
    uint8_t modmode = 0; // モジュレーションモード
    uint8_t feedback = 128; // フィードバック (デフォルトは128)
    uint8_t fbDest = 0; // フィードバック先 (0: OP1 ... 7: OP8)
    int8_t keyShift = 0; // キーシフト (デフォルトは0)
    std::array<Operator, 8> operators; // 各オペレーターのパラメータ
    std::array<uint8_t, 64> regValues{}; // 既存コード互換用

    // ...他に必要なパラメータを追加

    // レジスタ値配列へ変換
    std::array<uint8_t, 64> toRegValues(uint8_t midiVolume = 255);
    Patch applyMutation(const Mutation& mutation);
    bool volumeScalingNeeded(int operatorIndex) const {
        if (modmode < 13) {
            return volumeScalingMap[modmode][operatorIndex];
        }
        return false; // デフォルトはスケーリングなし
    }
};

bool patchesEqual(const Patch& a, const Patch& b); // 意味のあるパラメータだけを比較する

// バンクごとのパッチ定義（GSバンク対応）
// プラグインインスタンスごとに1つ持つ（_3HSPlugAudioProcessorが所有する）。
// プロセス全体で共有すると、同じDAW内の複数インスタンスが編集やステートを上書きし合う。
class PatchBankSet {
public:
    using Bank = std::vector<Patch>;
    using Banks = std::vector<Bank>;

    PatchBankSet();

    // banks:         発音に使う現在の内容（SysExオーバーライドを含む）
    // originalBanks: GMリセットで戻す基準（JSONの内容＋UIでの編集）
    // fileBanks:     JSONファイルから読み込んだままの内容（UIの「元に戻す」の基準）
    Banks banks;
    Banks originalBanks;
    Banks fileBanks;
    Patch defaultPatch; // デフォルトパッチ

    void initialize(const std::string& patchesDir = "patches/");
    bool loadFromJSON(const std::string& filePath, int bankNumber = 0);
    bool saveBankToJSON(const std::string& filePath, int bankNumber = 0) const;
    static bool saveBankToJSON(const std::string& filePath, const Bank& bank);

    // UIでの編集（GMリセット後も残る）
    void setUserPatch(int bankNumber, int programNumber, const Patch& patch);
    void revertUserPatch(int bankNumber, int programNumber); // JSONファイルの内容に戻す
    bool isUserPatchModified(int bankNumber, int programNumber) const; // JSONファイルの内容と異なるか
    bool saveUserPatchBank(const std::string& filePath, int bankNumber); // 基準の内容を保存し、ファイル側の基準を更新する
    void restoreFromFiles(); // 現在の内容とGMリセットの基準をJSONファイルの内容に戻す（UIでの編集も消える）
    void clearUserPatch(int bankNumber, int programNumber); // 現在の内容と基準の両方で未定義にする
    void exportCurrentToJSON() const; // 一時的な関数: 現在のPatchBankをJSONに書き出す
    Patch& getPatchOrDefault(int bankNumber, int programNumber);
    void setPatchOverride(int bankNumber, int patchNumber, int relativeAddr, int value);
    void reset(); // GMリセット: SysExオーバーライドを取り消す

    // 代理発音関連（バンク0へのフォールバック）
    Patch& getEffectivePatch(int bankNumber, int programNumber);
    const Patch& getEffectivePatch(int bankNumber, int programNumber) const;
    int getPatchAvailability(int bankNumber, int programNumber) const;

private:
    void initDefaultPatch();
};
