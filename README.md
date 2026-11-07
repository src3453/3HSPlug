# 3HSPlug

**8オペレーターFM音源とPCMドラムを備えた、MIDI対応のソフトウェアシンセサイザーです。**

3HSPlugは、DAWのVST3 / Audio Unitプラグインとして、またはスタンドアロンアプリとして使えます。FMパッチを画面で編集しながら演奏でき、GM互換のMIDIデータをそのまま鳴らせます。音源にはFurnaceやCPT32と共通の整数演算コア（3HS88PWN4）を使っています。

![3HSPlugのSynthタブ（8オペレーターのパッチエディタ）](docs/images/3hsplug-patch-editor.png)

## 主な機能

- 1ボイスあたり8オペレーターのFM音源。オペレーターごとに波形、周波数比、レベル、エンベロープを調整できます。
- 13種類のアルゴリズム（接続方式）と、フィードバック量・フィードバック先の設定。
- MIDI 16チャンネルに対応。バンク／プログラムチェンジ、ピッチベンド、サスティン、各種コントローラーを受け付けます。
- チップを1〜16個まで増やせます（1チップ = FM 8ボイス + PCM 4ボイス）。初期値は4チップで、最大128 FMボイス／64 PCMボイスです。
- 音を鳴らしたままパッチを編集でき、発音中のノートにも反映されます。
- ドラム用のPCMサンプルをノートごとに割り当て可能（ドラッグ＆ドロップ対応）。
- チャンネルごとのオシロスコープ、MIDI状態モニター、ボイス使用状況、ステレオメーター。
- 4種類の内蔵テーマと、自作テーマ（JSON）、フォント・UIサイズの切り替え。

## インストール

