# PCMサンプルとパッチJSONの読み込みパス指定機能

## 概要

3HSPlugにPCMサンプルファイルとパッチJSONファイルの読み込みパスをコマンドライン引数で指定できる機能を追加しました。

## 機能

- `--pcm-path`: PCMサンプルファイルが格納されているディレクトリパスを指定
- `--patch-json`: パッチバンクJSONファイルのパスを指定
- `--help`, `-h`: ヘルプメッセージを表示

## 使用方法

### スタンドアロンアプリケーション

```bash
# デフォルトパスを使用（./pcm/, patch_bank.json）
3HSPlug

# PCMパスのみ指定
3HSPlug --pcm-path ./custom_samples/

# 両方のパスを指定
3HSPlug --pcm-path ./my_samples/ --patch-json ./configs/my_patches.json

# ヘルプを表示
3HSPlug --help
```

### デフォルト値

- PCMパス: `./pcm/`
- パッチJSONパス: `patch_bank.json`

引数が指定されていない場合、またはプラグイン形式で動作している場合はデフォルト値が使用されます。

## 実装詳細

### 追加されたファイル

1. **CommandLineArgs.h/cpp**: コマンドライン引数解析クラス
2. **GlobalCommandLineArgs.h/cpp**: グローバルな引数管理クラス
3. **StandaloneApp.h/cpp**: JUCEアプリケーションエントリーポイント
4. **Main.cpp**: テスト用のメイン関数（デバッグ目的）

### 変更されたファイル

1. **PluginProcessor.h/cpp**: 
   - パス設定メンバ変数の追加
   - グローバル引数からのパス取得
   - 初期化処理の分離

2. **DrumPcmSampleLoader.h/cpp**:
   - `loadAllDrumSamples()`関数にパスパラメータ追加

3. **PatchBankData.h/cpp**:
   - `initializePatchBank()`関数にパスパラメータ追加

4. **CMakeLists.txt**:
   - 新しいソースファイルをビルドに追加

### アーキテクチャ

```
StandaloneApplication → CommandLineArgs → GlobalCommandLineArgs → PluginProcessor → DrumPcmSampleLoader/PatchBankData
```

1. **StandaloneApplication**: JUCEアプリケーションエントリーポイント（START_JUCE_APPLICATION使用）
2. **CommandLineArgs**: 引数解析の実装
3. **GlobalCommandLineArgs**: シングルトンパターンで引数をグローバル管理
4. **PluginProcessor**: 初期化時にグローバル引数からパスを取得
5. **DrumPcmSampleLoader/PatchBankData**: 動的パスでファイル読み込み

### エントリーポイント

- **スタンドアロンアプリケーション**: `StandaloneApplication`クラスが`START_JUCE_APPLICATION`マクロで起動
- **プラグイン形式**: 従来通りJUCEのプラグインホストが直接PluginProcessorを作成
- **テスト用**: `Main.cpp`で独立したテスト実行（コマンドライン引数のテストのみ）

## テスト方法

### 1. ビルド

```bash
cd /path/to/3HSPlug
mkdir build
cd build
cmake ..
cmake --build .
```

### 2. 実行テスト

```bash
# デフォルトパス
./3HSPlug

# カスタムパス
./3HSPlug --pcm-path ./assets/pcm/ --patch-json ./assets/patch_bank.json
```

### 3. ファイル確認

指定したパスにPCMファイル（35.wav, 36.wav等）とpatch_bank.jsonが存在することを確認してください。

## トラブルシューティング

### よくある問題

1. **ファイルが見つからない**
   - 指定したパスが正しいか確認
   - 相対パスの場合、実行ディレクトリからの相対位置を確認

2. **コンパイルエラー**
   - CMakeLists.txtに全ての新しいソースファイルが含まれているか確認
   - JUCE Headerのインクルードパスが正しいか確認

3. **引数が認識されない**
   - 引数の形式が正しいか確認（`--pcm-path`と`--patch-json`）
   - スペースで区切られているか確認

## 拡張予定

- 設定ファイルからのパス読み込み
- 環境変数からのパス取得
- GUIでのパス設定機能
- 複数のパッチファイル対応

## ライセンス

このプロジェクトはAGPLv3ライセンスの下で公開されています。