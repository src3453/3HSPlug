// アルゴリズム（モジュレーションモード）の信号経路図
//
// 3HS816コアの経路表（src/s3hs_core/3hs/3hs816/src/channel.cpp の kAlgorithms）の写し。
// コアは無名名前空間に置いているので、ここに同じ内容を持つ。コアを更新したら合わせること
// （test/algorithm_table_check.py で一致を確かめる）。
//
// 番号の向きはYamahaの図と逆: OP1が出力側で、番号が大きいほどモジュレーター側にある。
#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include "Theme.h"

namespace ui {

namespace algo {

// コアと同じ形: グループは段の列で、段はその演算子（bit i = OP(i+1)）の和を次の段へ位相変調で渡す。
// 最後の段がグループの出力。グループの出力は足すか（multiplyGroups なら）掛ける。
// directPhase では2段目以降が自分の位相を使わず、変調だけを位相にする。
struct Group {
    int stageCount;
    std::uint8_t stages[8];
};

struct Algorithm {
    bool multiplyGroups;
    bool directPhase;
    int groupCount;
    Group groups[4];
};

constexpr int kAlgorithmCount = 13;

// --- kAlgorithms (copied from hs816) ----------------------------------------
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
// ----------------------------------------------------------------------------

inline const Algorithm& get(int mode)
{
    return kAlgorithms[juce::jlimit(0, kAlgorithmCount - 1, mode)];
}

// 出力に出る演算子（各グループの最後の段）
inline std::uint8_t outputMask(int mode)
{
    const auto& a = get(mode);
    std::uint8_t mask = 0;
    for (int g = 0; g < a.groupCount; ++g)
        mask |= a.groups[g].stages[a.groups[g].stageCount - 1];
    return mask;
}

} // namespace algo

// 信号は左から右へ流れ、右端の Out で終わる。枠で囲んだ演算子は足し合わせる。
class AlgorithmDiagram : public juce::Component, public juce::SettableTooltipClient
{
public:
    AlgorithmDiagram();
    void setRouting(int mode, int feedbackTarget, int feedbackAmount);
    void paint(juce::Graphics&) override;

private:
    void updateTooltip();

    int mode = 4;
    int fbTarget = 0;
    int fbAmount = 128;
};

} // namespace ui
