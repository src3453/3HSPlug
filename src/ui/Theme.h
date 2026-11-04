// 3HSPlug UI の配色・フォント・LookAndFeel
#pragma once

#include <JuceHeader.h>
#include "Fonts.h"
#include "Themes.h"

namespace ui {

// 色と形はテーマ（Themes.h）で差し替える。ここにあるのは現在のテーマの値で、描画のたびに読む。
// 初期値は既定のテーマ「Midnight」と同じ。
namespace colours {
inline juce::Colour background   { 0xff15171c };
inline juce::Colour panel        { 0xff1e2128 };
inline juce::Colour panelRaised  { 0xff272b34 };
inline juce::Colour outline      { 0xff343944 }; // 区画の縁や区切り（装飾）
inline juce::Colour fieldEdge    { 0xff707888 }; // 操作できる部品の縁（パネルに対して3:1以上）
inline juce::Colour text         { 0xffe4e7ec };
inline juce::Colour textDim      { 0xff9aa2b0 }; // どの面に対しても4.5:1以上
inline juce::Colour accent       { 0xffffa53c }; // 選択・状態・主な操作だけに使う
inline juce::Colour accentDim    { 0xff7a5428 };
inline juce::Colour onAccent     { 0xff15171c }; // アクセントの上に載せる文字
inline juce::Colour focusRing    { 0xffffa53c };
inline juce::Colour carrier      { 0xff4fc3f7 }; // キャリア（出力に出るOP）
inline juce::Colour modulator    { 0xffc792ea }; // モジュレーター（アクセントと区別できる色相）
inline juce::Colour meterGreen   { 0xff5fd068 };
inline juce::Colour meterYellow  { 0xffffd54f };
inline juce::Colour meterRed     { 0xffff5252 };
inline juce::Colour meterRms     { 0x59e4e7ec };
inline juce::Colour scrim        { 0x8c0b0c0f }; // 設定を開いたときの背景の暗幕
inline juce::Colour lcdBack      { 0xff2a1d0c };
inline juce::Colour lcdOn        { 0xffffb347 };
inline juce::Colour lcdOff       { 0xff3d2b13 };
} // namespace colours

// 見た目の形（テーマで変わる）
namespace shape {
enum class Knob { arc, pointer, filled };   // 弧で値を示す / 指針だけ / 太い弧と小さな点
enum class Button { filled, outline };      // 塗りのボタン / 枠だけのボタン
inline float roundness = 1.0f;              // 角の丸み（0 で角、1 が標準）
inline Knob knob = Knob::arc;
inline Button button = Button::filled;
inline bool sectionBorders = true;          // 区画の枠線
inline bool titleGradient = true;           // タイトルバーのグラデーション
inline bool monochromeChannels = false;     // MIDIチャンネルの色をアクセントの濃淡にする
inline float channelSaturation = 0.55f;
inline float channelBrightness = 0.95f;

inline float r(float base) { return base * roundness; } // 角の半径
} // namespace shape

namespace colours {
// MIDIチャンネルごとの色（1～16 → 0～15）
inline juce::Colour forChannel(int index0)
{
    const float step = static_cast<float>((index0 * 5) % 16) / 15.0f;
    if (shape::monochromeChannels)
        return accent.withMultipliedBrightness(0.55f + 0.45f * step);
    return juce::Colour::fromHSV(static_cast<float>((index0 * 5) % 16) / 16.0f, shape::channelSaturation,
                                 shape::channelBrightness, 1.0f);
}
} // namespace colours

// 文字サイズ（13pxを本文とし、約1.2倍刻み。機能的な文字は11px以上）
namespace type {
inline constexpr float caption = 11.0f;
inline constexpr float small   = 12.0f;
inline constexpr float body    = 13.0f;
inline constexpr float title   = 15.5f;
inline constexpr float heading = 18.5f;
inline constexpr float display = 27.0f;
} // namespace type

// 余白（4px刻み）
namespace space {
inline constexpr int xs = 4;
inline constexpr int sm = 8;
inline constexpr int md = 12;
inline constexpr int lg = 16;
inline constexpr int xl = 24;
inline constexpr int sectionHeader = 24; // 区画の見出しの高さ
inline constexpr int control = 32;       // ボタン・コンボボックスの高さ
} // namespace space

// UIの文字はすべてここを通す。同梱フォントを選んでいるときは、数字もそのフォントで描く
// （同梱フォントは Regular だけなので、太字の指定はそのまま無視される）。
inline juce::Font font(float height, bool bold = false)
{
    if (auto typeface = customTypeface())
        return juce::Font(juce::FontOptions(typeface).withHeight(height));
    return juce::Font(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font monoFont(float height)
{
    if (auto typeface = customTypeface())
        return juce::Font(juce::FontOptions(typeface).withHeight(height));
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

// ラベルのフォントを設定し、フォントを切り替えたときに作り直せるよう大きさと種類を覚えておく
inline void setLabelFont(juce::Label& label, float height, bool bold = false, bool mono = false)
{
    auto& props = label.getProperties();
    props.set("uiFontHeight", height);
    props.set("uiFontBold", bold);
    props.set("uiFontMono", mono);
    label.setFont(mono ? monoFont(height) : font(height, bold));
}

// キーボードで選んだ部品のまわりに描くフォーカスリング（すぐに出す）
inline void drawFocusRing(juce::Graphics& g, juce::Rectangle<float> r, float radius = 4.0f)
{
    g.setColour(colours::focusRing);
    g.drawRoundedRectangle(r.reduced(1.0f), shape::r(radius), 2.0f);
}

// 区画の背景と見出しを描く。見出しの下の領域を返す。
inline juce::Rectangle<int> drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    g.setColour(colours::panel);
    g.fillRoundedRectangle(bounds.toFloat(), shape::r(6.0f));
    if (shape::sectionBorders) {
        g.setColour(colours::outline);
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), shape::r(6.0f), 1.0f);
    }
    if (title.isEmpty())
        return bounds.reduced(space::sm);
    auto header = bounds.removeFromTop(space::sectionHeader);
    g.setColour(colours::textDim);
    g.setFont(font(type::small, true));
    g.drawText(title, header.reduced(space::md, 0), juce::Justification::centredLeft);
    return bounds.reduced(space::sm, space::xs);
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel() { applyTheme(); }

    // 現在のテーマ（colours::）の色を LookAndFeel に入れ直す。テーマを切り替えたら呼ぶ。
    void applyTheme()
    {
        auto scheme = juce::LookAndFeel_V4::getDarkColourScheme();
        scheme.setUIColour(ColourScheme::windowBackground, colours::background);
        scheme.setUIColour(ColourScheme::widgetBackground, colours::panelRaised);
        scheme.setUIColour(ColourScheme::menuBackground, colours::panelRaised);
        scheme.setUIColour(ColourScheme::outline, colours::outline);
        scheme.setUIColour(ColourScheme::defaultText, colours::text);
        scheme.setUIColour(ColourScheme::defaultFill, colours::accent);
        scheme.setUIColour(ColourScheme::highlightedText, colours::onAccent);
        scheme.setUIColour(ColourScheme::highlightedFill, colours::accent);
        scheme.setUIColour(ColourScheme::menuText, colours::text);
        setColourScheme(scheme);

        setColour(juce::ResizableWindow::backgroundColourId, colours::background);
        setColour(juce::Slider::rotarySliderFillColourId, colours::accent);
        setColour(juce::Slider::rotarySliderOutlineColourId, colours::fieldEdge);
        setColour(juce::Slider::thumbColourId, colours::text);
        setColour(juce::Slider::trackColourId, colours::accent);
        setColour(juce::Slider::backgroundColourId, colours::fieldEdge);
        setColour(juce::Slider::textBoxTextColourId, colours::text);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxHighlightColourId, colours::accentDim);
        setColour(juce::ComboBox::backgroundColourId, colours::panelRaised);
        setColour(juce::ComboBox::outlineColourId, colours::fieldEdge);
        setColour(juce::ComboBox::focusedOutlineColourId, colours::focusRing);
        setColour(juce::ComboBox::textColourId, colours::text);
        setColour(juce::ComboBox::arrowColourId, colours::textDim);
        setColour(juce::TextButton::buttonColourId, colours::panelRaised);
        setColour(juce::TextButton::buttonOnColourId, colours::accent);
        setColour(juce::TextButton::textColourOffId, colours::text);
        setColour(juce::TextButton::textColourOnId, colours::onAccent);
        setColour(juce::ToggleButton::tickColourId, colours::accent);
        setColour(juce::ToggleButton::tickDisabledColourId, colours::textDim);
        setColour(juce::Label::textColourId, colours::text);
        setColour(juce::TextEditor::backgroundColourId, colours::panelRaised);
        setColour(juce::TextEditor::outlineColourId, colours::fieldEdge);
        setColour(juce::TextEditor::focusedOutlineColourId, colours::accent);
        setColour(juce::PopupMenu::backgroundColourId, colours::panelRaised);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colours::accent);
        setColour(juce::PopupMenu::highlightedTextColourId, colours::onAccent);
        setColour(juce::Label::textWhenEditingColourId, colours::text);
        setColour(juce::TextEditor::textColourId, colours::text);
        setColour(juce::TextEditor::highlightColourId, colours::accentDim);
        setColour(juce::CaretComponent::caretColourId, colours::accent);
        setColour(juce::AlertWindow::backgroundColourId, colours::panel);
        setColour(juce::AlertWindow::textColourId, colours::text);
        setColour(juce::AlertWindow::outlineColourId, colours::fieldEdge);
        setColour(juce::ToggleButton::textColourId, colours::text);
        setColour(juce::TabbedComponent::backgroundColourId, colours::background);
        setColour(juce::TabbedComponent::outlineColourId, colours::outline);
        setColour(juce::TabbedButtonBar::tabOutlineColourId, colours::fieldEdge);
        setColour(juce::TabbedButtonBar::tabTextColourId, colours::text);
        setColour(juce::TabbedButtonBar::frontTextColourId, colours::accent);
    }

    int getTabButtonOverlap(int) override { return 0; }
    int getTabButtonSpaceAroundImage() override { return 0; }

    juce::Font getTabButtonFont(juce::TabBarButton&, float height) override
    {
        return font(juce::jmin(type::body, height * 0.55f), true);
    }

    int getTabButtonBestWidth(juce::TabBarButton& button, int depth) override
    {
        return juce::jmax(depth * 2, juce::GlyphArrangement::getStringWidthInt(
            getTabButtonFont(button, static_cast<float>(depth)), button.getButtonText()) + space::lg * 2);
    }

    void drawTabbedButtonBarBackground(juce::TabbedButtonBar&, juce::Graphics& g) override
    {
        g.fillAll(colours::background);
    }

    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar&, juce::Graphics&, int, int) override {}

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool highlighted, bool down) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        const bool selected = button.isFrontTab();
        const bool outline = shape::button == shape::Button::outline;
        const float alpha = button.isEnabled() ? 1.0f : 0.5f;
        auto fill = selected ? colours::accent : colours::panelRaised;
        if (outline)
            fill = (selected ? colours::accent : colours::text).withAlpha(down ? 0.22f : 0.10f);
        else if (down || highlighted)
            fill = fill.brighter(down ? 0.2f : 0.08f);
        if (!outline || selected || highlighted || down) {
            g.setColour(fill.withMultipliedAlpha(alpha));
            g.fillRoundedRectangle(bounds, shape::r(4.0f));
        }
        g.setColour((selected ? colours::accent : colours::fieldEdge).withMultipliedAlpha(alpha));
        g.drawRoundedRectangle(bounds, shape::r(4.0f), selected ? 1.5f : 1.0f);
        g.setColour((selected ? (outline ? colours::accent : colours::onAccent) : colours::text).withMultipliedAlpha(alpha));
        g.setFont(getTabButtonFont(button, static_cast<float>(button.getHeight())));
        g.drawFittedText(button.getButtonText(), bounds.toNearestInt().reduced(8, 2), juce::Justification::centred, 1);
        if (button.hasKeyboardFocus(false))
            drawFocusRing(g, bounds.reduced(2.0f));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float startAngle, float endAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(3.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const bool enabled = slider.isEnabled();
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (!enabled)
            fill = colours::textDim.withAlpha(0.4f);
        const auto trackColour = slider.findColour(juce::Slider::rotarySliderOutlineColourId).withMultipliedAlpha(enabled ? 1.0f : 0.5f);
        const bool hot = slider.isMouseOverOrDragging();

        // 0をまたぐ範囲（Fine、Key shift、Feedback）は中央から弧を描く
        float fromAngle = startAngle;
        if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
            fromAngle = startAngle + static_cast<float>(slider.valueToProportionOfLength(0.0)) * (endAngle - startAngle);
        const auto arc = [&](float r, float from, float to, float w, juce::Colour c) {
            juce::Path p;
            p.addCentredArc(centre.x, centre.y, r, r, 0.0f, juce::jmin(from, to), juce::jmax(from, to), true);
            g.setColour(c);
            g.strokePath(p, juce::PathStrokeType(w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };

        if (shape::knob == shape::Knob::pointer) {
            // 細い目盛りの輪と、値の色の指針
            arc(radius - 1.5f, startAngle, endAngle, 1.5f, trackColour);
            const float bodyR = radius - 4.0f;
            g.setColour(colours::panelRaised.brighter(hot ? 0.15f : 0.0f));
            g.fillEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
            g.setColour(trackColour);
            g.drawEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);
            g.setColour(fill);
            g.drawLine({ centre.getPointOnCircumference(bodyR * 0.2f, angle), centre.getPointOnCircumference(bodyR + 2.0f, angle) },
                       juce::jmax(2.0f, radius * 0.12f));
            return;
        }

        const float lineW = juce::jmax(2.0f, radius * (shape::knob == shape::Knob::filled ? 0.30f : 0.16f));
        const float arcRadius = radius - lineW * 0.5f;
        arc(arcRadius, startAngle, endAngle, lineW, trackColour);
        arc(arcRadius, fromAngle, angle, lineW, fill);

        if (shape::knob == shape::Knob::filled) {
            // 太い弧の内側に小さな点だけ
            const float dotR = juce::jmax(2.0f, radius * 0.12f);
            const auto dot = centre.getPointOnCircumference(arcRadius - lineW * 0.9f, angle);
            g.setColour(enabled ? colours::text : colours::textDim);
            g.fillEllipse(dot.x - dotR, dot.y - dotR, dotR * 2.0f, dotR * 2.0f);
            return;
        }

        const float knobR = arcRadius - lineW * 1.1f;
        g.setColour(colours::panelRaised.brighter(hot ? 0.15f : 0.0f));
        g.fillEllipse(centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
        juce::Point<float> tip(centre.x + (knobR - 1.5f) * std::sin(angle), centre.y - (knobR - 1.5f) * std::cos(angle));
        g.setColour(enabled ? colours::text : colours::textDim);
        g.drawLine({ centre.getPointOnCircumference(knobR * 0.25f, angle), tip }, 2.0f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical) {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            return;
        }
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto track = bounds.withSizeKeepingCentre(4.0f, bounds.getHeight());
        g.setColour(colours::fieldEdge);
        g.fillRoundedRectangle(track, shape::r(2.0f));
        g.setColour(colours::accent);
        g.fillRoundedRectangle(track.withTop(sliderPos), shape::r(2.0f));
        auto thumb = juce::Rectangle<float>(juce::jmin(bounds.getWidth(), 30.0f), 12.0f).withCentre({ bounds.getCentreX(), sliderPos });
        g.setColour(colours::panelRaised.brighter(0.3f));
        g.fillRoundedRectangle(thumb, shape::r(3.0f));
        g.setColour(colours::text);
        g.drawLine(thumb.getX() + 4.0f, sliderPos, thumb.getRight() - 4.0f, sliderPos, 1.5f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool highlighted, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool on = button.getToggleState();
        if (shape::button == shape::Button::outline) {
            // 枠だけ。押したとき・マウスを載せたときは面をうっすら塗る
            if (down || highlighted || on) {
                g.setColour((on ? colours::accent : colours::text).withAlpha(down ? 0.22f : 0.10f));
                g.fillRoundedRectangle(bounds, shape::r(4.0f));
            }
            g.setColour(on ? colours::accent : colours::fieldEdge);
            g.drawRoundedRectangle(bounds, shape::r(4.0f), on ? 1.5f : 1.0f);
            return;
        }
        auto colour = on ? button.findColour(juce::TextButton::buttonOnColourId) : backgroundColour;
        if (down)
            colour = colour.brighter(0.2f);
        else if (highlighted)
            colour = colour.brighter(0.08f);
        g.setColour(colour);
        g.fillRoundedRectangle(bounds, shape::r(4.0f));
        g.setColour(colours::fieldEdge);
        g.drawRoundedRectangle(bounds, shape::r(4.0f), 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool highlighted, bool down) override
    {
        if (shape::button == shape::Button::outline && button.getToggleState()) {
            g.setFont(getTextButtonFont(button, button.getHeight()));
            g.setColour(colours::accent.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
            g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 2), juce::Justification::centred, 1);
            return;
        }
        juce::LookAndFeel_V4::drawButtonText(g, button, highlighted, down);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return font(juce::jmin(type::body, buttonHeight * 0.55f), true);
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        return font(juce::jmin(type::body, box.getHeight() * 0.6f));
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        return label.getFont();
    }

    juce::Font getPopupMenuFont() override { return font(type::body); }
    juce::Font getAlertWindowTitleFont() override { return font(type::title, true); }
    juce::Font getAlertWindowMessageFont() override { return font(type::body); }
    juce::Font getAlertWindowFont() override { return font(type::body); }

    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override
    {
        const auto layout = tooltipLayout(tipText, juce::Colours::black);
        const auto w = static_cast<int>(layout.getWidth() + 14.0f);
        const auto h = static_cast<int>(layout.getHeight() + 6.0f);
        return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                    screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
            .constrainedWithin(parentArea);
    }

    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        const juce::Rectangle<int> bounds(width, height);
        g.setColour(colours::panelRaised);
        g.fillRect(bounds.toFloat());
        g.setColour(colours::fieldEdge);
        g.drawRect(bounds.toFloat(), 1.0f);
        tooltipLayout(text, colours::text).draw(g, bounds.toFloat().reduced(7.0f, 3.0f));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down) override
    {
        const float box = juce::jmin(20.0f, button.getHeight() * 0.7f);
        drawTickBox(g, button, 4.0f, (button.getHeight() - box) * 0.5f, box, box, button.getToggleState(),
                    button.isEnabled(), highlighted, down);
        g.setColour(button.findColour(juce::ToggleButton::textColourId).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
        g.setFont(font(type::body));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(juce::roundToInt(box) + 10).withTrimmedRight(2),
                         juce::Justification::centredLeft, 2);
    }

private:
    static juce::TextLayout tooltipLayout(const juce::String& text, juce::Colour colour)
    {
        juce::AttributedString s;
        s.setJustification(juce::Justification::centredLeft);
        s.append(text, font(type::small), colour);
        juce::TextLayout layout;
        layout.createLayout(s, 400.0f);
        return layout;
    }

public:

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        setLabelFont(*label, type::small, false, true);
        label->setColour(juce::Label::outlineWhenEditingColourId, colours::accent);
        return label;
    }
};

} // namespace ui
