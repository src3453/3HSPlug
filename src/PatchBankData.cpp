

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

void initializePatchBank() {

    // パッチ番号は実際のMIDIプログラム番号から1を引いた値, 1-128 -> 0-127
/*
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

    // 9: Glockenspiel
    PatchBank[9].defined = true;
    PatchBank[9].modmode = 4;
    PatchBank[9].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[9].operators[0] = { 0, 0, 64, 0, 1, 255, 0 };
    PatchBank[9].operators[1] = { 0x5010, 0, 64, 255, 1, 10, 2 };

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
    PatchBank[11].operators[1] = { 0x5000, 0, 64, 255, 1, 6, 0 };

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

    // 30: Distortion Guitar	
    PatchBank[30].defined = true;
    PatchBank[30].modmode = 4;
    PatchBank[30].feedback = 0x80;
    PatchBank[30].keyShift = 12;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[30].operators[0] = { 0, 0, 255, 0, 16, 255, 2 };
    PatchBank[30].operators[1] = { 0x0800, 0, 255, 0, 16, 64, 2 };

    // 32: Acoustic Bass
    PatchBank[32].defined = true;
    PatchBank[32].modmode = 4;
    PatchBank[32].feedback = 0x80;
    PatchBank[32].keyShift = 12;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[32].operators[0] = { 0, 0, 96, 0, 4, 255, 0 };
    PatchBank[32].operators[1] = { 0x0800, 0, 64, 0, 4, 46, 0 };

    // 33: Electric Bass (finger)
    PatchBank[33].defined = true;
    PatchBank[33].modmode = 4;
    PatchBank[33].feedback = 0x80;
    PatchBank[33].keyShift = 12;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[33].operators[0] = { 0, 0, 96, 0, 4, 255, 0 };
    PatchBank[33].operators[1] = { 0x0800, 0, 64, 0, 4, 46, 0 };

    // 34: Electric Bass (pick)
    PatchBank[34].defined = true;
    PatchBank[34].modmode = 4;
    PatchBank[34].feedback = 0x80;
    PatchBank[34].keyShift = 12;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[34].operators[0] = { 0, 0, 96, 0, 4, 255, 0 };
    PatchBank[34].operators[1] = { 0x0800, 0, 64, 0, 4, 46, 0 };
    PatchBank[34].operators[2] = { 0x1000, 0, 8, 128, 4, 3, 1 };

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

    // 55: Ohchestra Hit
    PatchBank[55].defined = true;
    PatchBank[55].modmode = 4;
    PatchBank[55].feedback = 0x84;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[55].operators[0] = { 0, 1, 64, 0, 4, 255, 0 };
    PatchBank[55].operators[1] = { 0x0ff1, 1, 64, 0, 4, 16, 15 };
    PatchBank[55].operators[2] = { 0x1012, 1, 64, 0, 4, 16, 15 };

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
    PatchBank[80].operators[0] = { 0, 0, 1, 144, 0, 128, 3 };
    //PatchBank[80].operators[1] = { 0x1020, 0, 1, 144, 0, 128, 3 };

    // 81: Saw Wave
    PatchBank[81].defined = true;
    PatchBank[81].modmode = 0;
    PatchBank[81].feedback = 0x80;
    // frequency, attack, decay, sustain, release, volume, waveform
    PatchBank[81].operators[0] = { 0, 0, 1, 144, 0, 255, 5 };
    //PatchBank[81].operators[1] = { 0x1020, 0, 1, 144, 0, 128, 5 };

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

    // FX 1-8: temporarily copy FX 5 to other slots
    PatchBank[96] = PatchBank[100]; 
    PatchBank[97] = PatchBank[100]; 
    PatchBank[98] = PatchBank[100]; 
    PatchBank[99] = PatchBank[100]; 
    PatchBank[101] = PatchBank[100]; 
    PatchBank[102] = PatchBank[100]; 
    PatchBank[103] = PatchBank[100]; 
*/

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
    loadPatchBankFromJSON("patch_bank.json"); // JSONからパッチバンクを読み込む
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