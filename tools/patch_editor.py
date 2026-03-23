#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PatchBank JSON Editor
パッチバンクJSONファイルをグラフィカルに編集するツール
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import json
import os
import mido

# 波形の定義
WAVEFORMS = {
    0: "Sine",
    1: "Abs. Sine",
    2: "Squished Sine",
    3: "Square",
    4: "Triangle",
    5: "Sawtooth",
    6: "Abs. Square",
    7: "Abs. Triangle",
    8: "Abs. Sawtooth",
    9: "Noise",
    10: "PCM1 -> Wave [Not implemented]",
    11: "PCM2 -> Wave [Not implemented]",
    12: "PCM3 -> Wave [Not implemented]",
    13: "PCM4 -> Wave [Not implemented]",
    14: "Abs. AC. Sine",
    15: "Alternating Sine"
    }

# モジュレーションモードの定義
MODULATION_MODES = { 
    0: "Additive",
    1: "4x2OP FM",
    2: "4x2 RingMod",
    3: "2x4OP FM",
    4: "8OP FM",
    5: "4OP FM x2",
    6: "2OP FM x4",
    7: "4OP FMxRM x2",
    8: "2x4 RingMod",
    9: "2OP FMxRM x4",
    10: "2OP DirectPhase",
    11: "4OP DirectPhase",
    12: "8OP DirectPhase"
}

# MIDIプログラム番号とGM楽器名のマッピング
GM_INSTRUMENT_NAMES = {
    0: "Acoustic Grand Piano",
    1: "Bright Acoustic Piano",
    2: "Electric Grand Piano",
    3: "Honky-tonk Piano",
    4: "Electric Piano 1",
    5: "Electric Piano 2",
    6: "Harpsichord",
    7: "Clavi",
    8: "Celesta",
    9: "Glockenspiel",
    10: "Music Box",
    11: "Vibraphone",
    12: "Marimba",
    13: "Xylophone",
    14: "Tubular Bells",
    15: "Dulcimer",
    16: "Drawbar Organ",
    17: "Percussive Organ",
    18: "Rock Organ",
    19: "Church Organ",
    20: "Reed Organ",
    21: "Accordion",
    22: "Harmonica",
    23: "Tango Accordion",
    24: "Acoustic Guitar (nylon)",
    25: "Acoustic Guitar (steel)",
    26: "Electric Guitar (jazz)",
    27: "Electric Guitar (clean)",
    28: "Electric Guitar (muted)",
    29: "Overdriven Guitar",
    30: "Distortion Guitar",
    31: "Guitar harmonics",
    32: "Acoustic Bass",
    33: "Electric Bass (finger)",
    34: "Electric Bass (pick)",
    35: "Fretless Bass",
    36: "Slap Bass 1",
    37: "Slap Bass 2",
    38: "Synth Bass 1",
    39: "Synth Bass 2",
    40: "Violin",
    41: "Viola",
    42: "Cello",
    43: "Contrabass",
    44: "Tremolo Strings",
    45: "Pizzicato Strings",
    46: "Orchestral Harp",
    47: "Timpani",
    48: "String Ensemble 1",
    49: "String Ensemble 2",
    50: "SynthStrings 1",
    51: "SynthStrings 2",
    52: "Choir Aahs",
    53: "Voice Oohs",
    54: "Synth Voice",
    55: "Orchestra Hit",
    56: "Trumpet",
    57: "Trombone",
    58: "Tuba",
    59: "Muted Trumpet",
    60: "French Horn",
    61: "Brass Section",
    62: "SynthBrass 1",
    63: "SynthBrass 2",
    64: "Soprano Sax",
    65: "Alto Sax",
    66: "Tenor Sax",
    67: "Baritone Sax",
    68: "Oboe",
    69: "English Horn",
    70: "Bassoon",
    71: "Clarinet",
    72: "Piccolo",
    73: "Flute",
    74: "Recorder",
    75: "Pan Flute",
    76: "Blown Bottle",
    77: "Shakuhachi",
    78: "Whistle",
    79: "Ocarina",
    80: "Lead 1 (square)",
    81: "Lead 2 (sawtooth)",
    82: "Lead 3 (calliope)",
    83: "Lead 4 (chiff)",
    84: "Lead 5 (charang)",
    85: "Lead 6 (voice)",
    86: "Lead 7 (fifths)",
    87: "Lead 8 (bass + lead)",
    88: "Pad 1 (new age)",
    89: "Pad 2 (warm)",
    90: "Pad 3 (polysynth)",
    91: "Pad 4 (choir)",
    92: "Pad 5 (bowed)",
    93: "Pad 6 (metallic)",
    94: "Pad 7 (halo)",
    95: "Pad 8 (sweep)",
    96: "FX 1 (rain)",
    97: "FX 2 (soundtrack)",
    98: "FX 3 (crystal)",
    99: "FX 4 (atmosphere)",
    100: "FX 5 (brightness)",
    101: "FX 6 (goblins)",
    102: "FX 7 (echoes)",
    103: "FX 8 (sci-fi)",
    104: "Sitar",
    105: "Banjo",
    106: "Shamisen",
    107: "Koto",
    108: "Kalimba",
    109: "Bag pipe",
    110: "Fiddle",
    111: "Shanai",
    112: "Tinkle Bell",
    113: "Agogo",
    114: "Steel Drums",
    115: "Woodblock",
    116: "Taiko Drum",
    117: "Melodic Tom",
    118: "Synth Drum",
    119: "Reverse Cymbal",
    120: "Guitar Fret Noise",
    121: "Breath Noise",
    122: "Seashore",
    123: "Bird Tweet",
    124: "Telephone Ring",
    125: "Helicopter",
    126: "Applause",
    127: "Gunshot"
}

