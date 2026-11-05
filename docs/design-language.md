# 3HSPlug デザイン言語

UIを変えるときの基準。値の実体は [src/ui/Theme.h](../src/ui/Theme.h) にあり、この文書はその使い方を決める。
テーマファイルの書き方は [README_Themes.md](../README_Themes.md) を参照。

## 性格

3HSPlug の画面は **計器盤** として作る。音源チップの状態を読み、数値を合わせる道具である。

1. **読み取りが先。** 飾りより、値と状態が一目で読めることを優先する。
2. **面は平らで、階層は3段だけ。** 影やグラデーションで奥行きを作らない（例外はタイトルバーとメーターのみ）。
3. **アクセントは意味があるときだけ。** 選択・状態・主な操作に限る。飾りには使わない。
   唯一の例外はタイトルバーの下の線で、これはブランドの印として残す。
4. **色と形はテーマが決める。** 部品は役割を指定するだけで、具体的な色や角の丸みを持たない。
5. **どのテーマでも同じように働く。** 明るいテーマ、角ばったテーマ、枠線なしのテーマでも状態が見分けられること。

## 面の階層

| 段 | トークン | 使うところ |
| --- | --- | --- |
| 地 | `background` | ウィンドウとタブページの地 |
| 区画 | `panel` | `drawSection()` で描く区画、オーバーレイの面 |
| 部品 | `panelRaised` | ボタン、コンボボックス、入力欄、セル |
| 計器窓 | `background` | 区画の中にくぼませた表示。波形、エンベロープ、経路図、メーター、スコープ |

- タブページの地は `background`。`panel` にすると、`sectionBorders: false` のテーマで区画が消える。
- オーバーレイ（設定、Options、確認ダイアログ）は `panel` の面に 1px の `fieldEdge` の枠、角は `radius::overlay`。
  枠にアクセントは使わない。

## 色の役割

| 役割 | トークン | 決まり |
| --- | --- | --- |
| 文字 | `text` / `textDim` | 控えめな文字にアルファをかけない。`textDim` がいちばん薄い文字 |
| 区切り | `outline` | 飾りの線だけ。操作できる部品の縁には使わない |
| 操作部品の縁 | `fieldEdge` | ノブの目盛り、スクロールバーのつまみも同じ |
| 意味 | `accent` / `onAccent` | 選択・状態・主な操作。アクセントの面に載せる文字は `onAccent` |
| フォーカス | `focusRing` | キーボードフォーカスだけ。アクセントで代用しない |
| 状態 | `success` / `warning` / `error` | `meterGreen` / `meterYellow` / `meterRed` の別名。状態を示すコードではこの名前を使う |
| 信号の役割 | `carrier` / `modulator` | オペレーターの役割だけ |
| チャンネル | `forChannel()` | MIDIチャンネルの識別だけ。文字にも使えるよう、テーマの適用時にどの面でも 4.5:1 以上へ整える |
| 鍵盤 | `keyWhite` / `keyBlack` | 鍵盤の絵。テーマの明暗に関係なく白鍵は白鍵に見える色にする |
| LCD | `lcdBack` / `lcdOn` / `lcdOff` | GSのテキスト・ドットマトリクス表示だけ |

`juce::Colour(0x...)` と `juce::Colours::` を [Themes.cpp](../src/ui/Themes.cpp) の外に書かない
（透明を指定する `transparentBlack` と、影に使う黒は除く）。

## 状態

どの部品でも同じ見え方にする。`brighter()` / `darker()` は明るいテーマで効かないので使わない。

| 状態 | 表し方 |
| --- | --- |
| マウスを載せた | `state::hover(c)`（文字色を薄く重ねる）。クリックできるセルと行は 1px の `fieldEdge` の枠 |
| 押した | `state::down(c)` |
| 選択中（セル・行・列） | `drawSelection()`（アクセントの薄い面と 1.5px の枠） |
| オン（ボタン・タブ） | `accent` の面に `onAccent`。`button: outline` では枠と文字がアクセント |
| キーボードフォーカス | `drawFocusRing()`、2px、すぐに出す。ツールチップも待たずに出す |
| 入力中 | 1px の枠はそのままで `drawFocusRing()` を重ねる（マウスで入ったときも） |
| 使えない | 全体に `alpha::disabled` |
| 鳴った・変わった | `accent` を `alpha::flash` で重ね、徐々に消す |
| ドロップできる | `success` の 2px の枠 |
| 問題がある | `error` の文字と 1px の枠 |

