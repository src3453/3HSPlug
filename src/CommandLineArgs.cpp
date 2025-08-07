// CommandLineArgs.cpp
#include "CommandLineArgs.h"
#include <iostream>
#include <cstring>

CommandLineArgs::CommandLineArgs() : valid(false) {
    // デフォルト値を設定
    pcmPath = "./pcm/";
    patchJsonPath = "patch_bank.json";
    
    // デフォルト値マップを初期化
    defaultValues["--pcm-path"] = "./pcm/";
    defaultValues["--patch-json"] = "patch_bank.json";
    
    // 引数の説明を設定
    argDescriptions["--pcm-path"] = "PCMサンプルファイルが格納されているディレクトリパス";
    argDescriptions["--patch-json"] = "パッチバンクJSONファイルのパス";
    argDescriptions["--help"] = "このヘルプメッセージを表示";
}

bool CommandLineArgs::parse(int argc, char* argv[]) {
    valid = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // ヘルプオプション
        if (arg == "--help" || arg == "-h") {
            showHelp();
            valid = false;
            return false;
        }
        
        // PCMパスオプション
        if (arg == "--pcm-path") {
            if (i + 1 < argc) {
                pcmPath = argv[++i];
                // パスの末尾にスラッシュがない場合は追加
                if (!pcmPath.empty() && pcmPath.back() != '/' && pcmPath.back() != '\\') {
                    pcmPath += "/";
                }
                std::cout << "[CommandLineArgs] PCM path set to: " << pcmPath << std::endl;
            } else {
                std::cerr << "[CommandLineArgs] Error: --pcm-path requires a path argument" << std::endl;
                valid = false;
                return false;
            }
            continue;
        }
        
        // パッチJSONパスオプション
        if (arg == "--patch-json") {
            if (i + 1 < argc) {
                patchJsonPath = argv[++i];
                std::cout << "[CommandLineArgs] Patch JSON path set to: " << patchJsonPath << std::endl;
            } else {
                std::cerr << "[CommandLineArgs] Error: --patch-json requires a path argument" << std::endl;
                valid = false;
                return false;
            }
            continue;
        }
        
        // 不明な引数
        if (arg.substr(0, 2) == "--") {
            std::cerr << "[CommandLineArgs] Warning: Unknown argument: " << arg << std::endl;
        }
    }
    
    return valid;
}

void CommandLineArgs::showHelp() const {
    std::cout << "3HSPlug Audio Plugin - Command Line Arguments\n";
    std::cout << "============================================\n\n";
    std::cout << "Usage: 3HSPlug [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --pcm-path <path>     " << argDescriptions.at("--pcm-path") << "\n";
    std::cout << "                        デフォルト: " << defaultValues.at("--pcm-path") << "\n\n";
    std::cout << "  --patch-json <path>   " << argDescriptions.at("--patch-json") << "\n";
    std::cout << "                        デフォルト: " << defaultValues.at("--patch-json") << "\n\n";
    std::cout << "  --help, -h            " << argDescriptions.at("--help") << "\n\n";
    std::cout << "Examples:\n";
    std::cout << "  3HSPlug --pcm-path ./samples/ --patch-json ./config/patches.json\n";
    std::cout << "  3HSPlug --pcm-path C:/Audio/Samples/\n";
    std::cout << "  3HSPlug --help\n\n";
}