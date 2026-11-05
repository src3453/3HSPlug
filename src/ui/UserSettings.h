// このPCのユーザー設定（%APPDATA%\3HSPlug\3HSPlug-ui.settings など）。UIのフォントやテーマを入れる。
// DAWのプロジェクトではなく使う人の好みなので、プラグインの状態には入れない。
#pragma once

#include <JuceHeader.h>

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

inline juce::PropertiesFile& userSettings()
{
    static juce::PropertiesFile file(userSettingsOptions());
    return file;
}

// Debugタブを表示するか
inline bool showDebugTab() { return userSettings().getBoolValue("showDebugTab", false); }
inline void setShowDebugTab(bool show)
{
    userSettings().setValue("showDebugTab", show);
    userSettings().saveIfNeeded();
}

// ユーザーのテーマファイル（*.json）を置くフォルダ
inline juce::File userThemesFolder()
{
    return userSettingsOptions().getDefaultFile().getParentDirectory().getChildFile("themes");
}

} // namespace ui
