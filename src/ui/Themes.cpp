#include "Themes.h"
#include "Theme.h"
#include "UserSettings.h"

namespace ui {

namespace {

const char* const kSettingKey = "uiTheme";

juce::Colour c(juce::uint32 argb) { return juce::Colour(argb); }

// 同梱テーマ。どれも 文字に使う色 4.5:1 以上、操作部品の縁・フォーカス 3:1 以上（パネルに対して）。
ThemeData midnight()
{
    ThemeData t;
    t.name = "Midnight";
    t.background = c(0xff15171c); t.panel = c(0xff1e2128); t.panelRaised = c(0xff272b34); t.outline = c(0xff343944);
    t.fieldEdge = c(0xff707888); t.text = c(0xffe4e7ec); t.textDim = c(0xff9aa2b0);
    t.accent = c(0xffffa53c); t.accentDim = c(0xff7a5428); t.onAccent = c(0xff15171c); t.focusRing = c(0xffffa53c);
    t.carrier = c(0xff4fc3f7); t.modulator = c(0xffc792ea);
    t.meterGreen = c(0xff5fd068); t.meterYellow = c(0xffffd54f); t.meterRed = c(0xffff6363); t.meterRms = c(0x59e4e7ec);
    t.scrim = c(0x8c0b0c0f); t.lcdBack = c(0xff2a1d0c); t.lcdOn = c(0xffffb347); t.lcdOff = c(0xff3d2b13);
    t.keyWhite = c(0xffe4e7ec); t.keyBlack = c(0xff20232a);
    return t;
}

ThemeData daylight()
{
    ThemeData t = midnight();
    t.name = "Daylight";
    t.background = c(0xffe9ecf0); t.panel = c(0xfff6f7f9); t.panelRaised = c(0xfffdfdfe); t.outline = c(0xffd3d8df);
    t.fieldEdge = c(0xff7d8594); t.text = c(0xff1b1f27); t.textDim = c(0xff535b69);
    t.accent = c(0xffad4a00); t.accentDim = c(0xfff2cfae); t.onAccent = c(0xfffdfdfe); t.focusRing = c(0xffad4a00);
    t.carrier = c(0xff0a69ad); t.modulator = c(0xff8237b8);
    t.meterGreen = c(0xff23893a); t.meterYellow = c(0xff876500); t.meterRed = c(0xffc02a2a); t.meterRms = c(0x591b1f27);
    t.scrim = c(0x660b0c0f); t.keyWhite = c(0xfffdfdfe); t.keyBlack = c(0xff3a404c);
    t.channelSaturation = 0.75f; t.channelBrightness = 0.72f;
    return t;
}

ThemeData amberLcd()
{
    ThemeData t = midnight();
    t.name = "Amber LCD";
    t.background = c(0xff110b04); t.panel = c(0xff1a1208); t.panelRaised = c(0xff26190b); t.outline = c(0xff3d2b13);
    t.fieldEdge = c(0xff8c6b3b); t.text = c(0xffffd08a); t.textDim = c(0xffcf9e58);
    t.accent = c(0xffffb347); t.accentDim = c(0xff5e3f17); t.onAccent = c(0xff110b04); t.focusRing = c(0xfffff0c8);
    t.carrier = c(0xffffd27a); t.modulator = c(0xfff08a5d);
    t.meterGreen = c(0xffffb347); t.meterYellow = c(0xffffd27a); t.meterRed = c(0xffff6a3d); t.meterRms = c(0x59ffd08a);
    t.scrim = c(0x99050302); t.keyWhite = c(0xff6b4d24); t.keyBlack = c(0xff1a1208);
    t.roundness = 0.2f; t.knob = static_cast<int>(shape::Knob::pointer); t.button = static_cast<int>(shape::Button::outline);
    t.titleGradient = false; t.monochromeChannels = true;
    return t;
}

ThemeData highContrast()
{
    ThemeData t = midnight();
    t.name = "High Contrast";
    t.background = c(0xff050505); t.panel = c(0xff0c0c0c); t.panelRaised = c(0xff1a1a1a); t.outline = c(0xff5a5a5a);
    t.fieldEdge = c(0xffbdbdbd); t.text = c(0xfff7f7f7); t.textDim = c(0xffd4d4d4);
    t.accent = c(0xffffd400); t.accentDim = c(0xff6e5c00); t.onAccent = c(0xff050505); t.focusRing = c(0xff00e5ff);
    t.carrier = c(0xff5cc8ff); t.modulator = c(0xffff85e4);
    t.meterGreen = c(0xff39e75f); t.meterYellow = c(0xffffd400); t.meterRed = c(0xffff4d4d); t.meterRms = c(0x66f7f7f7);
    t.scrim = c(0xb3000000); t.keyWhite = c(0xfff7f7f7); t.keyBlack = c(0xff0c0c0c);
    t.roundness = 0.35f; t.knob = static_cast<int>(shape::Knob::filled); t.button = static_cast<int>(shape::Button::outline);
    t.titleGradient = false; t.channelSaturation = 0.7f; t.channelBrightness = 1.0f;
    return t;
}

juce::Array<ThemeData> builtInThemes()
{
    return { midnight(), daylight(), amberLcd(), highContrast() };
}

// --- テーマファイル -----------------------------------------------------------

struct ColourField { const char* name; juce::Colour ThemeData::* member; };
const ColourField kColourFields[] = {
    { "background", &ThemeData::background }, { "panel", &ThemeData::panel }, { "panelRaised", &ThemeData::panelRaised },
    { "outline", &ThemeData::outline }, { "fieldEdge", &ThemeData::fieldEdge }, { "text", &ThemeData::text },
    { "textDim", &ThemeData::textDim }, { "accent", &ThemeData::accent }, { "accentDim", &ThemeData::accentDim },
    { "onAccent", &ThemeData::onAccent }, { "focusRing", &ThemeData::focusRing }, { "carrier", &ThemeData::carrier },
    { "modulator", &ThemeData::modulator }, { "meterGreen", &ThemeData::meterGreen }, { "meterYellow", &ThemeData::meterYellow },
    { "meterRed", &ThemeData::meterRed }, { "meterRms", &ThemeData::meterRms }, { "scrim", &ThemeData::scrim },
    { "lcdBack", &ThemeData::lcdBack }, { "lcdOn", &ThemeData::lcdOn }, { "lcdOff", &ThemeData::lcdOff },
    { "keyWhite", &ThemeData::keyWhite }, { "keyBlack", &ThemeData::keyBlack },
};
const char* const kKnobNames[] = { "arc", "pointer", "filled" };
const char* const kButtonNames[] = { "filled", "outline" };

// "#rrggbb" か "#aarrggbb"
bool parseColour(const juce::String& text, juce::Colour& out)
{
    auto hex = text.trim().trimCharactersAtStart("#");
    if (!(hex.length() == 6 || hex.length() == 8) || !hex.containsOnly("0123456789abcdefABCDEF"))
        return false;
    if (hex.length() == 6)
        hex = "ff" + hex;
    out = juce::Colour(static_cast<juce::uint32>(hex.getHexValue64()));
    return true;
}

juce::String colourText(juce::Colour colour)
{
    return colour.getAlpha() == 0xff ? "#" + colour.toDisplayString(false).toLowerCase()
                                     : "#" + colour.toDisplayString(true).toLowerCase();
}

ThemeData loadThemeFile(const juce::File& file, const juce::Array<ThemeData>& bases)
{
    const auto json = juce::JSON::parse(file.loadFileAsString());
    const juce::String baseName = json.getProperty("base", "Midnight");
    ThemeData t = bases[0];
    for (const auto& b : bases)
        if (b.name == baseName)
            t = b;
    t.name = json.getProperty("name", file.getFileNameWithoutExtension()).toString();
    t.file = file;
    if (!json.isObject()) {
        t.loadError = "Not a JSON object";
        return t;
    }
    juce::StringArray bad;
    if (auto* coloursObj = json.getProperty("colours", {}).getDynamicObject()) {
        for (const auto& field : kColourFields)
            if (coloursObj->hasProperty(field.name) && !parseColour(coloursObj->getProperty(field.name).toString(), t.*(field.member)))
                bad.add(field.name);
    }
    if (auto* shapeObj = json.getProperty("shape", {}).getDynamicObject()) {
        const auto get = [&](const char* key, const juce::var& fallback) { return shapeObj->hasProperty(key) ? shapeObj->getProperty(key) : fallback; };
        t.roundness = juce::jlimit(0.0f, 3.0f, static_cast<float>(get("roundness", t.roundness)));
        const auto knob = get("knob", kKnobNames[t.knob]).toString();
        for (int i = 0; i < 3; ++i) if (knob == kKnobNames[i]) t.knob = i;
        const auto button = get("button", kButtonNames[t.button]).toString();
        for (int i = 0; i < 2; ++i) if (button == kButtonNames[i]) t.button = i;
        t.sectionBorders = static_cast<bool>(get("sectionBorders", t.sectionBorders));
        t.titleGradient = static_cast<bool>(get("titleGradient", t.titleGradient));
        t.monochromeChannels = static_cast<bool>(get("monochromeChannels", t.monochromeChannels));
        t.channelSaturation = juce::jlimit(0.0f, 1.0f, static_cast<float>(get("channelSaturation", t.channelSaturation)));
        t.channelBrightness = juce::jlimit(0.0f, 1.0f, static_cast<float>(get("channelBrightness", t.channelBrightness)));
    }
    if (!bad.isEmpty())
        t.loadError = "Unreadable colour: " + bad.joinIntoString(", ");
    return t;
}

juce::String themeToJson(const ThemeData& t, const juce::String& baseName)
{
    auto* coloursObj = new juce::DynamicObject();
    for (const auto& field : kColourFields)
        coloursObj->setProperty(field.name, colourText(t.*(field.member)));
    auto* shapeObj = new juce::DynamicObject();
    shapeObj->setProperty("roundness", t.roundness);
    shapeObj->setProperty("knob", kKnobNames[juce::jlimit(0, 2, t.knob)]);
    shapeObj->setProperty("button", kButtonNames[juce::jlimit(0, 1, t.button)]);
    shapeObj->setProperty("sectionBorders", t.sectionBorders);
    shapeObj->setProperty("titleGradient", t.titleGradient);
    shapeObj->setProperty("monochromeChannels", t.monochromeChannels);
    shapeObj->setProperty("channelSaturation", t.channelSaturation);
    shapeObj->setProperty("channelBrightness", t.channelBrightness);
    auto* root = new juce::DynamicObject();
    root->setProperty("name", t.name);
    root->setProperty("base", baseName);
    root->setProperty("colours", juce::var(coloursObj));
    root->setProperty("shape", juce::var(shapeObj));
    return juce::JSON::toString(juce::var(root));
}

// チャンネルの色は文字にも使う。どの面でも 4.5:1 に届くまで文字色へ寄せる（色相はなるべく残す）。
juce::Colour readableChannelColour(juce::Colour colour)
{
    const juce::Colour surfaces[] { colours::background, colours::panel, colours::panelRaised,
                                    colours::panel.overlaidWith(colours::accent.withAlpha(alpha::selected)) };
    for (float mix = 0.0f; mix < 1.0f; mix += 0.04f) {
        const auto candidate = colour.interpolatedWith(colours::text, mix);
        float worst = 21.0f;
        for (const auto& surface : surfaces)
            worst = juce::jmin(worst, contrastRatio(candidate, surface));
        if (worst >= 4.5f)
            return candidate;
    }
    return colours::text;
}

void apply(const ThemeData& t)
{
    colours::background = t.background; colours::panel = t.panel; colours::panelRaised = t.panelRaised;
    colours::outline = t.outline; colours::fieldEdge = t.fieldEdge; colours::text = t.text; colours::textDim = t.textDim;
    colours::accent = t.accent; colours::accentDim = t.accentDim; colours::onAccent = t.onAccent; colours::focusRing = t.focusRing;
    colours::carrier = t.carrier; colours::modulator = t.modulator;
    colours::meterGreen = t.meterGreen; colours::meterYellow = t.meterYellow; colours::meterRed = t.meterRed;
    colours::meterRms = t.meterRms; colours::scrim = t.scrim;
    colours::lcdBack = t.lcdBack; colours::lcdOn = t.lcdOn; colours::lcdOff = t.lcdOff;
    colours::keyWhite = t.keyWhite; colours::keyBlack = t.keyBlack;
    shape::roundness = t.roundness;
    shape::knob = static_cast<shape::Knob>(juce::jlimit(0, 2, t.knob));
    shape::button = static_cast<shape::Button>(juce::jlimit(0, 1, t.button));
    shape::sectionBorders = t.sectionBorders;
    shape::titleGradient = t.titleGradient;
    shape::monochromeChannels = t.monochromeChannels;
    shape::channelSaturation = t.channelSaturation;
    shape::channelBrightness = t.channelBrightness;
    for (int i = 0; i < 16; ++i)
        colours::channelTable[static_cast<size_t>(i)] = readableChannelColour(colours::rawChannelColour(i));
}

juce::String& currentName()
{
    static juce::String name = userSettings().getValue(kSettingKey, "Midnight");
    return name;
}

} // namespace

float contrastRatio(juce::Colour a, juce::Colour b)
{
    const auto luminance = [](juce::Colour c) {
        const auto linear = [](float v) { return v <= 0.03928f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
        return 0.2126f * linear(c.getFloatRed()) + 0.7152f * linear(c.getFloatGreen()) + 0.0722f * linear(c.getFloatBlue());
    };
    const float la = luminance(a), lb = luminance(b);
    return (juce::jmax(la, lb) + 0.05f) / (juce::jmin(la, lb) + 0.05f);
}

juce::Array<ThemeData> availableThemes()
{
    auto themes = builtInThemes();
    const auto bases = themes;
    const auto folder = userThemesFolder();
    if (folder.isDirectory()) {
        auto files = folder.findChildFiles(juce::File::findFiles, false, "*.json");
        files.sort();
        for (const auto& f : files) {
            auto t = loadThemeFile(f, bases);
            for (const auto& existing : themes)
                if (existing.name == t.name)
                    t.name << " (" << f.getFileName() << ")"; // 同じ名前は区別する
            themes.add(t);
        }
    }
    return themes;
}

juce::String currentThemeName()
{
    return currentName();
}

void selectTheme(const juce::String& name)
{
    currentName() = name;
    userSettings().setValue(kSettingKey, name);
    userSettings().saveIfNeeded();
    applySavedTheme();
    appearanceChanged().sendChangeMessage();
}

void applySavedTheme()
{
    const auto themes = availableThemes();
    for (const auto& t : themes)
        if (t.name == currentName()) {
            apply(t);
            return;
        }
    apply(themes[0]);
}

juce::ChangeBroadcaster& appearanceChanged()
{
    static juce::ChangeBroadcaster broadcaster;
    return broadcaster;
}

void refreshTheme(juce::Component& root)
{
    if (auto* lf = dynamic_cast<LookAndFeel*>(&root.getLookAndFeel()))
        lf->applyTheme();
    root.sendLookAndFeelChange();
    root.repaint();
}

juce::File prepareUserThemesFolder()
{
    const auto folder = userThemesFolder();
    folder.createDirectory();
    if (folder.findChildFiles(juce::File::findFiles, false, "*.json").isEmpty()) {
        // 見本: 現在のテーマを全項目つきで書き出す（名前だけ変えてある）
        auto themes = availableThemes();
        ThemeData sample = themes[0];
        for (const auto& t : themes)
            if (t.name == currentName())
                sample = t;
        const auto base = sample.file == juce::File() ? sample.name : juce::String("Midnight");
        sample.name = "My theme";
        folder.getChildFile("my-theme.json").replaceWithText(themeToJson(sample, base));
    }
    return folder;
}

} // namespace ui
