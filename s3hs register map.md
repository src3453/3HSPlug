3HS88PWN4
A core sound engine source code of 3HS88PWN4, a fantasy sound chip that written in C++. Also called S3HS for short, 3SGUC2 for internal Sound Generation Unit.

3HS88PWN4 Specifications:
8-Channel 8-Operator Harmonic Synthesizer
4-Channel PCM/Wavetable/Noise Synthesizer (With IIR Filter)
16-bit 48KHz Stereo Linear PCM DAC
3-band EQ (Low, Mid, High)
Envelope Generator (ADSR)
FM, RM, iPD, and combination synthesis modes
8-bit PCM Sample Memory (4096 KBytes)
Modulation with dynamic wavetable from PCM channel
3HS88PWN4 I/O Map Allocations:
0x000000 - 0x3FFFFF: PCM Sample Memory (4096 KBytes)
0x400000 - 0x4003FF: Register Memory (512 Bytes)
0x400400 - 0x403FFF: Unused

register maps:

|RAM Addr.| |0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F| | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|0x400000| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH1| |
|0x400010| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x400020| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x400030| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x400040| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH2| |
|0x400050| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x400060| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x400070| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x400080| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH3| |
|0x400090| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x4000A0| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x4000B0| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x4000C0| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH4| |
|0x4000D0| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x4000E0| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x4000F0| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x400100| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH5| |
|0x400110| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x400120| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x400130| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x400140| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH6| |
|0x400150| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x400160| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x400170| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x400180| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH7| |
|0x400190| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x4001A0| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x4001B0| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x4001C0| |OP1 Freq| |OP2 Freq| |OP3 Freq	| |OP4 Freq| |OP5 Freq| |OP6 Freq| |OP7 Freq| |OP8 Freq	| |CH8| |
|0x4001D0| |OP1V|OP2V|OP3V|OP4V|OP5V|OP6V|OP7V|OP8V|OP1W OP2W|OP3W OP4W|OP5W OP6W|OP7W OP8W|Mod Mode|L/R Vol|Gate|Mod FB| | |
|0x4001E0| |OP1A|OP1D|OP1S|OP1R|OP2A|OP2D|OP2S|OP2R|OP3A|OP3D|OP3S|OP3R|OP4A|OP4D|OP4S|OP4R| | |
|0x4001F0| |OP5A|OP5D|OP5S|OP5R|OP6A|OP6D|OP6S|OP6R|OP7A|OP7D|OP7S|OP7R|OP8A|OP8D|OP8S|OP8R| | |
|0x400200| |Frequency| |Vol|Mode|Filt Mode|Filt Cutoff|Reso/ BW|L/R Vol|(Reserved)| | | | | | | |CH9| |
|0x400210| |PCM Addr S (or Waveform)| | |PCM Addr E (or Waveform)| | |PCM Addr LS  (or Waveform)| | |Waveform (or not used)| | | | | | | | |
|0x400220| |Waveform (or not used)| | | | | | | | | | | | | | | | | |
|0x400230| |Frequency| |Vol|Mode|Filt Mode|Filt Cutoff|Reso/ BW|L/R Vol|(Reserved)| | | | | | | |CH10| |
|0x400240| |PCM Addr S (or Waveform)| | |PCM Addr E (or Waveform)| | |PCM Addr LS  (or Waveform)| | |Waveform (or not used)| | | | | | | | |
|0x400250| |Waveform (or not used)| | | | | | | | | | | | | | | | | |
|0x400260| |Frequency| |Vol|Mode|Filt Mode|Filt Cutoff|Reso/ BW|L/R Vol|(Reserved)| | | | | | | |CH11| |
|0x400270| |PCM Addr S (or Waveform)| | |PCM Addr E (or Waveform)| | |PCM Addr LS  (or Waveform)| | |Waveform (or not used)| | | | | | | | |
|0x400280| |Waveform (or not used)| | | | | | | | | | | | | | | | | |
|0x400290| |Frequency| |Vol|Mode|Filt Mode|Filt Cutoff|Reso/ BW|L/R Vol|(Reserved)| | | | | | | |CH12| |
|0x4002A0| |PCM Addr S (or Waveform)| | |PCM Addr E (or Waveform)| | |PCM Addr LS  (or Waveform)| | |Waveform (or not used)| | | | | | | | |
|0x4002B0| |Waveform (or not used)| | | | | | | | | | | | | | | | | |
|0x4002C0| |Comp Enable|EQ Enable|Comp Thres|Comp Ratio|Comp Volume|EQ Low|EQ Mid|EQ High|Comp RateU|Comp RateL| | | | | | | | |
|0x4002D0| |CH1 Mute|CH2 Mute|CH3 Mute|CH4 Mute|CH5 Mute|CH6 Mute|CH7 Mute|CH8 Mute|CH9 Mute|CH10 Mute|CH11 Mute|CH12 Mute| | | | | | |
|0x4002E0| | | | | | | | | | | | | | | | | | | |
|0x4002F0| | | | | | | | | | | | | | | | | | | |
|0x400300| | | | | | | | | | | | | | | | | | | |
|0x400310| | | | | | | | | | | | | | | | | | | |
|0x400320| | | | | | | | | | | | | | | | | | | |
|0x400330| | | | | | | | | | | | | | | | | | | |
|0x400340| | | | | | | | | | | | | | | | | | | |
|0x400350| | | | | | | | | | | | | | | | | | | |
|0x400360| | | | | | | | | | | | | | | | | | | |
|0x400370| | | | | | | | | | | | | | | | | | | |
|0x400380| | | | | | | | | | | | | | | | | | | |
|0x400390| | | | | | | | | | | | | | | | | | | |
|0x4003A0| | | | | | | | | | | | | | | | | | | |
|0x4003B0| | | | | | | | | | | | | | | | | | | |
|0x4003C0| | | | | | | | | | | | | | | | | | | |
|0x4003D0| | | | | | | | | | | | | | | | | | | |
|0x4003E0| | | | | | | | | | | | | | | | | | | |
