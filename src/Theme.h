#pragma once

#include <QCoreApplication>
#include <QList>
#include <QString>

enum class ThemeId {
    System,
    Nord,
    CatppuccinFrappe,
    Selenized,
    Solarized
};

class Theme {
    Q_DECLARE_TR_FUNCTIONS(Theme)

public:
    static void captureSystemDefaults();   // call once, before apply()
    static void apply(ThemeId id);
    static ThemeId current();
    static void setCurrent(ThemeId id);
    static QList<ThemeId> all();
    static QString key(ThemeId id);
    static ThemeId fromKey(const QString &key);
    static QString name(ThemeId id);
};
