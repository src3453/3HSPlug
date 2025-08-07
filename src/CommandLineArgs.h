// CommandLineArgs.h
#pragma once
#include <string>
#include <vector>
#include <map>

/**
 * コマンドライン引数解析クラス
 * PCMサンプルパスとパッチJSONパスを指定可能にする
 */
class CommandLineArgs {
public:
    CommandLineArgs();
    
    // コマンドライン引数を解析
    bool parse(int argc, char* argv[]);
    
    // PCMサンプルディレクトリパスを取得（デフォルト: "./pcm/"）
    std::string getPcmPath() const { return pcmPath; }
    
    // パッチJSONファイルパスを取得（デフォルト: "patch_bank.json"）
    std::string getPatchJsonPath() const { return patchJsonPath; }
    
    // ヘルプメッセージを表示
    void showHelp() const;
    
    // 引数が正常に解析されたかチェック
    bool isValid() const { return valid; }
    
private:
    std::string pcmPath;
    std::string patchJsonPath;
    bool valid;
    
    // 引数名とデフォルト値のマップ
    std::map<std::string, std::string> defaultValues;
    
    // 引数の説明
    std::map<std::string, std::string> argDescriptions;
};