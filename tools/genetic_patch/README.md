# 遺伝的アルゴリズムによる3HSパッチ探索

単音WAVに近い3HSPlugパッチを探索する独立CLIです。プラグイン本体やJUCEを
ビルドする必要はありません。`src/s3hs_core/sound.cpp` を直接使うDLLをPythonから
呼び出すため、探索用に別のFM音源を近似実装していません。

## セットアップ（Windows x64 / PowerShell）

Python 3.10以降、CMake 3.22以降、Visual Studio 2022のC++ビルドツールを使用します。
以下はリポジトリのルートで実行してください。

```powershell
cmake -S tools/genetic_patch -B tools/genetic_patch/build -G "Visual Studio 17 2022" -A x64
cmake --build tools/genetic_patch/build --config Release
python -m venv tools/genetic_patch/.venv
tools/genetic_patch/.venv/Scripts/python.exe -m pip install -r tools/genetic_patch/requirements.txt
```

音源コアを更新した場合も、上の`cmake --build`でDLLを再ビルドしてください。
探索・試聴の開始時にDLLのABIと音源ソースのSHA-256を照合し、古いDLLなら
再ビルド用のコマンドを表示して停止します。`search.json`にも使用したDLLの情報を保存します。
既存のABI 1 DLLは再ビルドが必要です。

## サンプルから探索

入力は単音・無加工の録音を想定します。**48 kHz / 16-bit PCM WAV（モノラル/ステレオ）** は
そのまま読み込み、それ以外のWAVやFLAC、MP3などはffmpegで48 kHz・16 bitの
一時WAVへ自動変換します。最初の音声ストリームを使い、モノラル／ステレオは維持し、
それ以外のチャンネル構成はffmpegでダウンミックスします。
読み込んだステレオは左右平均でモノラルにします。
元ファイルは変更せず、一時ファイルは読み込み後に削除します。
対応WAVを直接読む場合、ffmpegは不要です。変換が必要な場合はffmpegをPATHに追加するか、
`search` の引数に `--ffmpeg C:/path/to/ffmpeg.exe` を指定してください。
変換では音量正規化や無音の切り取りは行いません。
先頭の無音を事前に切り、音の立ち上がりを揃えてください。
`--note`を省略すると、デコード後の音声から基音を推定して最も近いMIDIノート番号を使います。
明示した`--note`は推定より優先します。`--note-off`はデコード後の音声先頭からの秒数で指定でき、
省略するとサンプル終端の時刻を使います（最後のサンプルまでキーを押した状態）。
途中でキーを離す場合は `--note-off` を指定し、録音末尾にリリースを含めてください。
`search.json` には実際に使用したノートオフ時刻を保存します。

