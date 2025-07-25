/*
  # 3HS88PWN4 Specifications:
  - 8-Channel 8-Operator Harmonic Synthesizer
  - 4-Channel PCM/Wavetable/Noise Synthesizer (With IIR Filter)
  - 16-bit 48KHz Stereo Linear PCM DAC
  - 3-band EQ (Low, Mid, High)
  - Envelope Generator (ADSR)
  - FM, RM, iPD, and combination synthesis modes
  - 8-bit PCM Sample Memory (4096 KBytes)
  - PCM DAC/ADC With DMA Mode (Currently not implemented)
  - GM Level 1 Support (Melodic 16 Channels, Drums 8 Channels) (with 3SGU2X, Not implemented)


  # 3HS88PWN4 I/O Map Allocations:
  - 0x000000 - 0x3FFFFF: PCM Sample Memory (4096 KBytes)
  - 0x400000 - 0x4003FF: Register Memory (512 Bytes)
  - 0x400400 - 0x403FFF: Unused
*/

#define S3HS_RAM_SIZE 0x404000  // PCM Sample 4096KB + Reg 4KB (Only used 512Bytes)
