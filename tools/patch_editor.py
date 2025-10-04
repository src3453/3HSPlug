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
    10: "PCM1 -> Wave",
    11: "PCM2 -> Wave",
    12: "PCM3 -> Wave",
    13: "PCM4 -> Wave",
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

class PatchEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("3HSPlug Patch Bank Editor")
        self.root.geometry("1000x700")
        
        self.patch_data = {"patches": []}
        self.current_patch = None
        
        self.setup_ui()
        
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