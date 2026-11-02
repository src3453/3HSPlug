// Master bus processing: 3-band EQ followed by the compressor.
//
// The bus is in Q8 DAC units (256 = one int16 step), so 1.0 of the
// reference's normalized bus (one full-scale channel) is 32767/4 * 256.
#pragma once

#include <cstdint>

#include "hs88/biquad.hpp"

namespace hs88 {

struct MasterSettings {
    bool compressorEnabled = false;
    bool equalizerEnabled = false;
    std::uint8_t threshold = 0;       // threshold / 255 of normalized bus level
    std::uint8_t compressorRate = 0;  // max gain rise per sample, / 255
    std::uint8_t outputVolume = 0;    // make-up gain, / 32
    std::uint8_t eqLow = 0;           // gains in dB, value / 8
    std::uint8_t eqMid = 0;
    std::uint8_t eqHigh = 0;
};

struct StereoBus {
    std::int32_t left = 0;
    std::int32_t right = 0;
};

class Equalizer {
public:
    void configure(std::uint8_t low, std::uint8_t mid, std::uint8_t high);
    StereoBus process(StereoBus in);
    void reset();

private:
    Biquad lowL_, lowR_, midL_, midR_, highL_, highR_;
};

// Gain in Q16, moved toward a target by at most `rise` up / 4.0 down per
// sample and kept in [0, 256].
class SlewLimiter {
public:
    std::int64_t process(std::int64_t target);
    void setRise(std::int64_t rise) { rise_ = rise; }
    std::int64_t value() const { return value_; }
    void reset() { value_ = 0; }

private:
    std::int64_t rise_ = 0;
    std::int64_t value_ = 0;
};

class Compressor {
public:
    Compressor();
    void configure(std::uint8_t threshold, std::uint8_t rate, std::uint8_t volume);
    StereoBus process(StereoBus in);
    void reset();

private:
    std::int32_t processSide(std::int32_t in, Biquad& detector, SlewLimiter& gain) const;

    std::int64_t threshold_ = 0;  // bus units
    std::int32_t volume_ = 0;     // Q5
    Biquad detectorL_, detectorR_;
    SlewLimiter gainL_, gainR_;
};

class MasterEffects {
public:
    MasterEffects();
    void configure(const MasterSettings& settings);
    const MasterSettings& settings() const { return settings_; }
    StereoBus process(StereoBus in);
    void reset();

private:
    MasterSettings settings_;
    Equalizer equalizer_;
    Compressor compressor_;
};

}  // namespace hs88
