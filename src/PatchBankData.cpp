

#include "PatchBankData.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <juce_core/juce_core.h>

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

std::array<std::array<Patch, PATCH_BANK_SIZE>, MAX_BANKS> PatchBanks;
std::array<std::array<Patch, PATCH_BANK_SIZE>, MAX_BANKS> PatchBanksOriginal;

Patch defaultPatch; // デフォルトパッチ

void initializePatchBanks(const std::string& patchesDir) {
    printf("[PatchBankData] Initializing patch banks from directory: %s\n", patchesDir.c_str());

    // パッチ番号は実際のMIDIプログラム番号から1を引いた値, 1-128 -> 0-127


    // 必要に応じて他のパッチも追加


    // デフォルトパッチを初期化
    defaultPatch.defined = false; // デフォルトは未定義扱い
    defaultPatch.modmode = 4;
    defaultPatch.feedback = 0x80;
    defaultPatch.keyShift = 0;
    // frequency, attack, decay, sustain, release, volume, waveform
    defaultPatch.operators[0] = { 0,      0, 64,  1,  32, 255, 3 };
    defaultPatch.operators[1] = { 0x1000, 0,  0,255, 255,  17, 6 };
    //defaultPatch.operators[2] = { 0x0FF3, 0, 0, 255, 255, 16, 0 };

    // すべてのバンクを初期化
    for (int bank = 0; bank < MAX_BANKS; ++bank) {
        for (auto& patch : PatchBanks[bank]) {
            patch.defined = false;
        }
    }
    
    // パッチファイルをフォルダから読み込み
    for (int bank = 0; bank < MAX_BANKS; ++bank) {
        std::string bankFilePath = patchesDir + std::to_string(bank) + ".json";
        loadPatchBankFromJSON(bankFilePath, bank);
    }
    
    PatchBanksOriginal = PatchBanks; // オリジナルのパッチバンクを保存
}

// 一時的な関数: 現在のPatchBankの内容をJSONファイルに書き出す
void exportCurrentPatchBankToJSON() {
    const std::string outputPath = "current_patch_bank.json";
    if (savePatchBankToJSON(outputPath, 0)) {
        std::cout << "[PatchLoaderJSON] Current PatchBank exported to " << outputPath << std::endl;
    } else {
        std::cerr << "[PatchLoaderJSON] Failed to export current PatchBank to " << outputPath << std::endl;
    }
}

void resetPatchBanks() {
    PatchBanks = PatchBanksOriginal;
}

// プログラム番号に該当しない場合は0番パッチを返す
Patch& getPatchOrDefault(int bankNumber, int programNumber) {
    if (bankNumber >= 0 && bankNumber < MAX_BANKS &&
        programNumber >= 0 && programNumber < PATCH_BANK_SIZE &&
        PatchBanks[bankNumber][programNumber].defined) {
        return PatchBanks[bankNumber][programNumber];
    }
    return defaultPatch;
}

// 代理発音機能：指定バンクにパッチがない場合はバンク0にフォールバック
Patch& getEffectivePatch(int bankNumber, int programNumber) {
    if (bankNumber >= 0 && bankNumber < MAX_BANKS &&
        programNumber >= 0 && programNumber < PATCH_BANK_SIZE) {
        
        // 指定バンクにパッチが定義されている場合
        if (PatchBanks[bankNumber][programNumber].defined) {
            return PatchBanks[bankNumber][programNumber];
        }
        
        // 指定バンクにパッチがない場合、バンク0（GM）にフォールバック
        if (bankNumber != 0 && PatchBanks[0][programNumber].defined) {
            //printf("[PatchBankData] Fallback to Bank 0 for program %d\n", programNumber);
            return PatchBanks[0][programNumber];
        }
    }
    
    // どちらにも定義がない場合はデフォルトパッチ
    return defaultPatch;
}

