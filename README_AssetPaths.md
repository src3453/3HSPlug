# VST3 / AU のアセット読み込みパス

VST3 と Audio Unit は、プロセッサ生成時に環境変数からアセットの場所を読み込みます。ホストの作業ディレクトリに依存させないため、絶対パスを指定してください。

| 環境変数 | 指定するディレクトリ | 内容 |
| --- | --- | --- |
| `3HSPLUG_PCM_PATH` | PCMサンプルのディレクトリ | `0.wav` ～ `127.wav` |
| `3HSPLUG_PATCHES_PATH` | パッチバンクのディレクトリ | `0.json` ～ `127.json` |

## Windows (VST3)

PowerShellでユーザー環境変数を設定します。

```powershell
setx 3HSPLUG_PCM_PATH "D:\3HSPlugAssets\pcm"
setx 3HSPLUG_PATCHES_PATH "D:\3HSPlugAssets\patches"
```

設定後にDAWを再起動し、プラグインを読み込み直してください。`setx` は起動中のDAWには反映されません。

## macOS (VST3 / AU)

ターミナルからDAWを起動する前に環境変数を設定します。

```sh
launchctl setenv 3HSPLUG_PCM_PATH "/Users/you/3HSPlugAssets/pcm"
launchctl setenv 3HSPLUG_PATCHES_PATH "/Users/you/3HSPlugAssets/patches"
```

設定後にDAWを再起動し、プラグインを読み込み直してください。

## ファイル配置と既定値

- PCMサンプル名はMIDIノート番号に対応する数値の `.wav` ファイルです。
- パッチJSONはバンク番号の `.json` ファイルです。各ファイルには `patches` 配列が必要です。
- 環境変数を設定しない場合、PCMは `./pcm/`、パッチは `patches/` から読み込みます。どちらもホストの作業ディレクトリ基準です。
- パスはプラグイン生成時に読み込まれます。環境変数を変更した後は、プラグインを一度アンロードして再読み込みしてください。