// Small C ABI around the actual plugin engine. Each render starts with fresh state.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include "renderer_build.h"
#include "../../src/s3hs_core/sound.cpp"

#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__((visibility("default")))
#endif

// regs: Patch::toRegValues(255) layout, with OP1 replaced by integer Hz.
// Output: mono float, mean of the engine's final L/R output, divided by 32768.
// No global engine state: separate calls may run concurrently.
EXPORT int s3hs_renderer_abi_version() noexcept { return 2; }
EXPORT const char* s3hs_renderer_source_id() noexcept { return S3HS_RENDERER_SOURCE_ID; }

EXPORT int s3hs_render_v2(const uint8_t* regs, int fbDest, int sampleRate, int count,
                       int noteOff, float* output) noexcept
{
    if (!regs || !output || sampleRate != 48000 || count <= 0 ||
        noteOff < 1 || noteOff > count || regs[0x1c] > 12 || fbDest < 0 || fbDest > 7)
        return -1;
    try {
        auto sound = std::make_unique<S3HS_sound>();
        sound->initSound();
        sound->setSampleRate(static_cast<float>(sampleRate));
        sound->setFrequencyQuantizeFrequency(0); // plugin default, no period quantization
        for (int i = 0; i < 64; ++i)
            sound->ram_poke(sound->ram, 0x400000 + i, regs[i]);
        // Feedback destination is outside the 64-byte voice register bank.
        sound->ram_poke(sound->ram, 0x4003c0, static_cast<uint8_t>(fbDest));
        sound->ram_poke(sound->ram, 0x40001d, 0x88);
        sound->ram_poke(sound->ram, 0x40001e, 1);
        sound->resetGate(0);
        for (int pos = 0; pos < count;) {
            if (pos == noteOff)
                sound->ram_poke(sound->ram, 0x40001e, 0);
            const int end = pos < noteOff ? noteOff : count;
            const int n = std::min(256, end - pos);
            auto frames = sound->AudioCallBack(n);
            for (int i = 0; i < n; ++i)
                output[pos + i] = (frames[0][12][i] + frames[1][12][i]) / 65536.0f;
            pos += n;
        }
        return 0;
    } catch (...) {
        return -2;
    }
}

// Preserve the original C entry point for older callers using the default OP1.
EXPORT int s3hs_render(const uint8_t* regs, int sampleRate, int count,
                       int noteOff, float* output) noexcept
{
    return s3hs_render_v2(regs, 0, sampleRate, count, noteOff, output);
}