void setPatchOverride(int bankNumber, int patchNumber, int relativeAddr, int value) {
    if (bankNumber >= 0 && bankNumber < MAX_BANKS &&
        patchNumber >= 0 && patchNumber < PATCH_BANK_SIZE) {
        Patch& patch = PatchBanks[bankNumber][patchNumber];
        // オーバーライド処理
        if (relativeAddr >= 0 && relativeAddr < 0x41) {
            if (relativeAddr >= 0x02 && relativeAddr <= 0x0F)
            {
                if ((relativeAddr & 1) == 0) {
                    patch.operators[relativeAddr/2].frequency &= 0x00FF;
                    patch.operators[relativeAddr/2].frequency |= (value << 8);
                } else// OP2-8 frequency MSB
                {
                    patch.operators[relativeAddr/2].frequency &= 0xFF00;
                    patch.operators[relativeAddr/2].frequency |= value;
                }// OP2-8 frequency LSB
            }
            if (relativeAddr >= 0x10 && relativeAddr <= 0x17)
            {
                // 音量レジスタのオーバーライド
                patch.operators[relativeAddr - 0x10].volume = value;
            }
            if (relativeAddr >= 0x18 && relativeAddr <= 0x1B)
            {
                patch.operators[(relativeAddr - 0x18)*2].waveform = (value & 0xF0) >> 4; // 波形は4bitなので下位4ビットのみを設定
                patch.operators[(relativeAddr - 0x18)*2 + 1].waveform = value & 0x0F; // MSBを設定
            }
            if (relativeAddr == 0x1C) {
                    patch.modmode = value;
            } 
            if (relativeAddr == 0x1F) {
                    patch.feedback = value;
            }
            if (relativeAddr >= 0x20 && relativeAddr <= 0x3F ) {
                int modulo = relativeAddr % 4;
                switch (modulo)
                {
                case 0:
                    patch.operators[(relativeAddr - 0x20)/4].attack = value;
                    break;
                case 1:
                    patch.operators[(relativeAddr - 0x20)/4].decay = value;
                    break;
                case 2:
                    patch.operators[(relativeAddr - 0x20)/4].sustain = value;
                    break;
                case 3:
                    patch.operators[(relativeAddr - 0x20)/4].release = value;
                    break;
                default:
                    break;
                }
            }
            if (relativeAddr == 0x40) {
                patch.keyShift = static_cast<int8_t>(value); // キーシフトはint8_tなのでキャスト
            }
        }
    }
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


// JSONからPatchBankを読み込む
bool loadPatchBankFromJSON(const std::string& filePath, int bankNumber) {
    try {
        // ファイルを読み込む
        
        juce::File file(filePath);
        if (!file.exists()) {
            std::cerr << "[PatchLoaderJSON] Error: JSON file does not exist: " << filePath << std::endl;
            return false;
        }
        juce::String jsonContent = file.loadFileAsString();
        juce::var jsonData = juce::JSON::parse(jsonContent);
        
        if (!jsonData.isObject()) {
            std::cerr << "[PatchLoaderJSON] Error: Invalid JSON format" << std::endl;
            return false;
        }
        
        juce::var patches = jsonData.getProperty("patches", juce::var());
        if (!patches.isArray()) {
            std::cerr << "[PatchLoaderJSON] Error: JSON file does not contain 'patches' array" << std::endl;
            return false;
        }
        
        // 指定バンクの全てのパッチを未定義状態にリセット
        if (bankNumber >= 0 && bankNumber < MAX_BANKS) {
            for (auto& patch : PatchBanks[bankNumber]) {
                patch.defined = false;
            }
        } else {
            printf("[PatchLoaderJSON] Warning: Invalid bank number %d\n", bankNumber);
            return false;
        }
        
        // パッチデータを読み込む
        juce::Array<juce::var>* patchArray = patches.getArray();
        for (int i = 0; i < patchArray->size(); ++i) {
            juce::var patchData = patchArray->getReference(i);
            
            if (!patchData.isObject()) {
                continue;
            }
            
            int programNumber = static_cast<int>(patchData.getProperty("program", 0));
            if (programNumber < 0 || programNumber >= PATCH_BANK_SIZE) {
                std::cerr << "[PatchLoaderJSON] Warning: Invalid program number " << programNumber << std::endl;
                continue;
            }
            
            Patch& patch = PatchBanks[bankNumber][programNumber];
            
            // パッチの基本情報
            patch.defined = true;
            patch.modmode = static_cast<uint8_t>(static_cast<int>(patchData.getProperty("modmode", 4)));
            patch.feedback = static_cast<uint8_t>(static_cast<int>(patchData.getProperty("feedback", 0x80)));
            patch.keyShift = static_cast<int8_t>(static_cast<int>(patchData.getProperty("keyShift", 0)));
            
            // オペレーターデータ
            juce::var operators = patchData.getProperty("operators", juce::var());
            if (operators.isArray()) {
                juce::Array<juce::var>* operatorArray = operators.getArray();
                for (int j = 0; j < 8 && j < operatorArray->size(); ++j) {
                    juce::var opData = operatorArray->getReference(j);
                    if (opData.isObject()) {
                        patch.operators[j].frequency = static_cast<uint16_t>(static_cast<int>(opData.getProperty("frequency", 0)));
                        patch.operators[j].attack = static_cast<uint8_t>(static_cast<int>(opData.getProperty("attack", 0)));
                        patch.operators[j].decay = static_cast<uint8_t>(static_cast<int>(opData.getProperty("decay", 0)));
                        patch.operators[j].sustain = static_cast<uint8_t>(static_cast<int>(opData.getProperty("sustain", 0)));
                        patch.operators[j].release = static_cast<uint8_t>(static_cast<int>(opData.getProperty("release", 0)));
                        patch.operators[j].volume = static_cast<uint8_t>(static_cast<int>(opData.getProperty("volume", 0)));
                        patch.operators[j].waveform = static_cast<uint8_t>(static_cast<int>(opData.getProperty("waveform", 0)));
                    }
                }
                printf("[PatchLoaderJSON] Loaded patch %d (has %d operator(s)).\n", programNumber, operatorArray->size());
            }
            
        }
        
        printf("[PatchLoaderJSON] Successfully loaded %d patches into bank %d from %s\n", patchArray->size(), bankNumber, filePath.c_str());
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PatchLoaderJSON] Error loading patch bank from JSON: " << e.what() << std::endl;
        return false;
    }
}

