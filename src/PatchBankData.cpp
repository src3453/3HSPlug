

#include "PatchBankData.h"
#include <yaml-cpp/yaml.h>
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

std::array<Patch, PATCH_BANK_SIZE> PatchBank;
Patch defaultPatch; // デフォルトパッチ

void initializePatchBank(const std::string& patchJsonPath) {
    printf("[PatchBankData] Initializing patch bank with file: %s\n", patchJsonPath.c_str());

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

    //exportCurrentPatchBankToJSON(); // 初期化後に現在のPatchBankをJSONに書き出す
    //std::cout << "PatchBank initialized and exported to JSON." << std::endl;
    loadPatchBankFromJSON(patchJsonPath); // 指定されたパスからパッチバンクを読み込む
}

// 一時的な関数: 現在のPatchBankの内容をJSONファイルに書き出す
void exportCurrentPatchBankToJSON() {
    const std::string outputPath = "current_patch_bank.json";
    if (savePatchBankToJSON(outputPath)) {
        std::cout << "[PatchLoaderJSON] Current PatchBank exported to " << outputPath << std::endl;
    } else {
        std::cerr << "[PatchLoaderJSON] Failed to export current PatchBank to " << outputPath << std::endl;
    }
}


// プログラム番号に該当しない場合は0番パッチを返す
Patch& getPatchOrDefault(int programNumber) {
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

// YAMLからPatchBankを読み込む
bool loadPatchBankFromYAML(const std::string& filePath) {
    try {
        YAML::Node config = YAML::LoadFile(filePath);
        
        if (!config["patches"]) {
            std::cerr << "[PatchLoaderYAML] Error: YAML file does not contain 'patches' section" << std::endl;
            return false;
        }
        
        // まず全てのパッチを未定義状態にリセット
        for (auto& patch : PatchBank) {
            patch.defined = false;
        }
        
        const YAML::Node& patches = config["patches"];
        for (const auto& patchNode : patches) {
            if (!patchNode["program"]) {
                continue;
            }
            
            int programNumber = patchNode["program"].as<int>();
            if (programNumber < 0 || programNumber >= PATCH_BANK_SIZE) {
                std::cerr << "[PatchLoaderYAML] Warning: Invalid program number " << programNumber << std::endl;
                continue;
            }
            
            Patch& patch = PatchBank[programNumber];
            
            // パッチの基本情報
            patch.defined = true;
            patch.modmode = patchNode["modmode"].as<uint8_t>(4);
            patch.feedback = patchNode["feedback"].as<uint8_t>(0x80);
            patch.keyShift = patchNode["keyShift"].as<int8_t>(0);
            
            // オペレーターデータ
            if (patchNode["operators"]) {
                const YAML::Node& operators = patchNode["operators"];
                for (size_t i = 0; i < 8 && i < operators.size(); ++i) {
                    const YAML::Node& op = operators[i];
                    patch.operators[i].frequency = op["frequency"].as<uint16_t>(0);
                    patch.operators[i].attack = op["attack"].as<uint8_t>(0);
                    patch.operators[i].decay = op["decay"].as<uint8_t>(0);
                    patch.operators[i].sustain = op["sustain"].as<uint8_t>(0);
                    patch.operators[i].release = op["release"].as<uint8_t>(0);
                    patch.operators[i].volume = op["volume"].as<uint8_t>(0);
                    patch.operators[i].waveform = op["waveform"].as<uint8_t>(0);
                }
                printf("[PatchLoaderYAML] Loaded patch %d (has %d operator(s)).\n", programNumber, operators.size());
            }
        }

        printf("[PatchLoaderYAML] Successfully loaded %d patch bank(s) from %s\n", patches.size(), filePath.c_str());
        return true;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "[PatchLoaderYAML] YAML parsing error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[PatchLoaderYAML] Error loading patch bank: " << e.what() << std::endl;
        return false;
    }
}

// JSONからPatchBankを読み込む
bool loadPatchBankFromJSON(const std::string& filePath) {
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
        
        // まず全てのパッチを未定義状態にリセット
        for (auto& patch : PatchBank) {
            patch.defined = false;
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
            
            Patch& patch = PatchBank[programNumber];
            
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
        
        printf("[PatchLoaderJSON] Successfully loaded %d patch bank(s) from %s\n", patchArray->size(), filePath.c_str());
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PatchLoaderJSON] Error loading patch bank from JSON: " << e.what() << std::endl;
        return false;
    }
}

// PatchBankをJSONに保存する
bool savePatchBankToJSON(const std::string& filePath) {
    try {
        std::ostringstream json;
        json << "{\n  \"patches\": [\n";
        
        bool first = true;
        for (int i = 0; i < PATCH_BANK_SIZE; ++i) {
            const Patch& patch = PatchBank[i];
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
        
        std::cout << "[PatchLoaderJSON] Successfully saved patch bank to " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PatchLoaderJSON] Error saving patch bank: " << e.what() << std::endl;
        return false;
    }
}

// PatchBankをYAMLに保存する（既存の機能を維持）
bool savePatchBankToYAML(const std::string& filePath) {
    try {
        YAML::Node config;
        YAML::Node patches;
        
        for (int i = 0; i < PATCH_BANK_SIZE; ++i) {
            const Patch& patch = PatchBank[i];
            if (!patch.defined) {
                continue;
            }
            
            YAML::Node patchNode;
            patchNode["program"] = i;
            patchNode["modmode"] = static_cast<int>(patch.modmode);
            patchNode["feedback"] = static_cast<int>(patch.feedback);
            patchNode["keyShift"] = static_cast<int>(patch.keyShift);
            
            YAML::Node operators;
            for (size_t j = 0; j < 8; ++j) {
                const Operator& op = patch.operators[j];
                YAML::Node opNode;
                opNode["frequency"] = static_cast<int>(op.frequency);
                opNode["attack"] = static_cast<int>(op.attack);
                opNode["decay"] = static_cast<int>(op.decay);
                opNode["sustain"] = static_cast<int>(op.sustain);
                opNode["release"] = static_cast<int>(op.release);
                opNode["volume"] = static_cast<int>(op.volume);
                opNode["waveform"] = static_cast<int>(op.waveform);
                operators.push_back(opNode);
            }
            patchNode["operators"] = operators;
            patches.push_back(patchNode);
        }
        
        config["patches"] = patches;
        
        std::ofstream fout(filePath);
        fout << config;
        fout.close();

        std::cout << "[PatchLoaderYAML] Successfully saved patch bank to " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PatchLoaderYAML] Error saving patch bank: " << e.what() << std::endl;
        return false;
    }
}