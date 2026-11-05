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
inline juce::Colour keyWhite     { 0xffe4e7ec }; // 鍵盤の絵（テーマの明暗によらず白鍵に見える色）
inline juce::Colour keyBlack     { 0xff20232a };

// 状態を示す色。メーターの色を共有するが、状態のコードではこの名前を使う
inline juce::Colour& success = meterGreen;
inline juce::Colour& warning = meterYellow;
inline juce::Colour& error   = meterRed;
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

// 角の丸みの段階。必ず shape::r() を通す。
namespace radius {
inline constexpr float fine    = 1.0f; // メーターのセグメント
inline constexpr float cell    = 2.0f; // 小さなセル、細いトラック
inline constexpr float data    = 3.0f; // 計器窓、ドラムのセル、チェックボックス、LCD
inline constexpr float control = 4.0f; // ボタン、タブ、コンボボックス、入力欄、選択枠
inline constexpr float section = 6.0f; // 区画
inline constexpr float overlay = 8.0f; // 設定、Options、確認ダイアログ
} // namespace radius

// 線の太さ
namespace stroke {
inline constexpr float hairline = 1.0f;
inline constexpr float selected = 1.5f;
inline constexpr float focus    = 2.0f;
} // namespace stroke

// 状態を表す不透明度（値そのものを濃さで示す描画は対象外）
namespace alpha {
inline constexpr float selected = 0.12f; // 選択中の面
inline constexpr float hover    = 0.10f; // マウスを載せたときに重ねる色
inline constexpr float down     = 0.20f; // 押したときに重ねる色
inline constexpr float disabled = 0.50f; // 使えない部品
inline constexpr float flash    = 0.60f; // 鳴った・変わった瞬間（ここから消えていく）
inline constexpr float wash     = 0.50f; // 面をうっすら分ける
inline constexpr float voiceFill = 0.30f; // ボイスのセルの塗りの上限（上に文字が載る）
} // namespace alpha

// 操作の状態。brighter() は明るいテーマの白い面で効かないので、文字の色を重ねる。
namespace state {
inline juce::Colour hover(juce::Colour c) { return c.overlaidWith(colours::text.withAlpha(alpha::hover)); }
inline juce::Colour down(juce::Colour c)  { return c.overlaidWith(colours::text.withAlpha(alpha::down)); }
} // namespace state

