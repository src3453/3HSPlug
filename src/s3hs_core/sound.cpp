// S3HS_sound: the 3HSPlug interface to the 3HS88PWN4, running on the integer
// 3HS core shared with Furnace (vendored in 3hs/, see 3hs/README.md).
//
// The plugin keeps writing the chip's memory image in `ram` (PCM RAM at 0,
// registers at 0x400000) and calls AudioCallBack() per block. As in the old
// float core, register bytes are latched at the start of each block: changed
// bytes are forwarded to the core in address order, then pending
// resetGate()/wtSync() requests are applied.
//
// Omissions kept from the previous 3HSPlug core:
// - The PCM/WT channel filter: its registers (+4..+6) are never forwarded, so
//   samples play unfiltered and unclamped (sample * volume).
// - FM user waves: WT table bytes reach only their PCM/WT channel, never the
//   harmonic waves A..D, which stay silent.
//
// The core runs at 48 kHz; output is resampled to the host rate with cubic
// (Catmull-Rom) interpolation. At a 48 kHz host rate that reproduces the
// core's samples exactly, three samples late.
#ifndef S3HS_SOUND_ADAPTER
#define S3HS_SOUND_ADAPTER

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "hs88/registers.hpp"

class S3HS_sound {
public:
    #include "ram.cpp"  // `ram` (the chip's memory image) and the ram_* helpers

    S3HS_sound() : chip(std::make_unique<Chip>()) {
        initSound();
    }
    S3HS_sound(S3HS_sound&&) noexcept = default;
    S3HS_sound& operator=(S3HS_sound&&) noexcept = default;
    S3HS_sound(const S3HS_sound&) = delete;
    S3HS_sound& operator=(const S3HS_sound&) = delete;

    // Power-on: registers cleared, PCM RAM in `ram` kept and loaded.
    void initSound() {
        ram_boot(ram);
        std::fill(ram.begin() + kRegisterBase, ram.begin() + kRegisterBase + kRegisterSize, Byte{0});
        chip->regs.reset();
        silenceUserWaves();
        shadow.fill(0);
        pendingReset.fill(false);
        pendingSync.fill(false);
        syncPcmMemory();
        resetResampler();
    }

    // Copy PCM RAM from `ram` into the core. Call after writing PCM data into
    // `ram` (registers are picked up automatically at the next block).
    void syncPcmMemory() {
        syncPcmMemory(0, kPcmMemorySize);
    }

    // Copy only [begin, end) of PCM RAM into the core.
    void syncPcmMemory(std::uint32_t begin, std::uint32_t end) {
        end = std::min<std::uint32_t>(end, kPcmMemorySize);
        for (std::uint32_t address = begin; address < end; address++) {
            chip->engine.writePcmMemory(address, ram[address]);
        }
    }

    void setSampleRate(float sampleRate) {
        hostRate = sampleRate > 0 ? sampleRate : static_cast<float>(hs88::kSampleRate);
        step = hs88::kSampleRate / static_cast<double>(hostRate);
    }

    // Restart channel `ch`'s envelopes, phases and feedback history at the
    // next block. If its gate is off then, the restarted envelopes are
    // released at once (the old core's voice-steal path released them too).
    void resetGate(int ch) {
        if (ch >= 0 && ch < hs88::kHarmonicChannels) pendingReset[ch] = true;
    }

    // Rewind PCM/WT channel `ch`'s cursor at the next block.
    void wtSync(int ch) {
        if (ch >= 0 && ch < hs88::kPcmChannels) pendingSync[ch] = true;
    }

    // FIFO (mode 5) data for PCM/WT channel `ch`. Returns the queue length,
    // -1 for a bad channel, -2 on overflow (as much as fits is kept), -3 for
    // a null pointer.
    int putDMABuffer(int ch, unsigned char* data, size_t dataSize) {
        if (ch < 0 || ch >= hs88::kPcmChannels) return -1;
        if (data == nullptr) return -3;
        return chip->engine.pcm(ch).pushFifo(data, dataSize);
    }

