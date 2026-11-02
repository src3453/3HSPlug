// 3HS816 register file: byte-addressed bus interface in front of the Engine.
// The address map is specified in doc/spec_3HS816.md.
#pragma once

#include <array>
#include <cstdint>

#include "hs816/engine.hpp"

namespace hs816 {

namespace reg {

// Region bases and sizes (chip-relative byte addresses).
constexpr std::uint32_t kChannelBase = 0x000;
constexpr std::uint32_t kChannelStride = 0x40;
constexpr std::uint32_t kUserWaveBase = 0x400;
constexpr std::uint32_t kMuteBase = 0x480;
constexpr std::uint32_t kFeedbackTargetBase = 0x490;
constexpr std::uint32_t kIdBase = 0x4F8;
constexpr std::uint32_t kSize = 0x500;

// Offsets inside one channel block.
constexpr std::uint32_t kFrequency = 0x00;     // u16 BE
constexpr std::uint32_t kRatio = 0x02;         // OP2..OP8, u16 BE each
constexpr std::uint32_t kVolume = 0x10;        // OP1..OP8
constexpr std::uint32_t kWave = 0x18;          // 2 operators per byte, high nibble first
constexpr std::uint32_t kAlgorithm = 0x1C;
constexpr std::uint32_t kPan = 0x1D;           // high nibble L, low nibble R
constexpr std::uint32_t kGate = 0x1E;
constexpr std::uint32_t kFeedback = 0x1F;
constexpr std::uint32_t kEnvelope = 0x20;      // OP1..OP8 x {A, D, S, R}

constexpr std::array<std::uint8_t, 8> kId = {'3', 'H', 'S', '8', '1', '6', 1, 0};

}  // namespace reg

// Apply a write to one 64-byte channel block. `block` is that channel's
// register bytes with the new value already stored at `offset`. Shared with
// the 3HS88PWN4, whose harmonic channels use the same block layout.
void writeChannelRegister(Channel& channel, const std::uint8_t* block, std::uint32_t offset, std::uint8_t value);

class RegisterInterface {
public:
    explicit RegisterInterface(Engine& engine);

    // Clear every register and reset the engine to match.
    void reset();

    std::uint8_t read(std::uint32_t address) const;
    void write(std::uint32_t address, std::uint8_t value);

private:
    Engine& engine_;
    std::array<std::uint8_t, reg::kSize> regs_{};
};

}  // namespace hs816
