import fluidsynth
import numpy as np
import soundfile as sf
import os
import sys

SAMPLE_RATE = 44100
OUT_DIR = r"C:\JUCEProj\3HSPlug\build\3HSPlug_artefacts\Release\Standalone\pcm"
os.makedirs(OUT_DIR, exist_ok=True)

sf2_path = sys.argv[1]
bank = int(sys.argv[2])
if bank != 128:
    print(f"Warning: Bank {bank} is not 128. This script is designed for drum kits, which are typically on bank 128. Proceeding anyway.")
preset = int(sys.argv[3])

fs = fluidsynth.Synth(samplerate=SAMPLE_RATE)
sfid = fs.sfload(sf2_path)

channel = 9
fs.program_select(channel, sfid, bank, preset)
fs.set_chorus(0,0,0,0,0)  # コーラスエフェクトを無効化
fs.set_reverb(0,0,0,0)  # リバーブエフェクトを無効化

def trim_silence(audio, threshold=0.001):
    # ステレオ → モノラル的に扱う（最大値）
    energy = np.max(np.abs(audio), axis=1)

    indices = np.where(energy > threshold)[0]

    if len(indices) == 0:
        return audio  # 全部無音

    start = indices[0]
    end = indices[-1] + 1

    return audio[start:end]

def render_note(note, duration=1.0):
    fs.noteon(channel, note, 127)

    audio1 = fs.get_samples(int(SAMPLE_RATE * duration))
    fs.noteoff(channel, note)
    audio2 = fs.get_samples(int(SAMPLE_RATE * 0.5))

    audio = np.concatenate([audio1, audio2])
    audio = np.array(audio)

    # dtype補正
    if audio.dtype == np.int16:
        audio = audio.astype(np.float32) / 32768.0
    else:
        audio = audio.astype(np.float32)

    audio = audio.reshape(-1, 2)

    return audio

# --------------------------
# ① 全ノートレンダリング & 最大値取得
# --------------------------
notes_audio = {}
global_peak = 0.0

for note in range(0, 128): # MIDIノート0-127
    audio = render_note(note)
    audio = trim_silence(audio)

    peak = np.max(np.abs(audio))
    if peak > global_peak: # 最高値更新
        global_peak = peak
    if peak <= 0.001: # 0.001以下はほぼ無音とみなす (未定義なノートやサンプルがないノートなど)
        print(f"Warning: Note {note} has very low peak ({peak:.6f}). Ignoring.")
    else: # 有効なノートとして保存
        notes_audio[note] = audio
        print(f"Note {note}: peak={peak:.6f}, length={len(audio)} samples")


print("Global peak:", global_peak)

# --------------------------
# ② ノーマライズして書き出し
# --------------------------
target_peak = 0.99  # クリップ防止
gain = target_peak / global_peak if global_peak > 0 else 1.0

for note, audio in notes_audio.items():
    audio_norm = audio * gain

    filename = f"{OUT_DIR}/{note}.wav"
    sf.write(filename, audio_norm, SAMPLE_RATE)

    print(f"Exported {filename}")