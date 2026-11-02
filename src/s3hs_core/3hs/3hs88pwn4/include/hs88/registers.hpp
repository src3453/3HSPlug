// 3HS88PWN4 bus interface: PCM RAM and the register file in front of Engine.
// Addresses are chip-relative bytes, as in the 3HS88PWN4 reference.
#pragma once

#include <array>
#include <cstdint>

#include "hs88/engine.hpp"

namespace hs88 {

namespace reg {

constexpr std::uint32_t kPcmMemoryBase = 0x000000;  // 4 MiB, read/write
constexpr std::uint32_t kRegisterBase = 0x400000;
constexpr std::uint32_t kRegisterSize = 0x400;

// Offsets inside the register file.
constexpr std::uint32_t kHarmonicBase = 0x000;  // 8 x 64 bytes, 3HS816 channel layout
constexpr std::uint32_t kHarmonicStride = 0x40;
constexpr std::uint32_t kPcmBase = 0x200;       // 4 x 48 bytes
constexpr std::uint32_t kPcmStride = 0x30;
constexpr std::uint32_t kMasterBase = 0x2C0;
constexpr std::uint32_t kMuteBase = 0x2D0;      // 12 channels, HS then PCM
constexpr std::uint32_t kFeedbackTargetBase = 0x3C0;

// Offsets inside a PCM/WT channel block.
constexpr std::uint32_t kPcmFrequency = 0x00;   // u16 BE
constexpr std::uint32_t kPcmVolume = 0x02;
constexpr std::uint32_t kPcmMode = 0x03;
constexpr std::uint32_t kPcmFilterMode = 0x04;
constexpr std::uint32_t kPcmCutoff = 0x05;
constexpr std::uint32_t kPcmResonance = 0x06;
constexpr std::uint32_t kPcmPan = 0x07;
constexpr std::uint32_t kPcmSync = 0x08;        // any write restarts the cursor
constexpr std::uint32_t kPcmTable = 0x10;       // 32 bytes

// Offsets inside the master block.
constexpr std::uint32_t kCompressorEnable = 0x0;
constexpr std::uint32_t kEqualizerEnable = 0x1;
constexpr std::uint32_t kThreshold = 0x2;
constexpr std::uint32_t kCompressorRate = 0x3;
constexpr std::uint32_t kOutputVolume = 0x4;
constexpr std::uint32_t kEqLow = 0x5;
constexpr std::uint32_t kEqMid = 0x6;
constexpr std::uint32_t kEqHigh = 0x7;

}  // namespace reg

class RegisterInterface {
public:
    explicit RegisterInterface(Engine& engine);

    // Clear PCM RAM and every register and reset the engine to match.
    void reset();

    std::uint8_t read(std::uint32_t address) const;
    void write(std::uint32_t address, std::uint8_t value);

private:
    void writePcmChannel(int channel, std::uint32_t offset, std::uint8_t value);
    void writeMaster();

    Engine& engine_;
    std::array<std::uint8_t, reg::kRegisterSize> regs_{};
};

}  // namespace hs88
