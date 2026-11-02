# S3HS core

3HSPlugの3HS88PWN4は、Furnaceと共通の整数演算コア（CPT32リポジトリの
`chips/3hs816`・`chips/3hs88pwn4`）で動作する。コアは
`src/s3hs_core/3hs/` に複製してあり、変更はCPT32側で行ってから複製し直す
（`src/s3hs_core/3hs/README.md`）。仕様はCPT32の `doc/spec_3HS816.md`。

`src/s3hs_core/sound.cpp` の `S3HS_sound` は従来のインターフェースを保つ
アダプター。

- プラグインは従来どおり `ram`（PCM RAM `0x000000`、レジスタ `0x400000`）へ
  書き込む。レジスタは各ブロックの先頭で読み取り、変化したバイトだけをコアへ
  送る（従来のコアと同じタイミング）。続けて `resetGate()`/`wtSync()` を適用する。
- `resetGate()` は8オペレーターのエンベロープ・位相・フィードバック履歴を
  リセットする。その時点でGateがオフならすぐリリースに入る（ボイススチール時の
  従来の動作。オフのまま鳴り続けることはない）。
- PCMデータを `ram` へ転送した後は `syncPcmMemory()` を呼ぶ（`initSound()` は
  自動で読み込む）。
- コアは48 kHz固定。ホストのサンプルレートへはCatmull-Rom補間で変換する。
  48 kHzではコアの出力と完全に一致し、3サンプル遅れる。
- 周波数レジスタはHzそのまま。192 kHz周期への量子化は廃止したので、
  `setFrequencyQuantizeFrequency()` は互換のため値を保持するだけで効果はない。

3HSPlugで省略している機能は引き続き省略する。

- PCMフィルター：レジスタ（+4〜+6）をコアへ送らない。PCMはフィルターなし・
  クリップなしで鳴る。
- FM用ユーザー波形（波形A〜D）：WT表のバイトはPCM/WTチャンネルにだけ送り、
  波形A〜Dは無音のまま。

パッチJSONの `fbDest` は0〜7（OP1〜OP8）。項目のない既存パッチは0（OP1）を使う。
JSON保存時にも書き出し、発音時に各チップの `0x4003C0 + チャンネル番号` へ反映する。
パッチオーバーライドSysExの相対アドレス `0x41` でも設定可能（`0x40` は従来のkeyShift）。
JSONとオーバーライドの範囲外の値は0〜7に制限する。

```json
{
  "patches": [
    { "program": 0, "modmode": 4, "feedback": 200, "fbDest": 7, "operators": [] }
  ]
}
```

検証方法は [test/README.md](test/README.md) を参照。
