#include "hs816/channel.hpp"

#include <algorithm>

#include "hs816/wavetables.hpp"

namespace hs816 {

namespace {

// Lookup phases are signed Q32 cycles: the operator's own phase (whole turns
// + Q32 fraction) plus signed modulation. The table index is the phase in
// 1/256 cycles truncated toward zero, so phases in (-1/256, 1/256) all read
// entry 0. That dead zone shapes FM attacks (the operator phase starts at 0)
// and the direct-phase algorithms, so it is part of the sound.

// Modulation never exceeds 4 ops * 4 cycles plus 4 cycles of feedback, so
// once an operator has run this many whole turns its looked-up phase is
// always positive and the turn count can saturate.
constexpr std::uint32_t kTurnsSaturated = 32;

// A Q15 modulator signal moves the phase by 4 cycles per 1.0:
// signal / 2^15 * 4 * 2^32 = signal << 19.
std::int64_t modulationToPhase(std::int64_t signal) {
    return signal * (std::int64_t{1} << 19);
}

// Feedback adds mean(h0, h1) * (fb - 128) / 128 * 4 cycles:
// (h0 + h1) / 2 / 2^15 * (fb - 128) / 2^7 * 2^34 = (h0 + h1) * (fb - 128) << 11.
std::int64_t feedbackToPhase(std::int32_t historySum, int feedback) {
    return static_cast<std::int64_t>(historySum) * (feedback - 128) * (std::int64_t{1} << 11);
}

// Table position: phase / 2^24 truncated toward zero, modulo 256.
unsigned tablePosition(std::int64_t phase) {
    const std::int64_t position = phase >= 0 ? phase >> 24 : -((-phase) >> 24);
    return static_cast<unsigned>(position) & 0xFF;
}

// Q32 turns per sample for an operator: F Hz times the Q12 ratio over the
// 48 kHz rate, F * R / 4096 / 48000 cycles = F * R * 2^20 / 48000, rounded
// up. Rounding up keeps the phase from falling behind the exact rational
// phase, so exactly periodic pitches (750 Hz = 64 samples per cycle) reach
// each table boundary they hit exactly instead of stopping one LSB short; the
// excess is under 1 LSB per sample, far below one table step (2^24). 0 Hz
// stops the phase. The whole-turn part (up to 22 at 65535 Hz x 16) only
// feeds the saturating turn count.
std::uint64_t phaseStepFor(std::uint16_t hz, std::uint64_t ratio) {
    return (static_cast<std::uint64_t>(hz) * ratio * (std::uint64_t{1} << 20) + kSampleRate - 1) / kSampleRate;
}

// --- Algorithm routing ------------------------------------------------------
//
// An algorithm is a list of groups whose outputs are summed or multiplied.
// Each group is a chain of stages; every stage sums its operators (bit i =
// OPi+1, in ascending order) and phase-modulates the next stage. The first
// stage runs on its own phase. In direct-phase algorithms the later stages
// use only the modulation, not their own phase.

struct Group {
    int stageCount;
    std::uint8_t stages[kOperatorCount];
};

struct Algorithm {
    bool multiplyGroups;
    bool directPhase;
    int groupCount;
    Group groups[4];
};

constexpr Group kAllAdditive = {1, {0xFF}};
constexpr Group kFourModulateFour = {2, {0xF0, 0x0F}};
constexpr Group kPairChain = {4, {0xC0, 0x30, 0x0C, 0x03}};
constexpr Group kEightChain = {8, {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}};
constexpr Group kChain8To5 = {4, {0x80, 0x40, 0x20, 0x10}};
constexpr Group kChain4To1 = {4, {0x08, 0x04, 0x02, 0x01}};
constexpr Group kPair2To1 = {2, {0x02, 0x01}};
constexpr Group kPair8To7 = {2, {0x80, 0x40}};
constexpr Group kPair6To5 = {2, {0x20, 0x10}};
constexpr Group kPair4To3 = {2, {0x08, 0x04}};

constexpr Algorithm kAlgorithms[kAlgorithmCount] = {
    /* 0 additive         */ {false, false, 1, {kAllAdditive}},
    /* 1 FM 4->4          */ {false, false, 1, {kFourModulateFour}},
    /* 2 ring 4x4         */ {true, false, 2, {{1, {0x0F}}, {1, {0xF0}}}},
    /* 3 FM pair chain    */ {false, false, 1, {kPairChain}},
    /* 4 FM 8-op chain    */ {false, false, 1, {kEightChain}},
    /* 5 FM 4-op x2       */ {false, false, 2, {kChain8To5, kChain4To1}},
    /* 6 FM 2-op x4       */ {false, false, 4, {kPair2To1, kPair8To7, kPair6To5, kPair4To3}},
    /* 7 FM 4-op x2, ring */ {true, false, 2, {kChain8To5, kChain4To1}},
    /* 8 ring of 4 pairs  */ {true, false, 4, {{1, {0x03}}, {1, {0x0C}}, {1, {0x30}}, {1, {0xC0}}}},
    /* 9 FM 2-op x4, ring */ {true, false, 4, {kPair2To1, kPair8To7, kPair6To5, kPair4To3}},
    /* 10 direct 4->4     */ {false, true, 1, {kFourModulateFour}},
    /* 11 direct pairs    */ {false, true, 1, {kPairChain}},
    /* 12 direct 8 chain  */ {false, true, 1, {kEightChain}},
};

}  // namespace

Channel::Channel() {
    reset();
}

void Channel::reset() {
    params_ = ChannelParams{};
    ops_ = {};
    for (int op = 0; op < kOperatorCount; op++) {
        setOperatorEnvelope(op, EnvelopeRegisters{});
    }
    setPan(0, 0);
    updatePhaseSteps();
    gateOn_ = false;
}

void Channel::setFrequency(std::uint16_t hz) {
    params_.frequency = hz;
    updatePhaseSteps();
}

void Channel::setOperatorRatio(int op, std::uint16_t ratio) {
    params_.operators[op].ratio = ratio;
    updatePhaseSteps();
}

void Channel::setOperatorVolume(int op, std::uint8_t volume) {
    params_.operators[op].volume = volume;
}

void Channel::setOperatorWave(int op, std::uint8_t wave) {
    params_.operators[op].wave = wave & 0x0F;
}

void Channel::setOperatorEnvelope(int op, const EnvelopeRegisters& envelope) {
    params_.operators[op].envelope = envelope;
    ops_[op].rates = EnvelopeRates::fromRegisters(envelope);
}

void Channel::setPan(std::uint8_t left, std::uint8_t right) {
    params_.panLeft = left & 0x0F;
    params_.panRight = right & 0x0F;
    const bool centered = params_.panLeft == 0 && params_.panRight == 0;
    panGainLeft_ = centered ? 15 : params_.panLeft;
    panGainRight_ = centered ? 15 : params_.panRight;
}

void Channel::keyOn() {
    for (Operator& op : ops_) {
        op.envelope.restart();
        op.phase = 0;
        op.turns = 0;
        op.history[0] = op.history[1] = 0;
    }
    gateOn_ = true;
}

void Channel::keyOff() {
    if (!gateOn_) {
        return;
    }
    for (Operator& op : ops_) {
        op.envelope.release();
    }
    gateOn_ = false;
}

void Channel::updatePhaseSteps() {
    for (int op = 0; op < kOperatorCount; op++) {
        const std::uint64_t ratio = op == 0 ? kRatioOne : params_.operators[op].ratio;
        ops_[op].phaseStep = phaseStepFor(params_.frequency, ratio);
    }
}

std::int32_t Channel::tick(const WaveTables& waves) {
    for (int index = 0; index < kOperatorCount; index++) {
        Operator& op = ops_[index];
        // The level from before this step's advance is the one heard.
        op.amplitude = (op.envelope.level() * params_.operators[index].volume) >> 16;
        op.envelope.advance(op.rates);
        const std::uint64_t next = op.phase + op.phaseStep;
        op.phase = static_cast<std::uint32_t>(next);
        if (op.turns < kTurnsSaturated) {
            op.turns = static_cast<std::uint32_t>(std::min<std::uint64_t>(op.turns + (next >> 32), kTurnsSaturated));
        }
    }
    return renderAlgorithm(waves);
}

// One operator lookup: wave sample times amplitude (Q15). The feedback target
// also adds its own last two outputs to its phase.
std::int32_t Channel::evaluateOperator(int index, std::int64_t phase, const WaveTables& waves) {
    Operator& op = ops_[index];
    if (index == params_.feedbackTarget) {
        phase += feedbackToPhase(op.history[0] + op.history[1], params_.feedback);
    }
    const std::int32_t output = waves.sample(params_.operators[index].wave, tablePosition(phase)) * op.amplitude;
    op.history[1] = op.history[0];
    op.history[0] = output;
    return output;
}

std::int32_t Channel::renderAlgorithm(const WaveTables& waves) {
    if (params_.algorithm >= kAlgorithmCount) {
        return 0;
    }
    const Algorithm& algorithm = kAlgorithms[params_.algorithm];

    std::int64_t combined = 0;
    for (int g = 0; g < algorithm.groupCount; g++) {
        const Group& group = algorithm.groups[g];
        std::int64_t modulation = 0;
        std::int32_t stageSum = 0;
        for (int s = 0; s < group.stageCount; s++) {
            stageSum = 0;
            for (int op = 0; op < kOperatorCount; op++) {
                if (!(group.stages[s] & (1u << op))) {
                    continue;
                }
                const Operator& source = ops_[op];
                std::int64_t phase = (static_cast<std::int64_t>(source.turns) << 32) | source.phase;
                if (s > 0) {
                    phase = algorithm.directPhase ? modulation : phase + modulation;
                }
                stageSum += evaluateOperator(op, phase, waves);
            }
            modulation = modulationToPhase(stageSum);
        }
        if (g == 0) {
            combined = stageSum;
        } else if (algorithm.multiplyGroups) {
            // Q15 * Q15 >> 15, floor.
            combined = (combined * stageSum) >> kSignalFracBits;
        } else {
            combined += stageSum;
        }
    }
    return static_cast<std::int32_t>(combined);
}

}  // namespace hs816
