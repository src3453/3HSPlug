// Per-operator ADSR envelope, integer-only.
//
// Time is an integer count of half-samples spent in the current segment, so
// changing a segment's length mid-segment rescales the position the same way
// the 3HS88PWN4 did (it compared elapsed seconds against the new length).
// The position t = elapsed / length comes from a per-length reciprocal
// (a division on register write, or a ROM in hardware) and one multiply.
// Attack is linear; Decay and Release follow 1 - 2^(-10t) through the
// generated 2^(-x) table.
#pragma once

#include <cstdint>

namespace hs816 {

// Raw register bytes of one operator's envelope.
struct EnvelopeRegisters {
    std::uint8_t attack = 0;
    std::uint8_t decay = 0;
    std::uint8_t sustain = 0;
    std::uint8_t release = 0;
};

// One segment: length in half-samples (0 = zero-length) and ceil(2^40 / length).
struct EnvelopeSegment {
    std::uint32_t length = 0;
    std::uint32_t reciprocal = 0;

    static EnvelopeSegment ofLength(std::uint32_t halfSamples);
};

struct EnvelopeRates {
    EnvelopeSegment attack;
    EnvelopeSegment decay;    // never zero-length
    EnvelopeSegment release;
    std::int32_t sustain = 0;  // Q16

    static EnvelopeRates fromRegisters(const EnvelopeRegisters& regs);
};

class Envelope {
public:
    enum class Stage { Attack, Decay, Sustain, Release };

    // Restart from Attack. The level is kept until the next advance(), so the
    // first sample after a retrigger still sees the previous level.
    void restart();
    // Enter Release from the current level. Repeated calls keep the elapsed
    // time but re-latch the start level.
    void release();
    // Compute this sample's level, then advance one sample.
    void advance(const EnvelopeRates& rates);

    std::int32_t level() const { return level_; }  // Q16, 0..65536
    Stage stage() const { return stage_; }

private:
    Stage stage_ = Stage::Attack;
    std::uint32_t elapsed_ = 0;  // half-samples in the current segment, saturating
    std::int32_t level_ = 0;
    std::int32_t releaseFrom_ = 0;
};

}  // namespace hs816
