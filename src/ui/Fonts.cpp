#include "Fonts.h"
#include "Theme.h"
#include "UiFontsData.h"
#include "UserSettings.h"
#include "Themes.h"

namespace ui {

namespace {

const char* const kSettingKey = "uiFont";
const char* const kChoiceIds[] = { "system", "AzurZulu", "future6x8_shrink" };

juce::PropertiesFile& settings() { return userSettings(); }

juce::Typeface::Ptr load(const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
}

FontChoice& choiceStorage()
{
    static FontChoice choice = [] {
        const auto id = settings().getValue(kSettingKey, kChoiceIds[0]);
        for (int i = 0; i < 3; ++i)
            if (id == kChoiceIds[i])
                return static_cast<FontChoice>(i);
        return FontChoice::system;
    }();
    return choice;
}

void restyleLabels(juce::Component& c)
{
    for (auto* child : c.getChildren()) {
        if (auto* label = dynamic_cast<juce::Label*>(child)) {
            const auto& props = label->getProperties();
            if (props.contains("uiFontHeight"))
                setLabelFont(*label, static_cast<float>(props["uiFontHeight"]), props["uiFontBold"], props["uiFontMono"]);
        }
        restyleLabels(*child);
    }
}

} // namespace

juce::StringArray fontChoiceNames()
{
    return { "System default", "AzurZulu", "future6x8_shrink" };
}

FontChoice currentFontChoice()
{
    return choiceStorage();
}

void setFontChoice(FontChoice choice)
{
    choiceStorage() = choice;
    settings().setValue(kSettingKey, kChoiceIds[static_cast<int>(choice)]);
    settings().saveIfNeeded();
    appearanceChanged().sendChangeMessage();
}

juce::Typeface::Ptr customTypeface()
{
    static const juce::Typeface::Ptr azurZulu = load(UiFontsData::azurzulu_ttf, UiFontsData::azurzulu_ttfSize);
    static const juce::Typeface::Ptr future6x8 = load(UiFontsData::f68_ttf, UiFontsData::f68_ttfSize);
    switch (currentFontChoice()) {
        case FontChoice::azurZulu: return azurZulu;
        case FontChoice::future6x8: return future6x8;
        case FontChoice::system: break;
    }
    return nullptr;
}

void refreshFonts(juce::Component& root)
{
    restyleLabels(root);
    root.sendLookAndFeelChange(); // スライダーの数値欄などを作り直す
    root.repaint();
}

} // namespace ui