namespace colours {
// MIDIチャンネルごとのテーマの色（1～16 → 0～15）。表示には forChannel() を使う。
inline juce::Colour rawChannelColour(int index0)
{
    const float step = static_cast<float>((index0 * 5) % 16) / 15.0f;
    if (shape::monochromeChannels)
        return accent.withMultipliedBrightness(0.7f + 0.3f * step); // 文字にも使うので暗くしすぎない
    return juce::Colour::fromHSV(static_cast<float>((index0 * 5) % 16) / 16.0f, shape::channelSaturation,
                                 shape::channelBrightness, 1.0f);
}

// 文字にも使うので、テーマを適用するときに面に対して読める色へ整える（Themes.cpp）
inline std::array<juce::Colour, 16> channelTable;
inline juce::Colour forChannel(int index0) { return channelTable[static_cast<size_t>(index0 & 15)]; }
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

// ラベルのフォントを設定し、テーマやフォントを切り替えたときに作り直せるよう種類を覚えておく。
// dim は控えめな文字（textDim）。色を setColour で固定すると、テーマを変えても古い色のまま残る。
inline void setLabelFont(juce::Label& label, float height, bool bold = false, bool mono = false, bool dim = false)
{
    auto& props = label.getProperties();
    props.set("uiFontHeight", height);
    props.set("uiFontBold", bold);
    props.set("uiFontMono", mono);
    props.set("uiFontDim", dim);
    label.setFont(mono ? monoFont(height) : font(height, bold));
    if (dim)
        label.setColour(juce::Label::textColourId, colours::textDim);
}

// 入力欄のフォントを設定する。TextEditor は入力済みの文字の色とフォントを覚えるので、
// 切り替えのときに作り直せるようここを通す。
inline void setEditorFont(juce::TextEditor& editor, float height, bool mono = true)
{
    auto& props = editor.getProperties();
    props.set("uiFontHeight", height);
    props.set("uiFontMono", mono);
    editor.applyFontToAllText(mono ? monoFont(height) : font(height));
}

// キーボードで選んだ部品のまわりに描くフォーカスリング（すぐに出す）
inline void drawFocusRing(juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius = radius::control)
{
    g.setColour(colours::focusRing);
    g.drawRoundedRectangle(r.reduced(1.0f), shape::r(cornerRadius), stroke::focus);
}

// 選択中のセル・行・列の印
inline void drawSelection(juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius = radius::control)
{
    g.setColour(colours::accent.withAlpha(alpha::selected));
    g.fillRoundedRectangle(r, shape::r(cornerRadius));
    g.setColour(colours::accent);
    g.drawRoundedRectangle(r.reduced(stroke::selected * 0.5f), shape::r(cornerRadius), stroke::selected);
}

// UIスケールがかかった画面全体。ここを親にしたポップアップはテーマとスケールを受け継ぐ。
inline constexpr const char* kRootComponentId = "mainUI";

inline juce::Component& uiRoot(juce::Component& from)
{
    for (auto* c = &from; c != nullptr; c = c->getParentComponent())
        if (c->getComponentID() == kRootComponentId)
            return *c;
    return *from.getTopLevelComponent();
}

// 元に戻せない操作の確認。AlertWindow::showAsync は親のない窓を作り、JUCE既定の LookAndFeel で
// 描いてしまうので、画面の中の子として開く。
inline void showConfirm(juce::Component& from, const juce::String& title, const juce::String& message,
                        const juce::String& action, std::function<void()> onConfirm)
{
    auto& root = uiRoot(from);
    auto* window = new juce::AlertWindow(title, message, juce::MessageBoxIconType::WarningIcon);
    window->setOpaque(false); // 角を丸めるので下の画面が見える
    root.addChildComponent(window);
    window->sendLookAndFeelChange();
    window->addButton(action, 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    window->setCentrePosition(root.getLocalBounds().getCentre());
    const juce::Component::SafePointer<juce::Component> safe(&from);
    window->enterModalState(true, juce::ModalCallbackFunction::create([safe, onConfirm = std::move(onConfirm)](int result) {
        if (safe != nullptr && result == 1)
            onConfirm();
    }), true);
}

// 区画の背景と見出しを描く。見出しの下の領域を返す。
inline juce::Rectangle<int> drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    g.setColour(colours::panel);
    g.fillRoundedRectangle(bounds.toFloat(), shape::r(radius::section));
    if (shape::sectionBorders) {
        g.setColour(colours::outline);
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), shape::r(radius::section), stroke::hairline);
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
        setColour(juce::TextEditor::focusedOutlineColourId, colours::focusRing);
        setColour(juce::PopupMenu::backgroundColourId, colours::panelRaised);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colours::accent);
        setColour(juce::PopupMenu::highlightedTextColourId, colours::onAccent);
        setColour(juce::Label::textWhenEditingColourId, colours::text);
        setColour(juce::TextEditor::textColourId, colours::text);
        setColour(juce::TextEditor::highlightColourId, colours::accentDim);
        setColour(juce::TextEditor::highlightedTextColourId, colours::text);
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
        setColour(juce::ScrollBar::thumbColourId, colours::fieldEdge); // JUCEの既定はアクセント
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
        const float opacity = button.isEnabled() ? 1.0f : alpha::disabled;
        auto fill = selected ? colours::accent : colours::panelRaised;
        if (outline)
            fill = (selected ? colours::accent : colours::text).withAlpha(down ? alpha::down : alpha::hover);
        else if (down || highlighted)
            fill = down ? state::down(fill) : state::hover(fill);
        if (!outline || selected || highlighted || down) {
            g.setColour(fill.withMultipliedAlpha(opacity));
            g.fillRoundedRectangle(bounds, shape::r(radius::control));
        }
        g.setColour((selected ? colours::accent : colours::fieldEdge).withMultipliedAlpha(opacity));
        g.drawRoundedRectangle(bounds, shape::r(radius::control), selected ? stroke::selected : stroke::hairline);
        g.setColour((selected ? (outline ? colours::accent : colours::onAccent) : colours::text).withMultipliedAlpha(opacity));
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
            fill = colours::textDim.withAlpha(alpha::disabled);
        const auto trackColour = slider.findColour(juce::Slider::rotarySliderOutlineColourId).withMultipliedAlpha(enabled ? 1.0f : alpha::disabled);
        const auto bodyColour = slider.isMouseOverOrDragging() ? state::hover(colours::panelRaised) : colours::panelRaised;

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
            g.setColour(bodyColour);
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
        g.setColour(bodyColour);
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
        g.fillRoundedRectangle(track, shape::r(radius::cell));
        g.setColour(colours::accent);
        g.fillRoundedRectangle(track.withTop(sliderPos), shape::r(radius::cell));
        auto thumb = juce::Rectangle<float>(juce::jmin(bounds.getWidth(), 30.0f), 12.0f).withCentre({ bounds.getCentreX(), sliderPos });
        g.setColour(state::down(colours::panelRaised));
        g.fillRoundedRectangle(thumb, shape::r(radius::data));
        g.setColour(colours::fieldEdge);
        g.drawRoundedRectangle(thumb.reduced(0.5f), shape::r(radius::data), stroke::hairline);
        g.setColour(colours::text);
        g.drawLine(thumb.getX() + 4.0f, sliderPos, thumb.getRight() - 4.0f, sliderPos, 1.5f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool highlighted, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool on = button.getToggleState();
        const float opacity = button.isEnabled() ? 1.0f : alpha::disabled;
        if (shape::button == shape::Button::outline) {
            // 枠だけ。押したとき・マウスを載せたときは面をうっすら塗る
            if (down || highlighted || on) {
                g.setColour((on ? colours::accent : colours::text).withAlpha((down ? alpha::down : alpha::hover) * opacity));
                g.fillRoundedRectangle(bounds, shape::r(radius::control));
            }
            g.setColour((on ? colours::accent : colours::fieldEdge).withMultipliedAlpha(opacity));
            g.drawRoundedRectangle(bounds, shape::r(radius::control), on ? stroke::selected : stroke::hairline);
            return;
        }
        auto colour = on ? button.findColour(juce::TextButton::buttonOnColourId) : backgroundColour;
        if (down)
            colour = state::down(colour);
        else if (highlighted)
            colour = state::hover(colour);
        g.setColour(colour.withMultipliedAlpha(opacity));
        g.fillRoundedRectangle(bounds, shape::r(radius::control));
        g.setColour(colours::fieldEdge.withMultipliedAlpha(opacity));
        g.drawRoundedRectangle(bounds, shape::r(radius::control), stroke::hairline);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool highlighted, bool down) override
    {
        if (shape::button == shape::Button::outline && button.getToggleState()) {
            g.setFont(getTextButtonFont(button, button.getHeight()));
            g.setColour(colours::accent.withMultipliedAlpha(button.isEnabled() ? 1.0f : alpha::disabled));
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
    int getAlertWindowButtonHeight() override { return space::control; }

    // JUCEの既定は角の丸みと枠の太さ、警告アイコンの色が固定なので、ここで描く
    void drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, const juce::Rectangle<int>&, juce::TextLayout& layout) override
    {
        const auto bounds = alert.getLocalBounds().toFloat();
        g.setColour(colours::panel);
        g.fillRoundedRectangle(bounds, shape::r(radius::overlay));
        g.setColour(colours::fieldEdge);
        g.drawRoundedRectangle(bounds.reduced(0.5f), shape::r(radius::overlay), stroke::hairline);

        int iconSpace = 0;
        if (alert.getAlertType() != juce::MessageBoxIconType::NoIcon) {
            iconSpace = 80; // AlertWindow が文字の左に空ける幅
            const auto icon = juce::Rectangle<float>(40.0f, 36.0f).withPosition(static_cast<float>(space::lg + space::xs), static_cast<float>(space::xl));
            juce::Path triangle;
            triangle.addTriangle(icon.getCentreX(), icon.getY(), icon.getRight(), icon.getBottom(), icon.getX(), icon.getBottom());
            g.setColour(colours::warning);
            g.fillPath(triangle.createPathWithRoundedCorners(shape::r(radius::data)));
            g.setColour(colours::panel);
            g.fillRect(juce::Rectangle<float>(3.0f, 14.0f).withCentre({ icon.getCentreX(), icon.getY() + 18.0f }));
            g.fillRect(juce::Rectangle<float>(3.0f, 3.0f).withCentre({ icon.getCentreX(), icon.getY() + 30.0f }));
        }
        layout.draw(g, juce::Rectangle<int>(iconSpace, 30, alert.getWidth() - iconSpace, alert.getHeight() - getAlertWindowButtonHeight() - 20).toFloat());
    }

    void drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&) override
    {
        g.setColour(colours::panel);
        g.fillPath(path);
        g.setColour(colours::fieldEdge);
        g.strokePath(path, juce::PathStrokeType(stroke::hairline));
    }

    float getCallOutBoxCornerSize(const juce::CallOutBox&) override { return shape::r(radius::overlay); }
    int getCallOutBoxBorderSize(const juce::CallOutBox&) override { return space::lg; }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int, int, int, int, juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<int>(width, height).toFloat();
        const float opacity = box.isEnabled() ? 1.0f : alpha::disabled;
        const auto fill = box.findColour(juce::ComboBox::backgroundColourId);
        g.setColour((isButtonDown ? state::down(fill) : fill).withMultipliedAlpha(opacity));
        g.fillRoundedRectangle(bounds, shape::r(radius::control));
        g.setColour(box.findColour(juce::ComboBox::outlineColourId).withMultipliedAlpha(opacity));
        g.drawRoundedRectangle(bounds.reduced(0.5f), shape::r(radius::control), stroke::hairline);

        const juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
        juce::Path arrow;
        arrow.startNewSubPath(static_cast<float>(arrowZone.getX()) + 3.0f, static_cast<float>(arrowZone.getCentreY()) - 2.0f);
        arrow.lineTo(static_cast<float>(arrowZone.getCentreX()), static_cast<float>(arrowZone.getCentreY()) + 3.0f);
        arrow.lineTo(static_cast<float>(arrowZone.getRight()) - 3.0f, static_cast<float>(arrowZone.getCentreY()) - 2.0f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(opacity));
        g.strokePath(arrow, juce::PathStrokeType(stroke::focus));
    }

    void drawTickBox(juce::Graphics& g, juce::Component&, float x, float y, float w, float h, bool ticked,
                     bool isEnabled, bool highlighted, bool down) override
    {
        const juce::Rectangle<float> box(x, y, w, h);
        const float opacity = isEnabled ? 1.0f : alpha::disabled;
        const auto fill = down ? state::down(colours::panelRaised) : highlighted ? state::hover(colours::panelRaised) : colours::panelRaised;
        g.setColour(fill.withMultipliedAlpha(opacity));
        g.fillRoundedRectangle(box, shape::r(radius::data));
        g.setColour(colours::fieldEdge.withMultipliedAlpha(opacity));
        g.drawRoundedRectangle(box.reduced(0.5f), shape::r(radius::data), stroke::hairline);
        if (ticked) {
            g.setColour(colours::accent.withMultipliedAlpha(opacity));
            const auto tick = getTickShape(0.75f);
            g.fillPath(tick, tick.getTransformToScaleToFit(box.reduced(4.0f, 5.0f), false));
        }
    }

    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        g.setColour(editor.findColour(juce::TextEditor::backgroundColourId).withMultipliedAlpha(editor.isEnabled() ? 1.0f : alpha::disabled));
        g.fillRoundedRectangle(juce::Rectangle<int>(width, height).toFloat(), shape::r(radius::control));
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        const auto bounds = juce::Rectangle<int>(width, height).toFloat();
        g.setColour(editor.findColour(juce::TextEditor::outlineColourId).withMultipliedAlpha(editor.isEnabled() ? 1.0f : alpha::disabled));
        g.drawRoundedRectangle(bounds.reduced(0.5f), shape::r(radius::control), stroke::hairline);
        // 枠の太さは変えず、入力中はリングを重ねる（マウスで入ったときも入力先が分かるように）
        if (editor.isEnabled() && editor.hasKeyboardFocus(true) && !editor.isReadOnly())
            drawFocusRing(g, bounds);
    }

    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override
    {
        const auto layout = tooltipLayout(tipText, colours::text);
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
        g.drawRect(bounds.toFloat(), stroke::hairline);
        tooltipLayout(text, colours::text).draw(g, bounds.toFloat().reduced(7.0f, 3.0f));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down) override
    {
        const float box = juce::jmin(20.0f, button.getHeight() * 0.7f);
        drawTickBox(g, button, 4.0f, (button.getHeight() - box) * 0.5f, box, box, button.getToggleState(),
                    button.isEnabled(), highlighted, down);
        g.setColour(button.findColour(juce::ToggleButton::textColourId).withMultipliedAlpha(button.isEnabled() ? 1.0f : alpha::disabled));
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
        label->setColour(juce::Label::outlineWhenEditingColourId, colours::focusRing);
        return label;
    }
};

} // namespace ui