自動推定はNumPyだけで実装したYIN方式で、librosaの追加インストールは不要です。
20〜5000 Hzを対象に、音量のある複数区間の推定を集約します。
無音・周期性の弱い音・区間ごとの推定が不安定な音ではエラーになり、`--note`での指定を案内します。
単音の録音を想定しており、和音やノイズ主体の音は手動指定してください。
推定周波数は整数のMIDIノートへ丸めます。録音の微妙な調律ずれを自動補正する機能ではありません。
`search.json`の`note`に採用した番号、`pitch`に手動/自動の区別・推定Hz・セント差・周期性を保存します。
方式の背景は[librosaのYIN解説](https://librosa.org/doc/main/api/generated/librosa.yin.html)も参照できます。

```powershell
tools/genetic_patch/.venv/Scripts/python.exe tools/genetic_patch/genetic_patch.py search C:/samples/a4.wav --note 69 --note-off 0.7 --population 48 --generations 100 --seed 1 --output tools/genetic_patch/results/a4
```

音程推定とノートオフ終端の自動指定を使う最小例:

```powershell
tools/genetic_patch/.venv/Scripts/python.exe tools/genetic_patch/genetic_patch.py search C:/samples/piano.wav --output tools/genetic_patch/results/piano
```

候補の合成・評価は既定で**8スレッド**で並列実行します。`--workers 4`などで変更でき、
`--workers 1`なら逐次実行します。1以上の整数を指定してください。
乱数生成・選択・交叉・保存はメイン側で行い、同じseedと設定ならスレッド数によらず
同じ探索履歴とJSON/WAV/FUIを出力します。使用したスレッド数は`search.json`に保存します。

端末では`tqdm`で世代全体と現在の世代の候補評価を2段表示します。
経過時間・残り時間の目安と、保存完了時点の最良lossを表示します。
候補のバーはキャッシュ済み・重複を除いた新規評価だけを数え、並列実行でも完了順に進みます。
`--no-progress`で非表示にできます。ファイルへのリダイレクトや非TTY環境では自動的に
世代ごとのログ表示に切り替わります。
既存環境には`python -m pip install -r tools/genetic_patch/requirements.txt`で`tqdm`を追加してください。
表示オプションの仕様は[tqdm公式ドキュメント](https://tqdm.github.io/docs/tqdm/)を参照できます。

各世代で次のファイルを保存します。`Ctrl+C` で停止しても最後に完了した保存が残ります。
既存の出力先を指定するとファイルを上書きします。

- `best_patch.json`: 既存の `tools/patch_editor.py` と3HSPlugのバンクローダーに対応。
  1パッチだけのバンクなので、既存バンクに使う場合は該当programの要素を移してください。
- `best_patch.fui`: FurnaceのS3HS楽器（type 68）。Patch EditorのFUIインポートにも対応。
- `best.wav`: 最良パッチを同じノート・同じ長さで合成した試聴用音声。
- `search.json`: 引数・乱数seed・各世代の評価値。`best_fine_loss` は小さいほど一致。

既存パッチを初期集団に入れる場合は `--initial assets/patches/0.json --program 0`
のように指定します（パスは手元のバンクに合わせてください）。キーシフトは0が必要です。
既存パッチのモードを `--modes` に含め、非サイン波を含む場合は `--waveforms` を指定します。
通常は `--modes 1,3,4,5,6` のFM系モードを探索します。
`--modes 6` なら2オペレーターFM×4に限定できます。

既存パッチの音を入力サンプルにして一連の操作を試すこともできます。

```powershell
tools/genetic_patch/.venv/Scripts/python.exe tools/genetic_patch/genetic_patch.py render C:/patches/0.json --program 0 --note 69 --duration 1 --note-off 0.7 --output tools/genetic_patch/results/reference.wav
```

`render` の出力先親フォルダーは先に作成してください。DLLを別の場所に置く場合は
サブコマンドより前に `--renderer C:/path/s3hs_patch_renderer.dll` を指定します。

保存済みのJSONパッチも、探索をやり直さずFUIに変換できます。この操作に音源DLLは不要です。

```powershell
tools/genetic_patch/.venv/Scripts/python.exe tools/genetic_patch/genetic_patch.py export-fui tools/genetic_patch/results/a4/best_patch.json --program 0 --name "GA Piano" --output tools/genetic_patch/results/a4/best_patch.fui
```

FUIはFurnaceの`FINS`ヘッダー、UTF-8楽器名の`NA`、S3HS設定の`3H`、終端の`EN`で構成します。
探索結果は`fbDest`を含む65バイトの`3H`形式で保存します。
旧JSONに`fbDest`がない場合はOP1として読み込みます。FUIコーデックは旧64バイト形式にも対応します。
周波数倍率、波形、音量、ADSR、モード、符号付きフィードバックを保存します。
MIDIプログラム番号はFUIの外で管理します。キーシフトはこの設定ブロックにないため、
`export-fui`には`keyShift=0`のパッチを指定してください（探索結果は常に0です）。
探索・試聴・JSON/FUIのすべてで`fbDest=0〜7`（OP1〜OP8）に対応します。
FUIのパラメーター保存と、Furnace本体での聴感の一致は別途確認してください。

## 初版の探索と評価

- 遺伝子: モード、フィードバック、フィードバック先、8 OPの周波数倍率・ADSR・音量。
  OP1の周波数はMIDIノートからプラグインと同じ整数Hzで設定。
  OP2〜8は1/4096単位の倍率で、整数倍付近から開始します。
- フィードバックは更新済みコアのOP単位の遅延フィードバックを使用。
  `fbDest`を`0x4003C0`に書き、候補ごとに新しいコアを作って履歴をリセットします。
  フィードバック量128はゼロで、フィードバック先は初期化・交叉・変異の対象です。
- 波形は初期状態ではサイン波固定。`--waveforms` で0〜15の波形も変異させます。
  コアで初期化されない波形番号には無音のものもあり、すべてが有用とは限りません。
- トーナメント選択、一様交叉、局所変異と整数倍への変異、エリート保存。
- 512/2048サンプル窓、75%重複のSTFT振幅を時間ごとに比較。
  周波数ビンを16→4→1個ずつ束ねて、世代数の各1/3で評価の分解能を上げます。
  切替は世代数による固定スケジュールです。
- 振幅の絶対差とlog振幅差を併用。全候補をターゲットの同一スケールで評価し、
  無音や過小音量が有利になることを抑えます。候補ごとの音量正規化は行いません。
- 全候補について最終分解能の評価も計算し、その最良個体を次世代へ残します。
  `selection_loss` は段階をまたいで比較できませんが、`best_fine_loss` は比較できます。

並列化の構成・実測結果は [PARALLELISM.md](PARALLELISM.md) にあります。
GPU、録音途中のノートオフ自動判定、複数ノート同時評価、探索再開は未実装です。
`--initial` は最良個体を引き継ぐ新しい探索であり、
乱数状態を含めた完全な再開ではありません。サンプルの倍音・ノイズが音源で表現可能か、
探索が十分かによって結果は変わり、原音の再現は保証されません。

音源条件は周波数量子化OFF、velocity/CC音量最大、パン0x88、EQ/コンプレッサーOFF。
プラグイン側で試す際も条件を揃えてください。出力WAVは音量正規化しません。
録音の音量が大きすぎる場合は音源の出力範囲に収まるよう、入力ゲインを調整してください。
単一ノートの一致だけでは鍵域全体での品質は評価できません。

## 検証

JSON/WAV/FUIの保存は、出力先と同じフォルダーに固有名の一時ファイルを作り、
完成後に置き換えます。Windowsのアクセス拒否・共有違反・ロック違反
（WinError 5/32/33）では、置き換えだけを最大3秒間、間隔を延ばしながら再試行します。
ロックが続く場合はエラーで終了し、以前のファイルと完成済み一時ファイルを残します。
エラーに表示する一時ファイルから新しい結果を回収できます。
ファイルごとの置き換えなので、保存途中の停止時には4ファイルの世代が異なる場合があります。
出力フォルダー自体に書き込み権限がない場合は、その権限・保存先を修正してください。

```powershell
Push-Location tools/genetic_patch
.venv/Scripts/python.exe -m unittest -v test_genetic_patch
Pop-Location
```

実コアでの決定的な再合成、ノートオフによる終端、音程変更、無音との評価差、
既知のFMパッチから作った音に対するGAの改善、JSON/WAVの再現、乱数seedの再現性を検証します。
全8 OPへのフィードバックの効果、候補間の履歴分離、非OP1パッチからの探索とFUI出力、
旧JSONの既定値、古いDLLの検出も検証します。
1/2/8スレッド間の探索履歴・JSON/WAV/FUIの一致、キャッシュと重複候補の評価省略、
評価失敗時のスレッド終了も検証します。
進捗バー表示・`--no-progress`・非TTYの自動切り替えで探索結果が変わらないことも検証します。
YINによる低音〜高音、第2倍音の強い音、調律ずれ・前後の無音、無音/ノイズの拒否、
ffmpeg変換後の自動推定と手動指定の優先も検証します。
ffmpegがPATH上にある場合は、44.1 kHz WAV/FLACの実変換、変換入力からの探索、
日本語・空白入りパスも検証します。
Windowsでは削除共有を許可しない実ファイルハンドルを使い、JSON/WAV/FUIの保存が
一時的なロック解除後に成功すること、解除されないと有限時間で終了して両方のデータを
保持すること、書き込み途中のエラーで以前の結果を壊さないことも検証します。