    int getDMABufferLength(int ch) {
        if (ch < 0 || ch >= hs88::kPcmChannels) return -1;
        return static_cast<int>(chip->engine.pcm(ch).fifoLength());
    }

    // The 192 kHz period quantization of frequency registers was removed from
    // the 3HS cores; the setting is kept for compatibility and has no effect.
    int getFrequencyQuantizeFrequency() const { return frequencyQuantizeFrequency; }
    void setFrequencyQuantizeFrequency(int frequency) { frequencyQuantizeFrequency = frequency; }

    // frames[side][0..11][i]: per-channel output (HS 0..7, PCM 8..11) in the
    // old core's scale; frames[side][12][i]: the chip output in int16 range.
    std::vector<std::vector<std::vector<float>>> AudioCallBack(int len) {
        const size_t count = static_cast<size_t>(std::max(len, 0));
        latchRegisters();
        // Render this block's source frames now, after the latch.
        renderFrames(static_cast<size_t>(std::floor(position + static_cast<double>(count) * step)));

        std::vector<std::vector<std::vector<float>>> frames(
            2, std::vector<std::vector<float>>(hs88::kChannelCount + 1, std::vector<float>(count, 0.0f)));
        for (size_t i = 0; i < count; i++) {
            const float t = static_cast<float>(position);
            for (int k = 0; k < kOutputs; k++) {
                float value = cubic(window[0][k], window[1][k], window[2][k], window[3][k], t);
                if (k >= kMasterLeft) value = std::clamp(value, -32768.0f, 32767.0f);
                frames[k % 2][k / 2][i] = value;
            }
            position += step;
            while (position >= 1.0) {
                position -= 1.0;
                advanceWindow();
            }
        }
        return frames;
    }

    hs88::Engine& core() { return chip->engine; }

private:
    struct Chip {
        hs88::Engine engine;
        hs88::RegisterInterface regs{engine};
    };

    static constexpr std::uint32_t kRegisterBase = 0x400000;
    static constexpr std::uint32_t kRegisterSize = 0x400;
    static constexpr std::uint32_t kPcmMemorySize = 0x400000;
    static constexpr std::uint32_t kPcmBlockBase = 0x200;
    static constexpr std::uint32_t kPcmBlockStride = 0x30;
    static constexpr std::uint32_t kPcmTable = 0x10;

    // Resampler columns, side-interleaved: (channel c, side s) at 2c + s; the
    // master is channel 12.
    static constexpr int kOutputs = 2 * (hs88::kChannelCount + 1);
    static constexpr int kMasterLeft = 2 * hs88::kChannelCount;
    using Frame = std::array<float, kOutputs>;

    // Taps are DAC units; the old core reported channel * pan / 15 before the
    // mix (/32768 * 32767/4), and its PCM channels before the mix's 1.3.
    static constexpr float kTapToLegacy = 4.0f * 32768.0f / 32767.0f;
    static constexpr float kPcmMixGain = 1.3f;

    // Waves A..D read the four PCM/WT tables in the full chip; 3HSPlug omits
    // that, so they are set to silence (sample 128 = 0) once at reset.
    void silenceUserWaves() {
        for (int wave = 0; wave < hs88::kPcmChannels; wave++) {
            for (int i = 0; i < hs88::kTableLength; i++) {
                chip->engine.setTableByte(wave, i, 128);
                chip->engine.pcm(wave).setTableByte(i, 0);
            }
        }
    }

