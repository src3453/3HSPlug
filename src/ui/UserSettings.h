// このPCのユーザー設定（%APPDATA%\3HSPlug\3HSPlug-ui.settings など）。UIのフォントやテーマを入れる。
// DAWのプロジェクトではなく使う人の好みなので、プラグインの状態には入れない。
#pragma once

#include <JuceHeader.h>
#include "../DebugLog.h"

namespace ui {

inline juce::PropertiesFile::Options userSettingsOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "3HSPlug-ui"; // Standalone の 3HSPlug.settings とは別のファイルにする
    options.folderName = "3HSPlug";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

// PropertiesFile は juce::Timer を継承していて、破棄時に TimerThread の終了を無期限に待つことがある。
// 関数内 static だと DLL の終了処理(ExitProcess の後)で破棄され、OS に殺されたスレッドを待ち続けて
// プロセスが終わらなくなる(ビルド中の juce_vst3_helper が固まる)。
// DeletedAtShutdown なら、スレッドが生きている JUCE の終了処理の中で破棄される。
class UserSettingsHolder : private juce::DeletedAtShutdown
{
public:
    UserSettingsHolder() : file(userSettingsOptions()) {}
    ~UserSettingsHolder() override { clearSingletonInstance(); }

    juce::PropertiesFile file;

    JUCE_DECLARE_SINGLETON_INLINE(UserSettingsHolder, false)
};

inline juce::PropertiesFile& userSettings()
{
    return UserSettingsHolder::getInstance()->file;
}

// Debugタブを表示するか
inline bool showDebugTab() { return userSettings().getBoolValue("showDebugTab", false); }
inline void setShowDebugTab(bool show)
{
    userSettings().setValue("showDebugTab", show);
    userSettings().saveIfNeeded();
}

// ログをコンソール(stderr)にも出力するか。既定OFF。OFFでもDebugタブのログ表示には溜まる
inline bool consoleLogEnabled() { return userSettings().getBoolValue("consoleLog", false); }
inline void applyConsoleLogSetting() { dbglog::Log::instance().setConsoleEnabled(consoleLogEnabled()); }
inline void setConsoleLogEnabled(bool on)
{
    userSettings().setValue("consoleLog", on);
    userSettings().saveIfNeeded();
    applyConsoleLogSetting();
}

// ユーザーのテーマファイル（*.json）を置くフォルダ
inline juce::File userThemesFolder()
{
    return userSettingsOptions().getDefaultFile().getParentDirectory().getChildFile("themes");
}

} // namespace ui
