#pragma once
#include "UserSettings.h"
#include "Themes.h"

namespace ui {
inline constexpr int scalePercentages[] { 50, 75, 100, 125, 150, 175, 200 };

inline int normaliseUIScale(int percent)
{
    for (int value : scalePercentages)
        if (percent == value) return value;
    return 100;
}

inline int& scaleStorage()
{
    static int percent = normaliseUIScale(userSettings().getIntValue("uiScalePercent", 100));
    return percent;
}

inline int currentUIScalePercent() { return scaleStorage(); }

inline void setUIScalePercent(int percent)
{
    percent = normaliseUIScale(percent);
    if (scaleStorage() == percent) return;
    scaleStorage() = percent;
    userSettings().setValue("uiScalePercent", percent);
    userSettings().saveIfNeeded();
    appearanceChanged().sendChangeMessage();
}
} // namespace ui