- フォーカスリングは `FocusRingOverlay` が描く。グリッドは `setComponentID("ownFocusRing")` を指定し、選択中のセルに自分で描く。
- 値そのものを濃さで表す描画（メーター、ボイスの音量など）は、`alpha::` の対象外。
- 元に戻せない操作は `showConfirm()` で確認する。
- 領域が空のときは、何が入るのかを文字で示す。

## 形

角の丸みは必ず `shape::r(radius::...)` を通す。

| トークン | 値 | 使うところ |
| --- | --- | --- |
| `radius::fine` | 1 | メーターのセグメント |
| `radius::cell` | 2 | 小さなセル、細いトラック |
| `radius::data` | 3 | 計器窓、ドラムのセル、チェックボックス、LCD |
| `radius::control` | 4 | ボタン、タブ、コンボボックス、入力欄、選択枠 |
| `radius::section` | 6 | 区画 |
| `radius::overlay` | 8 | 設定、Options、確認ダイアログ |

線の太さは `stroke::hairline` 1px、`stroke::selected` 1.5px、`stroke::focus` 2px。

## 文字

| トークン | px | 使うところ |
| --- | --- | --- |
| `type::caption` | 11 | ノブ名、列見出し、補足。機能的な文字の下限 |
| `type::small` | 12 | 区画の見出し、ラベル、状態文 |
| `type::body` | 13 | 本文、操作部品 |
| `type::title` | 15.5 | OP番号、パッチ名 |
| `type::heading` | 18.5 | オーバーレイの見出し |
| `type::display` | 27 | ロゴだけ |

- 文字は必ず `font()` / `monoFont()` を通す。`juce::Font` を直接作ると、Settings の UI font が効かない。
- 入らない文字を 11px より小さく縮めない。短い書き方に切り替えるか、省く。
- 数値・アドレス・ノート名など桁をそろえて読むものは `monoFont()`、言葉は `font()`。
- 表記は文頭だけ大文字（Patch editor、Bank）。略語の列見出し（VEL、CC）はそのまま大文字でよい。

## 余白と大きさ

- 余白は `space::` の4px刻み。`xs` 4、`sm` 8、`md` 12、`lg` 16、`xl` 24。
- 操作部品の高さは `space::control`（32px）。ノブの数値欄のように部品の内側に収まるものは除く。
- 区画の見出しは `space::sectionHeader`（24px）。区画の内側の余白は `sm`、区画どうしの間隔も `sm`。
- 区画の中に箱を重ねない。内側を分けるときはヘアラインを使う（計器窓は除く）。
- グループの見出しは、上の余白を下の倍にして、見出すものの側に寄せる。

## テーマを切り替えても崩れないために

JUCEの部品は色やフォントを覚えてしまうので、次の方法で書く。

- ラベルは `setLabelFont()` で設定する。控えめな文字は引数 `dim` を使う。`setColour()` で色を固定しない。
- 入力欄は `setEditorFont()` で設定する。`TextEditor` は入力済みの文字の色とフォントを覚えているため。
- 確認は `showConfirm()`、ポップアップのパネルは `uiRoot()` を親にして開く。親なしで開くと
  テーマもUIスケールも効かない（JUCEは親のない窓に既定の LookAndFeel を使う）。
- 新しい種類のJUCE部品を使うときは、`LookAndFeel_V4` の描画に固定の色や丸みがないか確かめ、あれば
  `ui::LookAndFeel` で上書きする。

## 変更するときの確認

- [ ] Midnight、Daylight、Amber LCD、High Contrast のすべてで見た
- [ ] `Colour(0x`、`Colours::`、`brighter(`、`darker(`、数字だけの丸みを増やしていない
- [ ] 使えない状態、キーボードフォーカス、最小サイズ 1000 x 800、UIスケール 200% でも崩れない
- [ ] 文字に使う色が 4.5:1 以上、操作部品の縁とフォーカスが 3:1 以上（テストで確かめている）
- [ ] テーマとフォントを切り替えた直後に、古い色やフォントが残らない

全テーマの全画面は次のコマンドで書き出せる。

```bash
build/Release/channel_status_tests.exe build/channel-status-preview
```
