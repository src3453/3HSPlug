// One 8-operator harmonic channel: parameters, envelopes, phases, feedback
// history and the algorithm routing. Integer-only.
#pragma once

#include <array>
#include <cstdint>

#include "hs816/constants.hpp"
#include "hs816/envelope.hpp"

namespace hs816 {

class WaveTables;

struct OperatorParams {
    std::uint16_t ratio = 0;  // Q12; ignored for OP1
    std::uint8_t volume = 0;
    std::uint8_t wave = 0;
    EnvelopeRegisters envelope;
};

struct ChannelParams {
    std::uint16_t frequency = 0;  // OP1 frequency in Hz
    std::uint8_t algorithm = 0;
    std::uint8_t panLeft = 0;     // 0..15; both 0 means centered at full
    std::uint8_t panRight = 0;
    std::uint8_t feedback = 0;    // 128 is neutral
    std::uint8_t feedbackTarget = 0;  // operator index 0..7, others disable
    bool muted = false;
    std::array<OperatorParams, kOperatorCount> operators{};
};

class Channel {
public:
    Channel();

    // Back to power-on: params cleared, envelopes in Attack, gate off.
    void reset();

    const ChannelParams& params() const { return params_; }

    void setFrequency(std::uint16_t hz);
    void setOperatorRatio(int op, std::uint16_t ratio);
    void setOperatorVolume(int op, std::uint8_t volume);
    void setOperatorWave(int op, std::uint8_t wave);
    void setOperatorEnvelope(int op, const EnvelopeRegisters& envelope);
    void setAlgorithm(std::uint8_t algorithm) { params_.algorithm = algorithm; }
    void setPan(std::uint8_t left, std::uint8_t right);
    void setFeedback(std::uint8_t feedback) { params_.feedback = feedback; }
    void setFeedbackTarget(std::uint8_t op) { params_.feedbackTarget = op; }
    void setMuted(bool muted) { params_.muted = muted; }

    // Retrigger: envelopes to Attack, phases and feedback history to zero,
    // gate on. Valid while the gate is already on.
    void keyOn();
    // Release all envelopes if the gate is on.
    void keyOff();
    bool gateOn() const { return gateOn_; }

    // Advance one 48 kHz sample and return the algorithm output in Q15
    // (+-32640 for one full-scale operator; products and sums can exceed it).
    std::int32_t tick(const WaveTables& waves);

    // Effective pan gains in fifteenths (0..15).
    int panGainLeft() const { return panGainLeft_; }
    int panGainRight() const { return panGainRight_; }

private:
    struct Operator {
        Envelope envelope;
        EnvelopeRates rates;
        std::uint64_t phaseStep = 0;   // Q32 turns per sample
        std::uint32_t phase = 0;       // Q32 fraction of a turn
        std::uint32_t turns = 0;       // whole turns since key-on, saturating
        std::int32_t history[2] = {0, 0};  // last two outputs, newest first
        std::int32_t amplitude = 0;    // envelope-scaled volume, 0..255
    };

    void updatePhaseSteps();
    std::int32_t evaluateOperator(int op, std::int64_t phase, const WaveTables& waves);
    std::int32_t renderAlgorithm(const WaveTables& waves);

    ChannelParams params_;
    std::array<Operator, kOperatorCount> ops_{};
    int panGainLeft_ = 15;
    int panGainRight_ = 15;
    bool gateOn_ = false;
};

}  // namespace hs816
