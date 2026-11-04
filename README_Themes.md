# UIのテーマとフォント

プラグイン画面右上の **Settings** を開き、**Appearance** で選びます。選んだものはこのPCのユーザー設定
（Windows: `%APPDATA%\3HSPlug\3HSPlug-ui.settings`、macOS: `~/Library/Application Support/3HSPlug/`）に保存され、
DAWのプロジェクトにかかわらず、開いているすべての 3HSPlug の画面に使われます。

- **UI font**: System default / AzurZulu / future6x8_shrink（後の2つはプラグインに同梱）
- **Theme**: 同梱の Midnight（既定）/ Daylight / Amber LCD / High Contrast と、自分のテーマファイル

## 自分のテーマ

**Themes folder...** を押すとテーマ用のフォルダ（`3HSPlug\themes`）が開きます。最初に押したときは、
その時点のテーマを全項目つきで書き出した見本 `my-theme.json` が置かれます。`*.json` を置くと、次に
Settings を開いたときに一覧に出ます。

`base` で同梱テーマを選び、変えたい項目だけを書けば、残りは `base` のテーマのままです。

```json
{
  "name": "Mint",
  "base": "Midnight",
  "colours": { "accent": "#3ce0b0", "focusRing": "#3ce0b0" },
  "shape": { "roundness": 0.4, "knob": "pointer", "button": "outline" }
}
```

### colours

色は `#rrggbb` か、不透明度つきの `#aarrggbb` で書きます。

| 名前 | 使われるところ |
| --- | --- |
| `background` | 画面の地、メーターや図の背景 |
| `panel` / `panelRaised` | 区画の面 / ボタン・コンボボックス・セルの面 |
| `outline` | 区画の枠や区切り線（飾り） |
| `fieldEdge` | 操作できる部品の縁、ノブの目盛り |
| `text` / `textDim` | 文字 / 控えめな文字 |
| `accent` / `accentDim` / `onAccent` | 選択・状態・主な操作 / 文字選択の背景 / アクセントの上の文字 |
| `focusRing` | キーボードで選んだ部品の枠 |
| `carrier` / `modulator` | 出力に出るオペレーター / 変調するオペレーター |
| `meterGreen` / `meterYellow` / `meterRed` / `meterRms` | 出力メーター |
| `scrim` | 設定画面を開いたときの暗幕 |
| `lcdBack` / `lcdOn` / `lcdOff` | GSのテキスト・ドットマトリクス表示 |

読みやすさの目安として、同梱テーマは文字（`text`, `textDim`）をどの面に対しても 4.5:1 以上、
`fieldEdge`・`focusRing`・`carrier`・`modulator` をパネルに対して 3:1 以上にしています。

### shape

| 名前 | 値 |
| --- | --- |
| `roundness` | 角の丸み。0 で角、1 が標準（最大 3） |
| `knob` | `arc`（弧で値を示す）/ `pointer`（指針だけ）/ `filled`（太い弧と点） |
| `button` | `filled`（塗り）/ `outline`（枠だけ） |
| `sectionBorders` | 区画の枠線（true / false） |
| `titleGradient` | タイトルバーのグラデーション（true / false） |
| `monochromeChannels` | MIDIチャンネルの色をアクセントの濃淡にする（true / false） |
| `channelSaturation` / `channelBrightness` | MIDIチャンネルの色の彩度・明るさ（0〜1） |

読めなかった色はベースのテーマの色のまま使い、Settings の Status に理由を出します。
