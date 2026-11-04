#pragma once
#include <algorithm>
#include <cstddef>

namespace ui {
// マルチパス最大値トリガー（fluentscope の trigger_waveform と同じ処理）。
//   length = N/2, half = length/2, trigger = N - half, search_start = max(0, trigger - half)
//   Pass1: [search_start, trigger] の最大値を求める
//   Pass2: trigger から後ろ向きに、最大値*(1-tolerance) 以上になる最初のサンプルへ移動
//   Pass3: そこから後ろ向きに、全体の平均値以下になる点へ移動（最大 half サンプル）
// 戻り値は、トリガー位置を中心にした [start, end) の先頭インデックス。
// 表示するサンプル数は outCount に返す（最大 2*half）。
inline size_t findScopeTrigger(const float* w, size_t n, size_t& outCount, float tolerance = 0.04f)
{
    outCount = 0;
    if (n == 0) return 0;
    const size_t half = (n / 2) / 2;
    size_t trigger = n - half;
    const size_t searchStart = trigger > half ? trigger - half : 0;
    if (trigger >= n) trigger = n - 1;

    // Pass1
    float maximum = w[searchStart];
    for (size_t i = searchStart; i <= trigger; ++i) maximum = std::max(maximum, w[i]);

    // Pass2
    const float threshold = maximum * (1.0f - tolerance);
    size_t best = trigger;
    for (size_t i = trigger + 1; i-- > searchStart;) {
        if (w[i] >= threshold) { best = i; break; }
    }
    trigger = best;

    // Pass3
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) sum += w[i];
    const float average = static_cast<float>(sum / static_cast<double>(n));
    const size_t zeroEnd = trigger > half ? trigger - half : 0;
    for (size_t i = trigger + 1; i-- > zeroEnd;) {
        if (w[i] <= average) { trigger = i; break; }
    }

    const size_t start = trigger > half ? trigger - half : 0;
    const size_t end = std::min(trigger + half, n);
    outCount = end - start;
    return start;
}
} // namespace ui
