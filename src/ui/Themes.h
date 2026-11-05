// UIのテーマ: 配色（colours::）と見た目の形（shape::）をまとめたもの
//
// 同梱のテーマのほかに、ユーザーのテーマファイル（%APPDATA%\3HSPlug\themes\*.json）を読み込む。
// ファイルは "base" で同梱テーマを指定し、変えたい色や形だけを書けばよい:
//   { "name": "Mint", "base": "Midnight",
//     "colours": { "accent": "#3ce0b0", "focusRing": "#3ce0b0" },
//     "shape": { "roundness": 0.4, "knob": "pointer", "button": "outline" } }
// 選んだテーマはこのPCのユーザー設定に保存し、開いているすべての 3HSPlug の画面に反映する。
#pragma once

#include <JuceHeader.h>

namespace ui {

struct ThemeData {
    juce::String name;
    juce::File file;           // ユーザーのテーマファイル（同梱テーマは空）
    juce::String loadError;    // 読めなかったときの理由

    juce::Colour background, panel, panelRaised, outline, fieldEdge, text, textDim, accent, accentDim, onAccent,
        focusRing, carrier, modulator, meterGreen, meterYellow, meterRed, meterRms, scrim, lcdBack, lcdOn, lcdOff,
        keyWhite, keyBlack;

    float roundness = 1.0f;
    int knob = 0;              // shape::Knob
    int button = 0;            // shape::Button
    bool sectionBorders = true;
    bool titleGradient = true;
    bool monochromeChannels = false;
    float channelSaturation = 0.55f;
    float channelBrightness = 0.95f;
};

// 同梱テーマのあとにユーザーのテーマが続く。ユーザーのテーマは呼ぶたびにフォルダから読み直す。
juce::Array<ThemeData> availableThemes();
float contrastRatio(juce::Colour a, juce::Colour b); // WCAG のコントラスト比
juce::String currentThemeName();
void selectTheme(const juce::String& name);   // 適用して保存し、開いている画面に知らせる
void applySavedTheme();                        // 起動時: 保存されているテーマを適用する（見つからなければ既定）

// テーマとフォントの変更の知らせ（開いているエディタがそれぞれ自分を描き直す）
juce::ChangeBroadcaster& appearanceChanged();
void refreshTheme(juce::Component& root);      // LookAndFeel の色を入れ直して描き直す

// ユーザーのテーマフォルダを作り、まだ無ければ現在のテーマを書き出した見本を置く。フォルダを返す。
juce::File prepareUserThemesFolder();

} // namespace ui