[GitHub Releases](https://github.com/src3453/3HSPlug/releases)から、お使いのOS向けのZIPをダウンロードして展開します。ZIPにはプラグイン、スタンドアロンアプリ、パッチバンク（`assets/patches`）、ドラムサンプル（`assets/pcm`）が入っています。

- Windows: x64
- macOS: Universal（Apple Silicon / Intel）
- `nightly`は最新の開発版（プレリリース）です。安定版を使いたい場合は、`v`から始まるバージョンのリリースを選んでください。

### DAWで使う

1. `3HSPlug.vst3`フォルダ（バンドル全体）をDAWが読み込むVST3フォルダに入れます。macOSではAudio Unitの`3HSPlug.component`も使えます。
2. DAWでプラグインを再スキャンし、MIDIトラックに**3HSPlug**をインストゥルメントとして読み込みます。
3. 右上の**Settings**を開き、**Asset folders**で次の2つを指定します。
   - **Patch folder...** : ZIP内の`assets/patches`
   - **PCM folder...** : ZIP内の`assets/pcm`
4. トラックにMIDIを送ります。メロディはチャンネル1、ドラムはチャンネル10を使うのが基本です（GM準拠）。

フォルダの場所は、DAWのプロジェクト（プラグイン状態）に保存されます。ZIPを展開した場所は動かさないでください。フォルダを切り替えると中身が読み込み直されるので、必要なら先に編集内容を保存しておきます。

### スタンドアロンで使う

Windowsでは`3HSPlug.exe`、macOSでは`3HSPlug.app`を起動します。JUCE標準の**Options**メニューからオーディオデバイスとMIDI入力を選び、上と同じ手順でアセットフォルダを設定してください。MIDIキーボードがなくても、**Synth**タブでCH 10を選び、ドラムのノートを選んで**Play**を押せば音を確認できます。

## 画面の見かた

画面上部のタイトルバーに、バージョン、**Panic**（鳴っている音をすべて止める）、**Settings**があります。右端には**Master**（マスター音量とレベルメーター）が常に表示されます。メインの領域は次のタブに分かれています。

### Synthタブ（パッチエディタ）

- **Channel activity**（上部）: 16チャンネルのMIDI入力の状況です。クリックするとそのチャンネルが編集対象になります。
- **パッチエディタ**（中央）: 編集するチャンネルを選び、そのチャンネルのパッチを編集します。
  - **バンク／パッチ番号**: 数字を入力してEnterを押すと、そのチャンネルにバンクセレクトとプログラムチェンジを送ります。
  - **Mode（アルゴリズム）**: 0〜12の13種類。Additive、4x2OP FM、4x2 RingMod、2x4OP FM、8OP FM、4OP FM x2、2OP FM x4などがあります。右側の図で、キャリア（音として出るオペレーター）とモジュレーター（変調をかけるオペレーター）の接続を確認できます。
  - **Feedback**: フィードバック量（-128〜+127、0でなし）。**Feedback to OPn**でフィードバックをかけるオペレーターを選びます。
  - **Key Shift**: 半音単位のキーシフト（-48〜+48）。
  - **オペレーター（OP1〜OP8）**: 波形（サイン、ノコギリ波、矩形波、ノイズなど16種類）、**Coarse**（周波数比の整数倍。x0〜x16）、**Fine**（そこからの微調整。-0.5〜+0.5）、**Level**、**Attack / Decay / Sustain / Release**（各0〜255）を設定します。OP1は常に鍵盤の音程で鳴るため、CoarseとFineは使えません。
  - **Save Bank**: 現在のバンクをパッチフォルダのJSONファイルに書き出します。
  - **Revert**: このパッチをJSONファイルの内容に戻します。
  - 保存していない編集は、DAWのプロジェクトには残りますが、JSONファイルには反映されません。同じバンク・プログラムを使うチャンネルは、同一インスタンス内で編集内容を共有します。
- **ドラムキット**（ドラムチャンネル選択時）: パッチエディタの代わりにキット編集画面が開きます。ノートを選んで**Assign...**でサンプルを割り当てるか、ノートにオーディオファイルをドロップします。**Play**で試聴、**Clear**で割り当て解除、**Load Kit... / Save Kit...**でキット（JSON）の読み書き、**Reset to Default**でPCMフォルダの`<ノート番号>.wav`に戻します。キットは全ドラムチャンネルで共通です。PCMメモリは1チップあたり4 MiBまでです。
- **Voices**（下部）: チップごとのFM 8ボイス／PCM 4ボイスの使用状況です。

![ドラムキットの編集画面](docs/images/3hsplug-drum-kit.png)

### Channelsタブ

16チャンネルすべての状態を一覧表示します。プログラム、バンク（MSB : LSB）、ベロシティ・ボリューム・エクスプレッション・パン・ベンド・モジュレーション・ホールドなどのコントローラー値、受信中のノートを鍵盤で確認できます。チャンネルを選んで編集に移ることもできます。

![Channelsタブ](docs/images/3hsplug-channels.png)

### Scopeタブ

ボイスごとの波形を表示するオシロスコープです。1ページに1チップ分（FM 8 + PCM 4 = 12チャンネル）が並び、`<` `>`でチップを切り替えます。波形は安定して見えるようにトリガーがかかります。表示が止まっているタブでは取得・描画を行わないため、使っていない間は負荷がかかりません。右上の**Options**で表示を調整できます。設定はこのPCに保存され、次回以降も引き継がれます。

| 項目 | 内容 |
| --- | --- |
| View | **Voice**（ボイスごと）／**MIDI channel**（同じMIDIチャンネルに割り当てられたボイスを足し合わせ、16チャンネル分を表示） |
| Window | 表示する時間幅。5 / 10 / 20 / 40 / 80 ms（初期値は20 ms） |
| Gain | 振幅の拡大。**Auto**（自動調整）または x1 / x2 / x4 / x8 / x16 / x32 |
| FPS | 更新頻度。10 / 15 / 20 / 30 / 60 / 120（初期値は30） |
| DC removal | 波形の縦方向のずれ（DCオフセット）を取り除く方式。下で説明します |
| DC smoothing | **Mean**と**Max-Min**で使う、補正値の平滑化。下で説明します |
| Colour | **Single colour**（テーマの文字色）／**Per voice**（ボイスごと）／**Per MIDI channel**（MIDIチャンネルごと） |

#### DC removal（DCオフセット除去）

波形が上下どちらかに偏って表示されるとき（DCオフセット）に、中心を0に合わせ直します。表示にだけ効果があり、実際に出力される音は変わりません。トリガーの判定にも効くので、波形の止まり方も安定します。

- **Off**: 何もしません（初期値）。
- **Mean**: 表示している区間の平均値を引きます。
- **High-pass**: 出力段と同じハイパスフィルター（約20 Hz）を通します。
- **Max-Min**: 区間内の最大値と最小値の中間を引きます。波形が非対称でも、上下の振れ幅が同じになるように中央へ寄せます。

#### DC smoothing（平滑化）

**Mean**と**Max-Min**では、画面を更新するたびに補正値が変わると、波形が縦にふらつくことがあります。DC smoothingは、補正値を前回までの値となめらかにつなぐ設定です（指数移動平均）。**Off / Weak / Strong**から選びます。

- **Off**: 毎回の補正値をそのまま使います。変化にすぐ追従しますが、ふらつきやすくなります。
- **Weak**: 新しい値を30%反映します。
- **Strong**: 新しい値を10%だけ反映します。ふらつきが最も小さい反面、音が急に変わったときの追従はゆるやかです（初期値）。

DC removalが**Off**または**High-pass**のときは、DC smoothingは使われません。

### Debugタブ

動作確認や不具合調査向けのタブです。初期状態では表示されません。**Settings → Show Debug tab**をオンにすると現れます。

- **Chip registers**: チップを選び、レジスタメモリ（0x400000〜0x4003FF）を16進数で表示します。直前に書き換わったバイトが光って、徐々に消えます。
- **Log**: MIDIメッセージなどのログです。**Clear**で消去できます。

## Settings

右上の**Settings**で開きます。**Close**で閉じます。

**Engine**

- **Number of chips**: エミュレートするチップ数（1〜16）。1チップごとにFM 8ボイスが増えます。
- **Voice allocation**: 新しいノートをFMボイスに割り当てる方式です。ボイスが埋まっているときは、古いボイスから奪います。
  - *Rolling*: ボイスを順番に使います（初期値）。
  - *Lowest free voice first*: 空いている若い番号のボイスから使います。
  - *Least recently used*: 最も長く使われていないボイスを使います。

**Program change**

- **Override Program Change**: オンにすると、MIDIのバンクセレクトとプログラムチェンジを無視し、下の**Bank**と**Program**で指定したパッチを全チャンネルで使います。

**Asset folders**

- **PCM folder...**: `0.wav`〜`127.wav`を含むフォルダを選びます。ドラムキットがその内容で作り直され、手で割り当てたノートは置き換わります。
- **Patch folder...**: `0.json`〜`127.json`を含むフォルダを選びます。パッチバンクを読み込み直し、保存していない編集は失われます。

**Appearance**（このPC上のすべての3HSPlugに共通で、DAWプロジェクトには保存されません）

- **UI font**: System default / AzurZulu / future6x8_shrink。
- **UI scale**: 画面全体の拡大率。
- **Theme**: Midnight（初期値）/ Daylight / Amber LCD / High Contrast、または自作テーマ。**Themes folder...**で自作テーマ用のフォルダを開けます。作り方は[テーマとフォントの説明](README_Themes.md)を参照してください。
- **Show Debug tab**: Debugタブの表示／非表示。
- **Log to console (stderr)**: ログをコンソールにも出力します（Debugタブにはつねに表示されます）。

**Reset**

- **GM Reset**: すべてのMIDIチャンネルを初期状態に戻し、SysExによるパッチの上書きも元に戻します。
- **All Notes Off**: 鳴っている音をすべて止めます。

**Status**: サンプルレート、バッファサイズ、CPU使用率、読み込み中のパッチ／PCMフォルダを表示します。

## アセットフォルダの設定

付属のパッチバンクは`assets/patches`、ドラムサンプルは`assets/pcm`にあります。

| アセット | ファイル名 | 内容 |
| --- | --- | --- |
| パッチバンク | `<バンク番号>.json`（0〜127） | `patches`配列を持つJSON |
| ドラムサンプル | `<ノート番号>.wav`（0〜127） | そのMIDIノートに割り当てるサンプル |

設定が保存されていないときは、環境変数`3HSPLUG_PATCHES_PATH`と`3HSPLUG_PCM_PATH`を参照し、どちらもなければホストの作業フォルダ基準の`patches/`と`./pcm/`を使います。DAWで確実に使うには、Settingsから指定してください。詳細は[アセットフォルダの設定](README_AssetPaths.md)にあります。

## 既知の制限

- コアは48 kHz固定です。ホストのサンプルレートが異なる場合は補間で変換します。
- PCMフィルターは未対応です。
- FMのユーザー波形A〜Dは無音です。
- 古いデータにある周波数量子化の設定は、現在の整数コアでは効果がありません。

## 開発者向け

ビルドにはCMake 3.22以降、C++17コンパイラ、JUCE 8.0.8（`lib/JUCE`のサブモジュール）が必要です。

```sh
git clone --recurse-submodules https://github.com/src3453/3HSPlug.git
```

**Windows**（x64のVisual Studio開発者シェルで）:

```powershell
cmake -S . -B build/ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/ninja --parallel 4 --target 3HSPlug_Standalone 3HSPlug_VST3
```

**macOS**:

```sh
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos --parallel 4 --target 3HSPlug_Standalone 3HSPlug_VST3 3HSPlug_AU
```

ユニバーサルビルドは、構成時に`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`を加えます。

**回帰テスト**:

```sh
cmake -S test -B build/s3hs-tests
cmake --build build/s3hs-tests --config Release
ctest --test-dir build/s3hs-tests -C Release --output-on-failure
```

**リリース**: `v`で始まるタグ（例: `v0.0.1`）をpushすると、GitHub ActionsがWindowsとmacOSをビルド・テストし、ZIPをReleasesに公開します。`main`へのpushと毎日03:17 JSTには`nightly`プレリリースが更新されます。プルリクエストではビルドとテストのみ行います。

関連ドキュメント: [テストの説明](test/README.md) / [サウンドコアの統合](README_S3HS.md) / [コアの取り込み手順](src/s3hs_core/3hs/README.md) / [外部パッチエディタ](tools/README_patch_editor.md) / [パッチの自動生成ツール](tools/genetic_patch/README.md) / [UIデザイン言語](docs/design-language.md)

## ライセンス

[LICENSE](LICENSE)（GNU General Public License version 2）を参照してください。JUCEなど同梱コンポーネントのライセンスは、それぞれの表示に従います。