"""void setPatchOverride(int bankNumber, int patchNumber, int relativeAddr, int value) {
    if (bankNumber >= 0 && bankNumber < MAX_BANKS &&
        patchNumber >= 0 && patchNumber < PATCH_BANK_SIZE) {
        Patch& patch = PatchBanks[bankNumber][patchNumber];
        // オーバーライド処理
        if (relativeAddr >= 0 && relativeAddr < 0x41) {
            if (relativeAddr >= 0x02 && relativeAddr <= 0x0F)
            {
                if ((relativeAddr & 1) == 0) {
                    patch.operators[relativeAddr/2].frequency &= 0x00FF;
                    patch.operators[relativeAddr/2].frequency |= (value << 8);
                } else// OP2-8 frequency MSB
                {
                    patch.operators[relativeAddr/2].frequency &= 0xFF00;
                    patch.operators[relativeAddr/2].frequency |= value;
                }// OP2-8 frequency LSB
            }
            if (relativeAddr >= 0x10 && relativeAddr <= 0x17)
            {
                // 音量レジスタのオーバーライド
                patch.operators[relativeAddr - 0x10].volume = value;
            }
            if (relativeAddr >= 0x18 && relativeAddr <= 0x1B)
            {
                patch.operators[(relativeAddr - 0x18)*2].waveform = (value & 0xF0) >> 4; // 波形は4bitなので下位4ビットのみを設定
                patch.operators[(relativeAddr - 0x18)*2 + 1].waveform = value & 0x0F; // MSBを設定
            }
            if (relativeAddr == 0x1C) {
                    patch.modmode = value;
            } 
            if (relativeAddr == 0x1F) {
                    patch.feedback = value;
            }
            if (relativeAddr >= 0x20 && relativeAddr <= 0x3F ) {
                int modulo = relativeAddr % 4;
                switch (modulo)
                {
                case 0:
                    patch.operators[(relativeAddr - 0x20)/4].attack = value;
                    break;
                case 1:
                    patch.operators[(relativeAddr - 0x20)/4].decay = value;
                    break;
                case 2:
                    patch.operators[(relativeAddr - 0x20)/4].sustain = value;
                    break;
                case 3:
                    patch.operators[(relativeAddr - 0x20)/4].release = value;
                    break;
                default:
                    break;
                }
            }
            if (relativeAddr == 0x40) {
                patch.keyShift = static_cast<int8_t>(value); // キーシフトはint8_tなのでキャスト
            }
        }
    }
}"""

def reinterpret_signed_byte(value):
    return value - 256 if value > 127 else value

class PatchEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("3HSPlug Patch Bank Editor")
        self.root.geometry("1000x700")
        
        self.patch_data = {"patches": []}
        self.current_patch = None
        
        self.setup_ui()

        # 3HSPlug Patch Override (SysEx: F0 7D 33 48 00 <bank#(00~7F)> <patch#(00~7F)> <relative addr(00~3F)> <data MSB(00~0F)> <data LSB(00~0F)> <checksum> F7)
    def create_patch_override_sysex(self, bank_num, patch_num, relative_addr, data):
        sysex = [0x7D, 0x33, 0x48, 0x00,
                bank_num & 0x7F,
                patch_num & 0x7F,
                relative_addr & 0x3F,
                (data & 0xF0) >> 4,
                data & 0x0F]
        checksum = (-(sum(sysex) & 0x7F)) & 0x7F
        sysex.append(checksum)
        print(f"Created SysEx: {[hex(b) for b in [0xF0]+sysex+[0xF7]]}")
        return sysex

    def send_sysex_message(self, patchnum):
        sysexes = []
        for addr in range(0x00, 0x41):
                if 0x02 <= addr <= 0x0F:  # OP2-8 frequency MSB/LSB
                    op_index = addr // 2
                    try:
                        operator = self.current_patch["operators"][op_index]
                    except IndexError:
                        continue # オペレーターが存在しない場合は終了
                    if addr % 2 == 0: # MSB
                        frequency = operator.get("frequency", 0) >> 8
                    else: # LSB
                        frequency = operator.get("frequency", 0) & 0xFF
                    sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, frequency))  # 例: bank 0, patch patchnum, relative addr addr, data 0x00
                if 0x10 <= addr <= 0x17:  # Volume
                    op_index = addr - 0x10
                    try:
                        operator = self.current_patch["operators"][op_index]
                    except IndexError:
                        continue # オペレーターが存在しない場合は終了
                    volume = operator.get("volume", 0)
                    sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, volume))
                if 0x18 <= addr <= 0x1B:  # Waveform
                    op_index = (addr - 0x18) * 2
                    try:
                        operator = self.current_patch["operators"][op_index]
                        operator2 = self.current_patch["operators"][op_index + 1]
                    except IndexError:
                        continue # オペレーターが存在しない場合は終了
                    if addr % 2 == 0:  # MSB
                        waveform = (operator.get("waveform", 0) & 0x0F) << 4
                        waveform += operator2.get("waveform", 0) & 0x0F
                        sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, waveform))
                if addr == 0x1C:  # Modulation mode
                    modmode = self.current_patch.get("modmode", 4)
                    sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, modmode))
                if addr == 0x1F:  # Feedback
                    feedback = self.current_patch.get("feedback", 128)
                    sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, feedback))
                if 0x20 <= addr <= 0x3F:  # ADSR
                    op_index = (addr - 0x20) // 4
                    modulo = (addr - 0x20) % 4
                    try:
                        operator = self.current_patch["operators"][op_index]
                    except IndexError:
                        continue # オペレーターが存在しない場合は終了
                    match modulo:
                        case 0:  # Attack
                            attack = operator.get("attack", 0)
                            sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, attack))
                        case 1:  # Decay
                            decay = operator.get("decay", 0)
                            sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, decay))
                        case 2:  # Sustain
                            sustain = operator.get("sustain", 0)
                            sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, sustain))
                        case 3:  # Release
                            release = operator.get("release", 0)
                            sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, release))
                if addr == 0x40:  # Key Shift
                    keyshift = self.current_patch.get("keyShift", 0) & 0xFF
                    sysexes.append(self.create_patch_override_sysex(0, patchnum, addr, keyshift))
                else:  # その他のアドレスはスキップ
                    continue
        for sysex in sysexes:
            ports = mido.get_output_names()
            if not ports:
                messagebox.showerror("MIDI Error", "No MIDI output ports available")
                return
            msg = mido.Message('sysex', data=sysex)  # F0とF7はmidoが自動で追加
            try:
                with mido.open_output(ports[0]) as outport:
                    outport.send(msg)
            except (AttributeError, OSError) as e:
                messagebox.showerror("MIDI Error", f"Failed to send SysEx: No MIDI output port available or rtmidi error.\n{str(e)}")
                return

    def send_current_patch_sysex(self):
        if not self.current_patch:
            messagebox.showwarning("No Patch Selected", "Please select a patch to send")
            return
        patchnum = self.current_patch.get("program", 0)
        self.send_sysex_message(patchnum)
        #messagebox.showinfo("SysEx Sent", f"SysEx messages for patch {patchnum} sent successfully")
            
    def sort_patches_by_program(self):
        """プログラム番号でパッチを昇順ソート"""
        self.patch_data["patches"].sort(key=lambda p: p.get("program", 0))
        self.update_patch_list()
        messagebox.showinfo("Info", "Patches sorted by Program#")
    
    def duplicate_patch_dialog(self):
        """パッチ複製ダイアログを表示"""
        import copy
        selection = self.patch_listbox.curselection()
        if not selection:
            messagebox.showwarning("Patch Selection", "Please select a patch to duplicate")
            return
        index = selection[0]
        patch = self.patch_data["patches"][index]
        # プログラム番号入力ダイアログ
        dialog = tk.Toplevel(self.root)
        dialog.title("Enter New Program Number")
        tk.Label(dialog, text="New Program Number (0-127):").pack(padx=10, pady=5)
        prog_var = tk.IntVar(value=patch.get("program", 0))
        spin = ttk.Spinbox(dialog, from_=0, to=127, textvariable=prog_var, width=10)
        spin.pack(padx=10, pady=5)
        def on_ok():
            new_prog = prog_var.get()
            # 既存番号チェック
            if any(p.get("program", -1) == new_prog for p in self.patch_data["patches"]):
                messagebox.showerror("Error", f"Program number {new_prog} already exists")
                return
            new_patch = copy.deepcopy(patch)
            new_patch["program"] = new_prog
            self.patch_data["patches"].append(new_patch)
            self.update_patch_list()
            dialog.destroy()
            messagebox.showinfo("Info", f"Patch duplicated to {new_prog}")
        ttk.Button(dialog, text="OK", command=on_ok).pack(side=tk.LEFT, padx=10, pady=10)
        ttk.Button(dialog, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=10, pady=10)

    
    def convert_fui_to_patch(self, fui_data):
        # FUIデータからパッチデータへの変換ロジックを実装
        # マジックナンバー: 33 48 40 00 (3H@ ) の後の64バイトのデータを想定
        # それぞれ8bitで表されるパラメーターを適切に変換してPatch構造にマッピングする必要があります。
        #0: op2fu, op3fu, op4fu, op5fu, op6fu, op7fu, op8fu, op2fl, 
        #8: op3fl, op4fl, op5fl, op6fl, op7fl, op8fl, op1w, op2w, 
        #16: op3w, op4w, op5w, op6w, op7w, op8w, op1v, op1a, 
        #24: op1d, op1s, op1r, op2v, op2a, op2d, op2s, op2r, 
        #32: op3v, op3a, op3d, op3s, op3r, op4v, op4a, op4d,
        #40: op4s, op4r, op5v, op5a, op5d, op5s, op5r, op6v,
        #48: op6a, op6d, op6s, op6r, op7v, op7a, op7d, op7s, 
        #56: op7r, op8v, op8a, op8d, op8s, op8r, mode, fb
        if len(fui_data) != 64:
            raise ValueError("Invalid FUI data length. Expected 64 bytes.")
        patch = {
            "program": -1, #FUIファイルにはプログラム番号が含まれていないため
            "modmode": fui_data[62],
            "feedback": reinterpret_signed_byte(fui_data[63])+128, #FUIのフィードバックは-128-127で表されているため、-128を加算して0-255に変換
            "keyShift": 0, #FUIファイルにはキーシフトが含まれていないため
            "operators": []
        }
        for i in range(8):
            operator = {
                "waveform": fui_data[16 + i],
                "volume": fui_data[22 + i*5 + 0],
                "attack": fui_data[22 + i*5 + 1],
                "decay": fui_data[22 + i*5 + 2],
                "sustain": fui_data[22 + i*5 + 3],
                "release": fui_data[22 + i*5 + 4]
            }
            if i != 0:
                operator["frequency"] = (fui_data[i-1] << 8) + fui_data[i-1+7]
            patch["operators"].append(operator)
        return patch

    def import_fui(self):
        filename = filedialog.askopenfilename(
            title="Import from FUI file",
            filetypes=[("FUI files", "*.fui"), ("All files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'rb') as f:
                    fui_data = f.read()
                #マジックナンバー: 33 48 40 00 (3H@ ) を探す
                magic_index = fui_data.find(b'\x33\x48\x40\x00')
                if magic_index == -1:
                    raise ValueError("Invalid FUI file: Magic number not found")
                fui_data = fui_data[magic_index + 4:magic_index + 4 + 64] #マジックナンバーの後の64バイトを抽出
                # FUIデータからパッチを変換して追加
                new_patch = self.convert_fui_to_patch(fui_data)
                # show dialog to ask for program number
                dialog = tk.Toplevel(self.root)
                dialog.title("Enter Program Number for Imported Patch")
                tk.Label(dialog, text="Program Number (0-127):").pack(padx=10, pady=5)
                prog_var = tk.IntVar(value=0)
                spin = ttk.Spinbox(dialog, from_=0, to=127, textvariable=prog_var, width=10)
                spin.pack(padx=10, pady=5)
                def on_ok():
                    new_prog = prog_var.get()
                    # 既存番号チェック
                    if any(p.get("program", -1) == new_prog for p in self.patch_data["patches"]):
                        if not messagebox.askyesno("Warning", f"Program number {new_prog} already exists. Proceed with overwrite?"):
                            dialog.destroy()
                            return
                    new_patch["program"] = new_prog
                # proceed overwrite if program number exists
                    self.patch_data["patches"] = [p for p in self.patch_data["patches"] if p.get("program", -1) != new_prog] + [new_patch]
                    dialog.destroy()
                    self.update_patch_list()
                    messagebox.showinfo("Success", "FUI file imported successfully")
                ttk.Button(dialog, text="OK", command=on_ok).pack(side=tk.LEFT, padx=10, pady=10)
                ttk.Button(dialog, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=10, pady=10)
            except Exception as e:
                messagebox.showerror("Error", f"Failed to import FUI file:\n{str(e)}")

    def setup_ui(self):
        # メニューバー
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open", command=self.load_file)
        file_menu.add_command(label="Save", command=self.save_file)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)

        # メインフレーム
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # 左側パネル: パッチリスト
        left_frame = ttk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))
        
        ttk.Label(left_frame, text="Patch list").pack()
        
        # パッチリストボックス
        list_frame = ttk.Frame(left_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        self.patch_listbox = tk.Listbox(list_frame, width=30)
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.patch_listbox.yview)
        self.patch_listbox.configure(yscrollcommand=scrollbar.set)
        
        self.patch_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.patch_listbox.bind('<<ListboxSelect>>', self.on_patch_select)
        
        # パッチ操作ボタン
        button_frame = ttk.Frame(left_frame)
        button_frame.pack(pady=10)
        
        ttk.Button(button_frame, text="New patch", command=self.new_patch).pack(pady=2)
        ttk.Button(button_frame, text="Delete Patch", command=self.delete_patch).pack(pady=2)
        ttk.Button(button_frame, text="Sort by Program#", command=self.sort_patches_by_program).pack(pady=2)
        ttk.Button(button_frame, text="Duplicate Patch...", command=self.duplicate_patch_dialog).pack(pady=2)
        ttk.Button(button_frame, text="Send SysEx", command=self.send_current_patch_sysex).pack(pady=2)
        ttk.Button(button_frame, text="Import from .fui file...", command=self.import_fui).pack(pady=2)
        
        # 右側パネル: パッチエディター
        right_frame = ttk.Frame(main_frame)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # パッチ基本情報
        info_frame = ttk.LabelFrame(right_frame, text="Patch info")
        info_frame.pack(fill=tk.X, pady=(0, 10))
        
        # プログラム番号
        ttk.Label(info_frame, text="Program number:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=2)
        self.program_var = tk.IntVar()
        self.program_spinbox = ttk.Spinbox(info_frame, from_=0, to=127, textvariable=self.program_var,
                                          command=self.on_value_change, width=10)
        self.program_spinbox.grid(row=0, column=1, padx=5, pady=2)
        self.program_spinbox.bind('<Return>', self.on_value_change)
        self.program_var.trace_add('write', self.on_value_change)
        
        # 楽器名表示
        self.instrument_label = ttk.Label(info_frame, text="", foreground="blue")
        self.instrument_label.grid(row=0, column=2, padx=10, pady=2, sticky=tk.W)
        
        # モジュレーションモード
        ttk.Label(info_frame, text="Modulation mode:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=2)
        self.modmode_var = tk.IntVar()
        self.modmode_spinbox = ttk.Spinbox(info_frame, from_=0, to=12, textvariable=self.modmode_var,
                                          command=self.on_value_change, width=10)
        self.modmode_spinbox.grid(row=1, column=1, padx=5, pady=2)
        self.modmode_spinbox.bind('<Return>', self.on_value_change)
        self.modmode_var.trace_add('write', self.on_value_change)
        modmode_label = ttk.Label(info_frame, text="(Additive)", foreground="gray")
        modmode_label.grid(row=1, column=2, padx=0, pady=2, sticky=tk.W)
        self.modmode_var.trace_add('write', lambda *args, lbl=modmode_label, v=self.modmode_var: lbl.config(text=f"({MODULATION_MODES.get(int(v.get()), 'Unknown')})"))
        
        # フィードバック
        ttk.Label(info_frame, text="Feedback amount:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=2)
        self.feedback_var = tk.IntVar(value=128)
        self.feedback_spinbox = ttk.Spinbox(info_frame, from_=0, to=255, textvariable=self.feedback_var,
                                           command=self.on_value_change, width=10)
        self.feedback_spinbox.grid(row=2, column=1, padx=5, pady=2)
        self.feedback_spinbox.bind('<Return>', self.on_value_change)
        self.feedback_var.trace_add('write', self.on_value_change)
        feedback_label = ttk.Label(info_frame, text="(0)", foreground="gray")
        feedback_label.grid(row=2, column=2, padx=0, pady=2, sticky=tk.W)
        self.feedback_var.trace_add('write', lambda *args, lbl=feedback_label, v=self.feedback_var: lbl.config(text=f"({int(v.get())-128})"))
        
        # キーシフト
        ttk.Label(info_frame, text="Key shift:").grid(row=3, column=0, sticky=tk.W, padx=5, pady=2)
        self.keyshift_var = tk.IntVar()
        self.keyshift_spinbox = ttk.Spinbox(info_frame, from_=-96, to=96, textvariable=self.keyshift_var,
                                           command=self.on_value_change, width=10)
        self.keyshift_spinbox.grid(row=3, column=1, padx=5, pady=2)
        self.keyshift_spinbox.bind('<Return>', self.on_value_change)
        self.keyshift_var.trace_add('write', self.on_value_change)
        
        # オペレーターエディター
        operators_frame = ttk.LabelFrame(right_frame, text="Operators")
        operators_frame.pack(fill=tk.BOTH, expand=True)
        
        # オペレーター選択
        op_select_frame = ttk.Frame(operators_frame)
        op_select_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(op_select_frame, text="Operators:").pack(side=tk.LEFT)
        self.operator_var = tk.IntVar(value=0)
        for i in range(8):
            ttk.Radiobutton(op_select_frame, text=f"OP{i}", variable=self.operator_var, 
                           value=i, command=self.on_operator_select).pack(side=tk.LEFT, padx=5)
        
        # オペレーターパラメーター
        params_frame = ttk.Frame(operators_frame)
        params_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # パラメーター入力フィールド
        self.operator_vars = {}
        self.operator_sliders = {}
        
        # Frequency FU (MSB)
        row = 0
        ttk.Label(params_frame, text="FU (MSB):").grid(row=row, column=0, sticky=tk.W, padx=5, pady=2)
        
        var_fu = tk.IntVar()
        self.operator_vars["frequency_msb"] = var_fu
        
        spinbox_fu = ttk.Spinbox(params_frame, from_=0, to=255, textvariable=var_fu,
                                command=self.on_operator_value_change, width=10)
        spinbox_fu.grid(row=row, column=1, padx=5, pady=2)
        spinbox_fu.bind('<Return>', self.on_operator_value_change)
        var_fu.trace_add('write', self.on_operator_value_change)
        
        slider_fu = ttk.Scale(params_frame, from_=0, to=255, orient=tk.HORIZONTAL,
                             variable=var_fu, command=self.on_operator_value_change)
        slider_fu.grid(row=row, column=2, padx=5, pady=2, sticky=tk.W)
        self.operator_sliders["frequency_msb"] = slider_fu
        
        hex_label_fu = ttk.Label(params_frame, text="(0x00)", foreground="gray")
        hex_label_fu.grid(row=row, column=3, padx=5, pady=2, sticky=tk.E)
        var_fu.trace_add('write', lambda *args, lbl=hex_label_fu, v=var_fu: lbl.config(text=f"(0x{v.get():02X})"))
        
        # Frequency FL (LSB)
        row = 1
        ttk.Label(params_frame, text="FL (LSB):").grid(row=row, column=0, sticky=tk.W, padx=5, pady=2)
        
        var_fl = tk.IntVar()
        self.operator_vars["frequency_lsb"] = var_fl
        
        spinbox_fl = ttk.Spinbox(params_frame, from_=0, to=255, textvariable=var_fl,
                                command=self.on_operator_value_change, width=10)
        spinbox_fl.grid(row=row, column=1, padx=5, pady=2)
        spinbox_fl.bind('<Return>', self.on_operator_value_change)
        var_fl.trace_add('write', self.on_operator_value_change)
        
        slider_fl = ttk.Scale(params_frame, from_=0, to=255, orient=tk.HORIZONTAL,
                             variable=var_fl, command=self.on_operator_value_change)
        slider_fl.grid(row=row, column=2, padx=5, pady=2, sticky=tk.W)
        self.operator_sliders["frequency_lsb"] = slider_fl
        
        hex_label_fl = ttk.Label(params_frame, text="(0x00)", foreground="gray")
        hex_label_fl.grid(row=row, column=3, padx=5, pady=2, sticky=tk.E)
        var_fl.trace_add('write', lambda *args, lbl=hex_label_fl, v=var_fl: lbl.config(text=f"(0x{v.get():02X})"))
        
        # Combined frequency display
        row = 2
        ttk.Label(params_frame, text="Frequency:").grid(row=row, column=0, sticky=tk.E, padx=5, pady=2)
        self.freq_combined_label = ttk.Label(params_frame, text="0 (0x0000)", foreground="blue")
        self.freq_combined_label.grid(row=row, column=1, columnspan=2, padx=5, pady=2, sticky=tk.W)
        
        # Update combined frequency when FU or FL changes
        def update_combined_freq(*args):
            combined = (var_fu.get() << 8) | var_fl.get()
            self.freq_combined_label.config(text=f"{combined} (0x{combined:04X})")
        
        var_fu.trace_add('write', update_combined_freq)
        var_fl.trace_add('write', update_combined_freq)
        
        # その他のパラメーター
        param_names = ["waveform", "volume", "attack", "decay", "sustain", "release"]
        param_ranges = [(0, 15), (0, 255), (0, 255), (0, 255), (0, 255), (0, 255),]
        
        for i, (param, (min_val, max_val)) in enumerate(zip(param_names, param_ranges)):
            row = i + 3
            ttk.Label(params_frame, text=f"{param.capitalize()}:").grid(row=row, column=0, sticky=tk.W, padx=5, pady=2)
            
            var = tk.IntVar()
            self.operator_vars[param] = var
            
            spinbox = ttk.Spinbox(params_frame, from_=min_val, to=max_val, textvariable=var,
                                 command=self.on_operator_value_change, width=10)
            spinbox.grid(row=row, column=1, padx=5, pady=2)
            spinbox.bind('<Return>', self.on_operator_value_change)
            var.trace_add('write', self.on_operator_value_change)
            
            slider = ttk.Scale(params_frame, from_=min_val, to=max_val, orient=tk.HORIZONTAL,
                              variable=var, command=self.on_operator_value_change)
            slider.grid(row=row, column=2, padx=5, pady=2, sticky=tk.W)
            self.operator_sliders[param] = slider
            
            # 16進数表示（waveformのみ）
            if param == "waveform":
                hex_label = ttk.Label(params_frame, text="(Sine)", foreground="gray")
                hex_label.grid(row=row, column=3, padx=5, pady=2, sticky=tk.E)
                var.trace_add('write', lambda *args, lbl=hex_label, v=var: lbl.config(text=f"({WAVEFORMS.get(int(v.get()), 'Unknown')})"))
        
        # カラムの重みを設定してスライダーを伸縮可能にする
        params_frame.columnconfigure(2, weight=1)
        
    def load_file(self):
        filename = filedialog.askopenfilename(
            title="Open JSON file",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'r', encoding='utf-8') as f:
                    self.patch_data = json.load(f)
                self.update_patch_list()
                messagebox.showinfo("Success", "File loaded successfully")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load file:\n{str(e)}")

    def save_file(self):
        filename = filedialog.asksaveasfilename(
            title="Save JSON file",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(self.patch_data, f, indent=2, ensure_ascii=False)
                messagebox.showinfo("Success", "File saved successfully")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save file:\n{str(e)}")

    def update_patch_list(self):
        self.patch_listbox.delete(0, tk.END)
        for patch in self.patch_data.get("patches", []):
            program = patch.get("program", 0)
            name = GM_INSTRUMENT_NAMES.get(program, f"Unknown ({program})")
            self.patch_listbox.insert(tk.END, f"{program:3d}: {name}")
    
    def on_patch_select(self, event):
        selection = self.patch_listbox.curselection()
        if selection:
            index = selection[0]
            self.current_patch = self.patch_data["patches"][index]
            self.load_patch_to_ui()
    
    def load_patch_to_ui(self):
        if not self.current_patch:
            return
        
        # トレースを一時的に無効化してUIの更新中に余計なイベントを防ぐ
        self._loading_patch = True
        
        # 基本情報をUIに反映
        self.program_var.set(self.current_patch.get("program", 0))
        self.modmode_var.set(self.current_patch.get("modmode", 4))
        self.feedback_var.set(self.current_patch.get("feedback", 128))
        self.keyshift_var.set(self.current_patch.get("keyShift", 0))
        
        # 楽器名を更新
        program = self.program_var.get()
        name = GM_INSTRUMENT_NAMES.get(program, f"Unknown ({program})")
        self.instrument_label.config(text=name)
        
        # 現在選択されているオペレーターの値を更新
        self.load_operator_to_ui()
        
        # トレースを再度有効化
        self._loading_patch = False
    
    def load_operator_to_ui(self):
        if not self.current_patch:
            return
        
        # オペレーター読み込み中フラグを設定
        self._loading_operator = True
        
        op_index = self.operator_var.get()
        operators = self.current_patch.get("operators", [])
        
        if op_index < len(operators):
            operator = operators[op_index]
            
            # Frequency を MSB と LSB に分割
            frequency = operator.get("frequency", 0)
            self.operator_vars["frequency_msb"].set((frequency >> 8) & 0xFF)
            self.operator_vars["frequency_lsb"].set(frequency & 0xFF)
            
            # その他のパラメーター
            for param, var in self.operator_vars.items():
                if param not in ["frequency_msb", "frequency_lsb"]:
                    var.set(operator.get(param, 0))
        else:
            # オペレーターが存在しない場合は0で初期化
            for var in self.operator_vars.values():
                var.set(0)
        
        # オペレーター読み込み完了
        self._loading_operator = False
    
    def on_operator_select(self):
        self.load_operator_to_ui()
    
    def on_value_change(self, *args):
        if not self.current_patch or getattr(self, '_loading_patch', False):
            return
            
        # 基本パラメーターを更新
        self.current_patch["program"] = self.program_var.get()
        self.current_patch["modmode"] = self.modmode_var.get()
        self.current_patch["feedback"] = self.feedback_var.get()
        self.current_patch["keyShift"] = self.keyshift_var.get()
        
        # 楽器名を更新
        program = self.program_var.get()
        name = GM_INSTRUMENT_NAMES.get(program, f"Unknown ({program})")
        self.instrument_label.config(text=name)
        
        # パッチリストを更新
        self.update_patch_list()
    
    def on_operator_value_change(self, *args):
        if not self.current_patch or getattr(self, '_loading_patch', False) or getattr(self, '_loading_operator', False):
            return
            
        op_index = self.operator_var.get()
        operators = self.current_patch.setdefault("operators", [])
        
        # 必要に応じてオペレーターリストを拡張
        while len(operators) <= op_index:
            operators.append({
                "frequency": 0, "attack": 0, "decay": 0, "sustain": 0,
                "release": 0, "volume": 0, "waveform": 0
            })
        
        # オペレーターパラメーターを更新
        operator = operators[op_index]
        
        # MSB と LSB から frequency を合成（int変換）
        if "frequency_msb" in self.operator_vars and "frequency_lsb" in self.operator_vars:
            msb = int(self.operator_vars["frequency_msb"].get())
            lsb = int(self.operator_vars["frequency_lsb"].get())
            operator["frequency"] = (msb << 8) | lsb
        
        # その他のパラメーター（int変換）
        for param, var in self.operator_vars.items():
            if param not in ["frequency_msb", "frequency_lsb"]:
                operator[param] = int(var.get())
    
    def new_patch(self):
        # 新しいパッチを作成
        new_program = 0
        existing_programs = {patch.get("program", 0) for patch in self.patch_data["patches"]}
        while new_program in existing_programs and new_program < 128:
            new_program += 1
        
        if new_program >= 128:
            messagebox.showwarning("Warning", "Patch limit reached. Cannot create new patch.")
            return
        
        new_patch = {
            "program": new_program,
            "modmode": 4,
            "feedback": 128,
            "keyShift": 0,
            "operators": [
                {"frequency": 0, "attack": 0, "decay": 0, "sustain": 0, "release": 0, "volume": 0, "waveform": 0}
                for _ in range(8)
            ]
        }
        
        self.patch_data.setdefault("patches", []).append(new_patch)
        self.update_patch_list()
        
        # 新しいパッチを選択
        self.patch_listbox.selection_clear(0, tk.END)
        self.patch_listbox.selection_set(tk.END)
        self.patch_listbox.see(tk.END)
        self.current_patch = new_patch
        self.load_patch_to_ui()
    
    def delete_patch(self):
        selection = self.patch_listbox.curselection()
        if not selection:
            return

        if messagebox.askyesno("Confirm", "Are you sure you want to delete the selected patch?"):
            index = selection[0]
            del self.patch_data["patches"][index]
            self.update_patch_list()
            self.current_patch = None
            
            # UIをクリア
            for var in [self.program_var, self.modmode_var, self.feedback_var, self.keyshift_var]:
                var.set(0)
            for var in self.operator_vars.values():
                var.set(0)
            self.instrument_label.config(text="")

def main():
    root = tk.Tk()
    app = PatchEditor(root)
    root.mainloop()

if __name__ == "__main__":
    main()