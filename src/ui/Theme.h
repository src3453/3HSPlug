// 3HSPlug UI の配色・フォント・LookAndFeel
#pragma once

#include <JuceHeader.h>

namespace ui {

namespace colours {
inline const juce::Colour background   { 0xff15171c };
inline const juce::Colour panel        { 0xff1e2128 };
inline const juce::Colour panelRaised  { 0xff272b34 };
inline const juce::Colour outline      { 0xff343944 }; // 区画の縁や区切り（装飾）
inline const juce::Colour fieldEdge    { 0xff707888 }; // 操作できる部品の縁（パネルに対して3:1以上）
inline const juce::Colour text         { 0xffe4e7ec };
inline const juce::Colour textDim      { 0xff9aa2b0 }; // どの面に対しても4.5:1以上
inline const juce::Colour accent       { 0xffffa53c }; // 3HSのオレンジ。選択・状態・主な操作だけに使う
inline const juce::Colour accentDim    { 0xff7a5428 };
inline const juce::Colour focusRing    { 0xffffa53c };
inline const juce::Colour carrier      { 0xff4fc3f7 }; // キャリア（出力に出るOP）
inline const juce::Colour modulator    { 0xffc792ea }; // モジュレーター（アクセントと区別できる色相）
inline const juce::Colour meterGreen   { 0xff5fd068 };
inline const juce::Colour meterYellow  { 0xffffd54f };
inline const juce::Colour meterRed     { 0xffff5252 };
inline const juce::Colour meterRms     { 0x59e4e7ec }; // textの35%
inline const juce::Colour scrim        { 0x8c0b0c0f }; // 設定を開いたときの背景の暗幕
inline const juce::Colour lcdBack      { 0xff2a1d0c };
inline const juce::Colour lcdOn        { 0xffffb347 };
inline const juce::Colour lcdOff       { 0xff3d2b13 };

// MIDIチャンネルごとの色（1～16 → 0～15）
inline juce::Colour forChannel(int index0)
{
    return juce::Colour::fromHSV(static_cast<float>((index0 * 5) % 16) / 16.0f, 0.55f, 0.95f, 1.0f);
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

inline juce::Font font(float height, bool bold = false)
{
    return juce::Font(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font monoFont(float height)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

// キーボードで選んだ部品のまわりに描くフォーカスリング（すぐに出す）
inline void drawFocusRing(juce::Graphics& g, juce::Rectangle<float> r, float radius = 4.0f)
{
    g.setColour(colours::focusRing);
    g.drawRoundedRectangle(r.reduced(1.0f), radius, 2.0f);
}

// 区画の背景と見出しを描く。見出しの下の領域を返す。
inline juce::Rectangle<int> drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    g.setColour(colours::panel);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(colours::outline);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
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
    LookAndFeel()
    {
        auto scheme = juce::LookAndFeel_V4::getDarkColourScheme();
        scheme.setUIColour(ColourScheme::windowBackground, colours::background);
        scheme.setUIColour(ColourScheme::widgetBackground, colours::panelRaised);
        scheme.setUIColour(ColourScheme::menuBackground, colours::panelRaised);
        scheme.setUIColour(ColourScheme::outline, colours::outline);
        scheme.setUIColour(ColourScheme::defaultText, colours::text);
        scheme.setUIColour(ColourScheme::defaultFill, colours::accent);
        scheme.setUIColour(ColourScheme::highlightedText, colours::background);
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
        setColour(juce::TextButton::textColourOnId, colours::background);
        setColour(juce::ToggleButton::tickColourId, colours::accent);
        setColour(juce::ToggleButton::tickDisabledColourId, colours::textDim);
        setColour(juce::Label::textColourId, colours::text);
        setColour(juce::TextEditor::backgroundColourId, colours::panelRaised);
        setColour(juce::TextEditor::outlineColourId, colours::fieldEdge);
        setColour(juce::TextEditor::focusedOutlineColourId, colours::accent);
        setColour(juce::PopupMenu::backgroundColourId, colours::panelRaised);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colours::accent);
        setColour(juce::PopupMenu::highlightedTextColourId, colours::background);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float startAngle, float endAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(3.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float lineW = juce::jmax(2.0f, radius * 0.16f);
        const float arcRadius = radius - lineW * 0.5f;
        const bool enabled = slider.isEnabled();
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (!enabled)
            fill = colours::textDim.withAlpha(0.4f);

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId).withMultipliedAlpha(enabled ? 1.0f : 0.5f));
        g.strokePath(track, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 0をまたぐ範囲（Fine、Key shift）は中央から弧を描く
        float fromAngle = startAngle;
        if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
            fromAngle = startAngle + static_cast<float>(slider.valueToProportionOfLength(0.0)) * (endAngle - startAngle);
        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            juce::jmin(fromAngle, angle), juce::jmax(fromAngle, angle), true);
        g.setColour(fill);
        g.strokePath(value, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float knobR = arcRadius - lineW * 1.1f;
        g.setColour(colours::panelRaised.brighter(slider.isMouseOverOrDragging() ? 0.15f : 0.0f));
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
        g.fillRoundedRectangle(track, 2.0f);
        g.setColour(colours::accent);
        g.fillRoundedRectangle(track.withTop(sliderPos), 2.0f);
        auto thumb = juce::Rectangle<float>(juce::jmin(bounds.getWidth(), 30.0f), 12.0f).withCentre({ bounds.getCentreX(), sliderPos });
        g.setColour(colours::panelRaised.brighter(0.3f));
        g.fillRoundedRectangle(thumb, 3.0f);
        g.setColour(colours::text);
        g.drawLine(thumb.getX() + 4.0f, sliderPos, thumb.getRight() - 4.0f, sliderPos, 1.5f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool highlighted, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto colour = button.getToggleState() ? button.findColour(juce::TextButton::buttonOnColourId) : backgroundColour;
        if (down)
            colour = colour.brighter(0.2f);
        else if (highlighted)
            colour = colour.brighter(0.08f);
        g.setColour(colour);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(colours::fieldEdge);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
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

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        label->setFont(monoFont(type::small));
        label->setColour(juce::Label::outlineWhenEditingColourId, colours::accent);
        return label;
    }
};

} // namespace ui
