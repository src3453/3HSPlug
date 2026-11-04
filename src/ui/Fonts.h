// UIのフォント選択（システム標準か、同梱の AzurZulu / future6x8_shrink）
//
// 選んだフォントはこのPCのユーザー設定（3HSPlug-ui.settings）に保存し、どのインスタンスでも同じものを使う。
// フォントはDAWのプロジェクトではなく、使う人の好みなのでプラグインの状態には入れない。
#pragma once

#include <JuceHeader.h>

namespace ui {

enum class FontChoice { system = 0, azurZulu = 1, future6x8 = 2 };

juce::StringArray fontChoiceNames();          // メニューに出す名前（FontChoice の順）
FontChoice currentFontChoice();
void setFontChoice(FontChoice choice);         // 切り替えて設定ファイルに保存する
juce::Typeface::Ptr customTypeface();          // システム標準のときは nullptr

// 選んだフォントでUI全体を描き直す（ラベルのフォントを作り直し、LookAndFeel の変更を伝える）
void refreshFonts(juce::Component& root);

} // namespace ui
