#include "AlgorithmDiagram.h"

namespace ui {

namespace {

// 段の演算子を、番号の大きい順（信号の上流から）に並べる
juce::Array<int> stageOperators(std::uint8_t mask)
{
    juce::Array<int> ops;
    for (int op = 7; op >= 0; --op)
        if (mask & (1u << op))
            ops.add(op);
    return ops;
}

void drawArrow(juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, bool dashed, float thickness)
{
    const float headLen = juce::jmin(5.0f, (to.x - from.x) * 0.6f);
    juce::Path line;
    line.startNewSubPath(from);
    line.lineTo(to.translated(-headLen + 1.0f, 0.0f));
    juce::PathStrokeType stroke(thickness);
    if (dashed) {
        const float dashes[] = { 3.0f, 2.5f };
        juce::Path dashedLine;
        stroke.createDashedStroke(dashedLine, line, dashes, 2);
        g.fillPath(dashedLine);
    } else {
        g.strokePath(line, stroke);
    }
    juce::Path head;
    head.addTriangle(to, to.translated(-headLen, -3.0f), to.translated(-headLen, 3.0f));
    g.fillPath(head);
}

} // namespace

AlgorithmDiagram::AlgorithmDiagram()
{
    setInterceptsMouseClicks(true, false); // ツールチップのため
    updateTooltip();
}

void AlgorithmDiagram::setRouting(int newMode, int feedbackTarget, int feedbackAmount)
{
    newMode = juce::jlimit(0, algo::kAlgorithmCount - 1, newMode);
    if (newMode == mode && feedbackTarget == fbTarget && feedbackAmount == fbAmount)
        return;
    mode = newMode;
    fbTarget = feedbackTarget;
    fbAmount = feedbackAmount;
    updateTooltip();
    repaint();
}

void AlgorithmDiagram::updateTooltip()
{
    const auto& a = algo::get(mode);
    juce::String tip = "Signal flows left to right into Out. OP1 is the output end: "
                       "Yamaha diagrams number their operators the other way round.\n"
                       "Operators in one frame are summed and modulate every operator in the next frame.";
    if (a.directPhase)
        tip << "\nDashed arrows are direct phase: the later operators run on the modulation alone, not their own phase.";
    if (a.groupCount > 1)
        tip << (a.multiplyGroups ? "\nThe rows are multiplied (ring modulation)." : "\nThe rows are added.");
    tip << "\nThe loop marks the feedback destination (OP" << (fbTarget + 1) << "), which feeds its own output back into its phase"
        << (fbAmount == 128 ? "; feedback is 0, so it is off." : ".");
    setTooltip(tip);
}

void AlgorithmDiagram::paint(juce::Graphics& g)
{
    const auto& a = algo::get(mode);
    auto bounds = getLocalBounds().toFloat();

    g.setColour(colours::background);
    g.fillRoundedRectangle(bounds, shape::r(radius::data));

    auto area = bounds.reduced(8.0f, 4.0f);
    const float outW = 30.0f;
    const float combinerW = a.groupCount > 1 ? 26.0f : 0.0f;
    auto outArea = area.removeFromRight(outW);
    auto combinerArea = area.removeFromRight(combinerW);
    area.removeFromRight(10.0f);

    // 箱の大きさ: いちばん長い行が収まるように。狭いときは矢印と枠の余白から詰める。
    float arrowW = 14.0f, innerGap = 3.0f, framePad = 3.0f, boxW = 26.0f;
    const auto fitBox = [&] {
        float best = 26.0f;
        for (int gi = 0; gi < a.groupCount; ++gi) {
            const auto& grp = a.groups[gi];
            int ops = 0;
            for (int s = 0; s < grp.stageCount; ++s)
                ops += stageOperators(grp.stages[s]).size();
            const float fixed = (ops - grp.stageCount) * innerGap + grp.stageCount * 2.0f * framePad + (grp.stageCount - 1) * arrowW;
            best = juce::jmin(best, (area.getWidth() - fixed) / static_cast<float>(ops));
        }
        return best;
    };
    for (const float tryArrow : { 14.0f, 10.0f, 7.0f }) {
        arrowW = tryArrow;
        framePad = tryArrow >= 10.0f ? 3.0f : 1.5f;
        innerGap = tryArrow >= 10.0f ? 3.0f : 2.0f;
        boxW = fitBox();
        if (boxW >= 15.0f)
            break;
    }
    boxW = juce::jmax(9.0f, boxW);

    const float rowGap = 4.0f;
    const float loopH = 7.0f; // フィードバックのループを描く分
    const float rowH = juce::jmin(30.0f, (area.getHeight() - rowGap * (a.groupCount - 1)) / a.groupCount);
    const float boxH = juce::jmax(11.0f, rowH - loopH - 2.0f);
    const float totalH = a.groupCount * rowH + (a.groupCount - 1) * rowGap;
    const float top = area.getCentreY() - totalH * 0.5f;
    const std::uint8_t outputs = algo::outputMask(mode);
    const bool feedbackOn = fbAmount != 128;

    juce::Array<float> rowEndsY;
    const float rowsRight = area.getRight();

    for (int gi = 0; gi < a.groupCount; ++gi) {
        const auto& grp = a.groups[gi];
        // 行の幅を測って右揃え（出力が同じ位置にそろう）
        float width = 0.0f;
        for (int s = 0; s < grp.stageCount; ++s) {
            const int n = stageOperators(grp.stages[s]).size();
            width += n * boxW + (n - 1) * innerGap + 2.0f * framePad;
        }
        width += (grp.stageCount - 1) * arrowW;
        float x = rowsRight - width;
        const float rowTop = top + gi * (rowH + rowGap);
        const float boxY = rowTop + loopH + 1.0f;
        const float midY = boxY + boxH * 0.5f;

        for (int s = 0; s < grp.stageCount; ++s) {
            const auto ops = stageOperators(grp.stages[s]);
            const float frameW = ops.size() * boxW + (ops.size() - 1) * innerGap + 2.0f * framePad;
            const juce::Rectangle<float> frame(x, boxY - framePad, frameW, boxH + 2.0f * framePad);
            if (ops.size() > 1) {
                g.setColour(colours::fieldEdge);
                g.drawRoundedRectangle(frame, shape::r(radius::data), stroke::hairline);
            }
            float bx = x + framePad;
            for (int op : ops) {
                const juce::Rectangle<float> box(bx, boxY, boxW, boxH);
                const bool isOutput = (outputs >> op) & 1u;
                const auto role = isOutput ? colours::carrier : colours::modulator;
                g.setColour(colours::panelRaised);
                g.fillRoundedRectangle(box, shape::r(radius::cell));
                g.setColour(role);
                g.drawRoundedRectangle(box.reduced(0.5f), shape::r(radius::cell), stroke::selected);
                g.setColour(colours::text);
                g.setFont(font(juce::jmin(type::small, boxH - 1.0f), true));
                g.drawText(juce::String(op + 1), box, juce::Justification::centred, false);

                if (op == fbTarget) {
                    // 自分の出力を自分の位相へ戻すループ
                    juce::Path loop;
                    const float lx0 = box.getRight() - 3.0f, lx1 = box.getX() + 3.0f, ly = box.getY();
                    loop.startNewSubPath(lx0, ly);
                    loop.cubicTo(lx0, ly - loopH, lx1, ly - loopH, lx1, ly - 1.0f);
                    g.setColour(feedbackOn ? colours::text : colours::textDim);
                    g.strokePath(loop, juce::PathStrokeType(1.3f));
                    juce::Path head;
                    head.addTriangle(lx1, ly, lx1 - 3.0f, ly - 4.0f, lx1 + 3.0f, ly - 4.0f);
                    g.fillPath(head);
                }
                bx += boxW + innerGap;
            }
            x += frameW;
            if (s < grp.stageCount - 1) {
                g.setColour(colours::textDim);
                drawArrow(g, { x + 1.0f, midY }, { x + arrowW - 1.0f, midY }, a.directPhase, 1.3f);
                x += arrowW;
            }
        }
        rowEndsY.add(midY);
    }

    // 行をまとめて Out へ
    g.setColour(colours::textDim);
    const float outX = outArea.getX();
    if (a.groupCount == 1) {
        drawArrow(g, { rowsRight + 1.0f, rowEndsY[0] }, { outX - 2.0f, rowEndsY[0] }, false, 1.3f);
        g.setColour(colours::text);
        g.setFont(font(type::caption, true));
        g.drawText("Out", outArea.withY(rowEndsY[0] - 8.0f).withHeight(16.0f), juce::Justification::centredRight, false);
        return;
    }
    const float cx = combinerArea.getCentreX();
    const float cy = (rowEndsY.getFirst() + rowEndsY.getLast()) * 0.5f;
    const float r = 8.0f;
    for (float y : rowEndsY) {
        juce::Path wire;
        wire.startNewSubPath(rowsRight + 1.0f, y);
        wire.lineTo(cx, y);
        wire.lineTo(cx, y < cy ? cy - r : cy + r);
        g.strokePath(wire, juce::PathStrokeType(1.3f));
    }
    g.setColour(colours::panelRaised);
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    g.setColour(colours::textDim);
    g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, 1.2f);
    g.setColour(colours::text);
    // 掛け算は「×」を線で描く（フォントに頼らない）
    const float k = r * 0.45f;
    if (a.multiplyGroups) {
        g.drawLine(cx - k, cy - k, cx + k, cy + k, 1.5f);
        g.drawLine(cx - k, cy + k, cx + k, cy - k, 1.5f);
    } else {
        g.drawLine(cx - k, cy, cx + k, cy, 1.5f);
        g.drawLine(cx, cy - k, cx, cy + k, 1.5f);
    }
    g.setColour(colours::textDim);
    drawArrow(g, { cx + r, cy }, { outX - 2.0f, cy }, false, 1.3f);
    g.setColour(colours::text);
    g.setFont(font(type::caption, true));
    g.drawText("Out", outArea.withY(cy - 8.0f).withHeight(16.0f), juce::Justification::centredRight, false);
}

} // namespace ui
