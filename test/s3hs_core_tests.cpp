// Tests for the S3HS_sound adapter over the integer 3HS core.
//
// The core itself (synthesis, envelopes, filters, effects) is tested against
// the original float implementation in the CPT32 repository. Here: the
// adapter reproduces the core sample for sample at 48 kHz, keeps 3HSPlug's
// omissions, resamples to other host rates, and keeps the plugin-facing API.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../src/s3hs_core/sound.cpp"

namespace {

void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

constexpr int kRegs = 0x400000;
constexpr int kLatency = 3;  // adapter output at 48 kHz trails the core

using Frames = std::vector<std::vector<std::vector<float>>>;

// A bus that drives the adapter through `ram` (as the plugin does) and a
// bare core through its register interface, with the same writes.
struct Pair {
    S3HS_sound plug;
    hs88::Engine engine;
    hs88::RegisterInterface regs{engine};
    std::vector<std::int16_t> coreLeft, coreRight;

    Pair() {
        plug.setSampleRate(48000);
        // The plugin's FM user waves are silent; give the bare core the same.
        for (int w = 0; w < 4; w++)
            for (int i = 0; i < 32; i++) engine.setTableByte(w, i, 128), engine.pcm(w).setTableByte(i, 0);
    }
    void poke(int address, int value) {
        plug.ram_poke(plug.ram, address, static_cast<Byte>(value));
        regs.write(static_cast<std::uint32_t>(address), static_cast<std::uint8_t>(value));
    }
    void keyOn(int ch) {
        poke(kRegs + 64 * ch + 0x1E, 1);
        plug.resetGate(ch);
    }
    // One block from both; the core renders the same frames.
    Frames block(int len) {
        Frames frames = plug.AudioCallBack(len);
        std::vector<std::int16_t> l(static_cast<size_t>(len)), r(static_cast<size_t>(len));
        engine.render(l.data(), r.data(), static_cast<size_t>(len));
        coreLeft.insert(coreLeft.end(), l.begin(), l.end());
        coreRight.insert(coreRight.end(), r.begin(), r.end());
        return frames;
    }
};

void voice(Pair& p, int ch, int hz, int algorithm, int feedback, int target) {
    const int base = kRegs + 64 * ch;
    p.poke(base, hz >> 8);
    p.poke(base + 1, hz & 255);
    for (int op = 1; op < 8; op++) {
        p.poke(base + op * 2, 0x10 + op * 3);
        p.poke(base + op * 2 + 1, op * 37);
    }
    for (int op = 0; op < 8; op++) {
        p.poke(base + 0x10 + op, 90 + 20 * op);
        p.poke(base + 0x20 + op * 4, op % 3);
        p.poke(base + 0x21 + op * 4, 4 + op);
        p.poke(base + 0x22 + op * 4, 150 + 10 * op);
        p.poke(base + 0x23 + op * 4, 6);
    }
    for (int i = 0; i < 4; i++) p.poke(base + 0x18 + i, ((2 * i) % 10) << 4 | (2 * i + 1) % 10);
    p.poke(base + 0x1C, algorithm);
    p.poke(base + 0x1D, 0x9F);
    p.poke(base + 0x1F, feedback);
    p.poke(kRegs + 0x3C0 + ch, target);
    p.keyOn(ch);
}

// At 48 kHz the adapter is the core, three samples late, across register
// changes between blocks: every algorithm, feedback, WT/noise, EQ and the
// compressor.
void testMatchesCore() {
    Pair p;
    for (int i = 0; i < 32; i++) p.poke(kRegs + 0x210 + i, (i * 29) & 255);
    p.poke(kRegs + 0x200, 0x01);
    p.poke(kRegs + 0x201, 0xB8);
    p.poke(kRegs + 0x202, 200);
    p.poke(kRegs + 0x203, 1);
    p.poke(kRegs + 0x207, 0x8F);
    p.poke(kRegs + 0x233, 2);  // noise on PCM/WT channel 1
    p.poke(kRegs + 0x230, 0x03);
    p.poke(kRegs + 0x232, 120);
    std::vector<float> plugLeft, plugRight;
    const int blocks[] = {64, 1, 300, 129, 512, 77, 256, 400};
    for (int b = 0; b < 8; b++) {
        voice(p, b, 120 + b * 61, (b * 5) % 13, 100 + b * 17, b);
        if (b == 3) { p.poke(kRegs + 0x2C1, 1); p.poke(kRegs + 0x2C5, 24); p.poke(kRegs + 0x2C7, 40); }
        if (b == 5) { p.poke(kRegs + 0x2C0, 1); p.poke(kRegs + 0x2C2, 90); p.poke(kRegs + 0x2C3, 60); p.poke(kRegs + 0x2C4, 40); }
        if (b == 6) p.poke(kRegs + 64 * 2 + 0x1E, 0);  // release a voice
        const Frames f = p.block(blocks[b]);
        plugLeft.insert(plugLeft.end(), f[0][12].begin(), f[0][12].end());
        plugRight.insert(plugRight.end(), f[1][12].begin(), f[1][12].end());
    }
    bool audible = false;
    for (size_t n = kLatency; n < plugLeft.size(); n++) {
        check(plugLeft[n] == p.coreLeft[n - kLatency] && plugRight[n] == p.coreRight[n - kLatency],
              "adapter differs from the core at 48 kHz");
        audible |= plugLeft[n] != 0;
    }
    check(audible, "matching test is silent");
}

// The filter registers never reach the core: output is the raw sample.
void testFilterOmitted() {
    S3HS_sound a, b;
    for (S3HS_sound* s : {&a, &b}) {
        s->setSampleRate(48000);
        for (int n = 0; n < 64; n++) s->ram_poke(s->ram, n, static_cast<Byte>((n * 13) & 255));
        s->syncPcmMemory();
        s->ram_poke(s->ram, kRegs + 0x200, 1);
        s->ram_poke(s->ram, kRegs + 0x201, 175);
        s->ram_poke(s->ram, kRegs + 0x202, 255);
        s->ram_poke(s->ram, kRegs + 0x215, 63);  // end address 63
        s->wtSync(0);
    }
    b.ram_poke(b.ram, kRegs + 0x204, 2);
    b.ram_poke(b.ram, kRegs + 0x205, 96);
    b.ram_poke(b.ram, kRegs + 0x206, 180);
    const Frames fa = a.AudioCallBack(256);
    check(fa == b.AudioCallBack(256), "PCM filter omission lost");
    check(std::any_of(fa[0][12].begin(), fa[0][12].end(), [](float v) { return v != 0; }), "PCM test silent");
}

// Without syncPcmMemory() the core has not seen the RAM written into `ram`.
void testPcmNeedsSync() {
    S3HS_sound s;
    s.setSampleRate(48000);
    for (int n = 0; n < 64; n++) s.ram_poke(s.ram, n, 255);
    s.ram_poke(s.ram, kRegs + 0x200, 1);
    s.ram_poke(s.ram, kRegs + 0x202, 255);
    s.ram_poke(s.ram, kRegs + 0x215, 63);
    const float before = s.AudioCallBack(64)[0][12].back();
    s.syncPcmMemory();
    const float after = s.AudioCallBack(64)[0][12].back();
    check(before < 0 && after > 0, "PCM RAM not loaded by syncPcmMemory");
}

// Waves A..D stay silent whatever the PCM/WT tables hold.
void testUserWavesSilent() {
    S3HS_sound s;
    s.setSampleRate(48000);
    for (int i = 0; i < 32; i++) s.ram_poke(s.ram, kRegs + 0x210 + i, 255);
    s.ram_poke(s.ram, kRegs + 0x01, 220);
    s.ram_poke(s.ram, kRegs + 0x10, 255);
    s.ram_poke(s.ram, kRegs + 0x22, 255);
    s.ram_poke(s.ram, kRegs + 0x18, 0xA0);  // OP1 = wave A
    s.ram_poke(s.ram, kRegs + 0x1F, 128);
    s.ram_poke(s.ram, kRegs + 0x1E, 1);
    s.resetGate(0);
    const Frames f = s.AudioCallBack(2048);
    check(std::all_of(f[0][12].begin(), f[0][12].end(), [](float v) { return v == 0; }), "FM user-wave omission lost");
}

// Voice stealing (gate 0 + resetGate) releases instead of holding a note.
void testStealReleases() {
    S3HS_sound s;
    s.setSampleRate(48000);
    s.ram_poke(s.ram, kRegs + 0x00, 1);
    s.ram_poke(s.ram, kRegs + 0x01, 0xB8);
    s.ram_poke(s.ram, kRegs + 0x10, 255);
    s.ram_poke(s.ram, kRegs + 0x22, 255);
    s.ram_poke(s.ram, kRegs + 0x23, 2);  // release 0.047 s
    s.ram_poke(s.ram, kRegs + 0x1F, 128);
    s.ram_poke(s.ram, kRegs + 0x1E, 1);
    s.resetGate(0);
    s.AudioCallBack(4800);
    s.ram_poke(s.ram, kRegs + 0x1E, 0);
    s.resetGate(0);
    s.AudioCallBack(4800);
    const Frames f = s.AudioCallBack(480);
    check(std::all_of(f[0][12].begin(), f[0][12].end(), [](float v) { return v == 0; }), "stolen voice keeps sounding");
    // Restart while already off: released at once, never held.
    s.resetGate(0);
    s.AudioCallBack(4800);
    const Frames g = s.AudioCallBack(480);
    check(std::all_of(g[0][12].begin(), g[0][12].end(), [](float v) { return v == 0; }), "reset of an idle voice holds a note");
}

int risingCrossings(const std::vector<float>& v) {
    int count = 0;
    for (size_t i = 1; i < v.size(); i++) count += v[i - 1] < 0 && v[i] >= 0;
    return count;
}

// Other host rates: pitch and level follow, and block sizes do not matter.
void testHostRates() {
    for (float rate : {44100.0f, 96000.0f, 22050.0f}) {
        S3HS_sound whole, split;
        for (S3HS_sound* s : {&whole, &split}) {
            s->setSampleRate(rate);
            s->ram_poke(s->ram, kRegs + 0x00, 0x01);
            s->ram_poke(s->ram, kRegs + 0x01, 0xB8);  // 440 Hz
            s->ram_poke(s->ram, kRegs + 0x10, 255);
            s->ram_poke(s->ram, kRegs + 0x22, 255);
            s->ram_poke(s->ram, kRegs + 0x1F, 128);
            s->ram_poke(s->ram, kRegs + 0x1E, 1);
            s->resetGate(0);
        }
        const int length = static_cast<int>(rate);
        const Frames f = whole.AudioCallBack(length);
        const int cycles = risingCrossings(f[0][12]);
        check(std::abs(cycles - 440) <= 1, "pitch wrong after resampling");
        const float peak = *std::max_element(f[0][12].begin(), f[0][12].end());
        check(peak > 7900 && peak < 8300, "level wrong after resampling");
        std::vector<float> pieces;
        for (int len : {1, 7, length / 3, length - 1 - 7 - length / 3}) {
            const Frames part = split.AudioCallBack(len);
            pieces.insert(pieces.end(), part[0][12].begin(), part[0][12].end());
        }
        check(pieces == f[0][12], "output depends on block sizes");
    }
}

void testApi() {
    S3HS_sound s;
    s.setFrequencyQuantizeFrequency(192000);
    check(s.getFrequencyQuantizeFrequency() == 192000, "quantization setting lost");
    unsigned char data[3] = {255, 0, 128};
    check(s.putDMABuffer(0, data, 3) == 3 && s.getDMABufferLength(0) == 3, "FIFO");
    check(s.putDMABuffer(4, data, 3) == -1 && s.putDMABuffer(0, nullptr, 3) == -3, "FIFO errors");
    check(s.ram.size() == S3HS_RAM_SIZE, "RAM image size");
    // Moves keep the chip (the plugin keeps a std::vector of these).
    std::vector<S3HS_sound> chips;
    chips.resize(3);
    chips[2].setSampleRate(48000);
    check(chips[2].AudioCallBack(16)[0][12].size() == 16, "moved chip");
}

}  // namespace

int main() {
    try {
        testMatchesCore();
        testFilterOmitted();
        testPcmNeedsSync();
        testUserWavesSilent();
        testStealReleases();
        testHostRates();
        testApi();
        std::cout << "PASS: core parity at 48 kHz, filter and FM user-wave omissions, PCM sync, "
                     "voice steal, host-rate resampling, API\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
