#include "hs816/registers.hpp"

namespace hs816 {

RegisterInterface::RegisterInterface(Engine& engine) : engine_(engine) {
    reset();
}

void RegisterInterface::reset() {
    regs_.fill(0);
    for (std::size_t i = 0; i < reg::kId.size(); i++) {
        regs_[reg::kIdBase + i] = reg::kId[i];
    }
    engine_.reset();
}

std::uint8_t RegisterInterface::read(std::uint32_t address) const {
    return address < reg::kSize ? regs_[address] : 0;
}

void RegisterInterface::write(std::uint32_t address, std::uint8_t value) {
    if (address >= reg::kSize || address >= reg::kIdBase) {
        return;  // outside the chip, or the read-only ID
    }
    regs_[address] = value;

    if (address < reg::kUserWaveBase) {
        const std::uint32_t offset = address % reg::kChannelStride;
        writeChannelRegister(engine_.channel(static_cast<int>(address / reg::kChannelStride)),
                             &regs_[address - offset], offset, value);
    } else if (address < reg::kMuteBase) {
        const std::uint32_t offset = address - reg::kUserWaveBase;
        engine_.setUserWaveSample(static_cast<int>(offset / kUserWaveLength),
                                  static_cast<int>(offset % kUserWaveLength), value);
    } else if (address < reg::kMuteBase + kChannelCount) {
        // Only 0 is audible.
        engine_.channel(static_cast<int>(address - reg::kMuteBase)).setMuted(value != 0);
    } else if (address >= reg::kFeedbackTargetBase && address < reg::kFeedbackTargetBase + kChannelCount) {
        engine_.channel(static_cast<int>(address - reg::kFeedbackTargetBase)).setFeedbackTarget(value);
    }
    // Everything else is reserved: stored, no effect.
}

void writeChannelRegister(Channel& channel, const std::uint8_t* block, std::uint32_t offset, std::uint8_t value) {
    const auto read16 = [block](std::uint32_t at) { return static_cast<std::uint16_t>(block[at] << 8 | block[at + 1]); };

    if (offset < reg::kRatio) {
        channel.setFrequency(read16(reg::kFrequency));
    } else if (offset < reg::kVolume) {
        const int op = static_cast<int>((offset - reg::kFrequency) / 2);
        channel.setOperatorRatio(op, read16(offset - offset % 2));
    } else if (offset < reg::kWave) {
        channel.setOperatorVolume(static_cast<int>(offset - reg::kVolume), value);
    } else if (offset < reg::kAlgorithm) {
        const int op = static_cast<int>(offset - reg::kWave) * 2;
        channel.setOperatorWave(op, value >> 4);
        channel.setOperatorWave(op + 1, value & 0x0F);
    } else if (offset == reg::kAlgorithm) {
        channel.setAlgorithm(value);
    } else if (offset == reg::kPan) {
        channel.setPan(value >> 4, value & 0x0F);
    } else if (offset == reg::kGate) {
        // 1 retriggers even when the gate is already on; 0 releases; other
        // values are stored without effect.
        if (value == 1) {
            channel.keyOn();
        } else if (value == 0) {
            channel.keyOff();
        }
    } else if (offset == reg::kFeedback) {
        channel.setFeedback(value);
    } else {
        const int op = static_cast<int>((offset - reg::kEnvelope) / 4);
        const std::uint8_t* at = block + reg::kEnvelope + op * 4;
        channel.setOperatorEnvelope(op, EnvelopeRegisters{at[0], at[1], at[2], at[3]});
    }
}

}  // namespace hs816