// PatchBankをJSONに保存する
bool savePatchBankToJSON(const std::string& filePath, int bankNumber) {
    try {
        std::ostringstream json;
        json << "{\n  \"patches\": [\n";
        
        bool first = true;
        for (int i = 0; i < PATCH_BANK_SIZE; ++i) {
            const Patch& patch = PatchBanks[bankNumber][i];
            if (!patch.defined) {
                continue;
            }
            
            if (!first) {
                json << ",\n";
            }
            first = false;
            
            json << "    {\n";
            json << "      \"program\": " << i << ",\n";
            json << "      \"modmode\": " << static_cast<int>(patch.modmode) << ",\n";
            json << "      \"feedback\": " << static_cast<int>(patch.feedback) << ",\n";
            json << "      \"keyShift\": " << static_cast<int>(patch.keyShift) << ",\n";
            json << "      \"operators\": [\n";
            
            for (size_t j = 0; j < 8; ++j) {
                const Operator& op = patch.operators[j];
                json << "        {\n";
                json << "          \"frequency\": " << static_cast<int>(op.frequency) << ",\n";
                json << "          \"attack\": " << static_cast<int>(op.attack) << ",\n";
                json << "          \"decay\": " << static_cast<int>(op.decay) << ",\n";
                json << "          \"sustain\": " << static_cast<int>(op.sustain) << ",\n";
                json << "          \"release\": " << static_cast<int>(op.release) << ",\n";
                json << "          \"volume\": " << static_cast<int>(op.volume) << ",\n";
                json << "          \"waveform\": " << static_cast<int>(op.waveform) << "\n";
                json << "        }";
                if (j < 7) json << ",";
                json << "\n";
            }
            
            json << "      ]\n";
            json << "    }";
        }
        
        json << "\n  ]\n}";
        
        std::ofstream fout(filePath);
        fout << json.str();
        fout.close();
        
        std::cout << "[PatchLoaderJSON] Successfully saved bank " << bankNumber << " to " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PatchLoaderJSON] Error saving patch bank: " << e.what() << std::endl;
        return false;
    }
}