    void forwardRegister(std::uint32_t offset, std::uint8_t value) {
        if (offset >= kPcmBlockBase && offset < kPcmBlockBase + hs88::kPcmChannels * kPcmBlockStride) {
            const int ch = static_cast<int>((offset - kPcmBlockBase) / kPcmBlockStride);
            const std::uint32_t inBlock = (offset - kPcmBlockBase) % kPcmBlockStride;
            if (inBlock >= 4 && inBlock <= 6) {
                return;  // filter omitted
            }
            if (inBlock >= kPcmTable) {
                chip->engine.pcm(ch).setTableByte(static_cast<int>(inBlock - kPcmTable), value);
                return;  // no FM user waves
            }
        }
        chip->regs.write(kRegisterBase + offset, value);
    }

    void latchRegisters() {
        for (std::uint32_t offset = 0; offset < kRegisterSize; offset++) {
            const std::uint8_t value = ram[kRegisterBase + offset];
            if (value != shadow[offset]) {
                shadow[offset] = value;
                forwardRegister(offset, value);
            }
        }
        for (int ch = 0; ch < hs88::kHarmonicChannels; ch++) {
            if (!pendingReset[ch]) continue;
            pendingReset[ch] = false;
            hs816::Channel& channel = chip->engine.harmonic(ch);
            const bool wasOn = channel.gateOn();
            channel.keyOn();
            if (!wasOn) channel.keyOff();
        }
        for (int ch = 0; ch < hs88::kPcmChannels; ch++) {
            if (!pendingSync[ch]) continue;
            pendingSync[ch] = false;
            chip->engine.pcm(ch).sync();
        }
    }

    static float cubic(float p0, float p1, float p2, float p3, float t) {
        return p1 + 0.5f * t * (p2 - p0 + t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 + t * (3.0f * (p1 - p2) + p3 - p0)));
    }

    // Append `count` 48 kHz frames of every output column to `pending`.
    void renderFrames(size_t count) {
        pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(pendingIndex));
        pendingIndex = 0;
        if (count == 0) return;
        std::vector<std::int16_t> left(count), right(count);
        std::array<std::vector<std::int32_t>, hs88::kChannelCount> tapL, tapR;
        hs88::ChannelOutputs taps;
        for (int ch = 0; ch < hs88::kChannelCount; ch++) {
            tapL[ch].resize(count);
            tapR[ch].resize(count);
            taps.left[ch] = tapL[ch].data();
            taps.right[ch] = tapR[ch].data();
        }
        chip->engine.render(left.data(), right.data(), count, &taps);
        for (size_t i = 0; i < count; i++) {
            Frame frame{};
            for (int ch = 0; ch < hs88::kChannelCount; ch++) {
                const float scale = kTapToLegacy / (ch >= hs88::kHarmonicChannels ? kPcmMixGain : 1.0f);
                frame[2 * ch] = static_cast<float>(tapL[ch][i]) * scale;
                frame[2 * ch + 1] = static_cast<float>(tapR[ch][i]) * scale;
            }
            frame[kMasterLeft] = left[i];
            frame[kMasterLeft + 1] = right[i];
            pending.push_back(frame);
        }
    }

    // Shift in the next frame; render one on demand if the block's estimate
    // fell short by a rounding step.
    void advanceWindow() {
        if (pendingIndex >= pending.size()) renderFrames(1);
        window[0] = window[1];
        window[1] = window[2];
        window[2] = window[3];
        window[3] = pending[pendingIndex++];
    }

    void resetResampler() {
        pending.clear();
        pendingIndex = 0;
        position = 0.0;
        window = {};
    }

    std::unique_ptr<Chip> chip;
    std::array<std::uint8_t, kRegisterSize> shadow{};
    std::array<bool, hs88::kHarmonicChannels> pendingReset{};
    std::array<bool, hs88::kPcmChannels> pendingSync{};
    int frequencyQuantizeFrequency = 0;

    float hostRate = static_cast<float>(hs88::kSampleRate);
    double step = 1.0;
    double position = 0.0;
    std::array<Frame, 4> window{};
    std::vector<Frame> pending;
    size_t pendingIndex = 0;
};

#endif  // S3HS_SOUND_ADAPTER
