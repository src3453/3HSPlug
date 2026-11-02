#include "hs88/registers.hpp"

#include "hs816/registers.hpp"

namespace hs88 {

RegisterInterface::RegisterInterface(Engine& engine) : engine_(engine) {
    reset();
}

void RegisterInterface::reset() {
    regs_.fill(0);
    engine_.reset();
}

std::uint8_t RegisterInterface::read(std::uint32_t address) const {
    if (address < kPcmMemorySize) {
        return engine_.readPcmMemory(address);
    }
    const std::uint32_t offset = address - reg::kRegisterBase;
    if (address >= reg::kRegisterBase && offset < reg::kRegisterSize) {
        // The sync strobe has no state to read back.
        const bool isSync = offset >= reg::kPcmBase && offset < reg::kPcmBase + kPcmChannels * reg::kPcmStride &&
                            (offset - reg::kPcmBase) % reg::kPcmStride == reg::kPcmSync;
        return isSync ? 0 : regs_[offset];
    }
    return 0;
}

void RegisterInterface::write(std::uint32_t address, std::uint8_t value) {
    if (address < kPcmMemorySize) {
        engine_.writePcmMemory(address, value);
        return;
    }
    const std::uint32_t offset = address - reg::kRegisterBase;
    if (address < reg::kRegisterBase || offset >= reg::kRegisterSize) {
        return;
    }
    regs_[offset] = value;

    if (offset < reg::kPcmBase) {
        const std::uint32_t inBlock = offset % reg::kHarmonicStride;
        hs816::writeChannelRegister(engine_.harmonic(static_cast<int>(offset / reg::kHarmonicStride)),
                                    &regs_[offset - inBlock], inBlock, value);
    } else if (offset < reg::kPcmBase + kPcmChannels * reg::kPcmStride) {
        const std::uint32_t relative = offset - reg::kPcmBase;
        writePcmChannel(static_cast<int>(relative / reg::kPcmStride), relative % reg::kPcmStride, value);
    } else if (offset >= reg::kMasterBase && offset <= reg::kMasterBase + reg::kEqHigh) {
        writeMaster();
    } else if (offset >= reg::kMuteBase && offset < reg::kMuteBase + kChannelCount) {
        // Only 0 is audible.
        const int ch = static_cast<int>(offset - reg::kMuteBase);
        if (ch < kHarmonicChannels) {
            engine_.harmonic(ch).setMuted(value != 0);
        } else {
            engine_.pcm(ch - kHarmonicChannels).setMuted(value != 0);
        }
    } else if (offset >= reg::kFeedbackTargetBase && offset < reg::kFeedbackTargetBase + kHarmonicChannels) {
        engine_.harmonic(static_cast<int>(offset - reg::kFeedbackTargetBase)).setFeedbackTarget(value);
    }
    // Everything else is reserved: stored, no effect.
}

void RegisterInterface::writePcmChannel(int index, std::uint32_t offset, std::uint8_t value) {
    PcmChannel& channel = engine_.pcm(index);
    const std::uint8_t* block = &regs_[reg::kPcmBase + static_cast<std::uint32_t>(index) * reg::kPcmStride];

    if (offset < reg::kPcmVolume) {
        channel.setFrequency(static_cast<std::uint16_t>(block[0] << 8 | block[1]));
    } else if (offset == reg::kPcmVolume) {
        channel.setVolume(value);
    } else if (offset == reg::kPcmMode) {
        channel.setMode(value);
    } else if (offset <= reg::kPcmResonance) {
        channel.setFilter(block[reg::kPcmFilterMode], block[reg::kPcmCutoff], block[reg::kPcmResonance]);
    } else if (offset == reg::kPcmPan) {
        channel.setPan(value >> 4, value & 0x0F);
    } else if (offset == reg::kPcmSync) {
        channel.sync();
    } else if (offset >= reg::kPcmTable) {
        engine_.setTableByte(index, static_cast<int>(offset - reg::kPcmTable), value);
    }
}

void RegisterInterface::writeMaster() {
    const auto byte = [this](std::uint32_t offset) { return regs_[reg::kMasterBase + offset]; };
    MasterSettings settings;
    // Effects turn on only for exactly 1.
    settings.compressorEnabled = byte(reg::kCompressorEnable) == 1;
    settings.equalizerEnabled = byte(reg::kEqualizerEnable) == 1;
    settings.threshold = byte(reg::kThreshold);
    settings.compressorRate = byte(reg::kCompressorRate);
    settings.outputVolume = byte(reg::kOutputVolume);
    settings.eqLow = byte(reg::kEqLow);
    settings.eqMid = byte(reg::kEqMid);
    settings.eqHigh = byte(reg::kEqHigh);
    engine_.setMasterSettings(settings);
}

}  // namespace hs88
