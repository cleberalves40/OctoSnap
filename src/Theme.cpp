#include "Theme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

#include <initializer_list>

namespace {

QPalette g_systemPalette;
QString g_systemStyle;

struct Colors {
    QColor window, base, alt, text;
    QColor button, buttonText;
    QColor highlight, highlightText, link;
    QColor tipBase, tipText, placeholder, disabled;
};

// Palette values follow each project's published dark variant.
Colors colorsFor(ThemeId id) {
    switch (id) {
    case ThemeId::Nord:
        return {
            QColor("#2E3440"), QColor("#3B4252"), QColor("#434C5E"), QColor("#D8DEE9"),
            QColor("#3B4252"), QColor("#ECEFF4"),
            QColor("#88C0D0"), QColor("#2E3440"), QColor("#88C0D0"),
            QColor("#3B4252"), QColor("#ECEFF4"), QColor("#4C566A"), QColor("#4C566A"),
        };
    case ThemeId::CatppuccinFrappe:
        return {
            QColor("#303446"), QColor("#292C3C"), QColor("#303446"), QColor("#C6D0F5"),
            QColor("#414559"), QColor("#C6D0F5"),
            QColor("#8CAAEE"), QColor("#303446"), QColor("#8CAAEE"),
            QColor("#292C3C"), QColor("#C6D0F5"), QColor("#737994"), QColor("#737994"),
        };
    case ThemeId::Selenized:
        return {
            QColor("#184956"), QColor("#103C48"), QColor("#184956"), QColor("#ADBCBC"),
            QColor("#2D5B69"), QColor("#CAD8D9"),
            QColor("#4695F7"), QColor("#103C48"), QColor("#41C7B9"),
            QColor("#103C48"), QColor("#CAD8D9"), QColor("#72898F"), QColor("#72898F"),
        };
    case ThemeId::Solarized:
        return {
            QColor("#002B36"), QColor("#073642"), QColor("#002B36"), QColor("#839496"),
            QColor("#073642"), QColor("#93A1A1"),
            QColor("#268BD2"), QColor("#FDF6E3"), QColor("#2AA198"),
            QColor("#073642"), QColor("#93A1A1"), QColor("#586E75"), QColor("#586E75"),
        };
    case ThemeId::System:
        break;
    }
    return {};
}

QPalette buildPalette(const Colors &c) {
    QPalette p;
    p.setColor(QPalette::Window, c.window);
    p.setColor(QPalette::WindowText, c.text);
    p.setColor(QPalette::Base, c.base);
    p.setColor(QPalette::AlternateBase, c.alt);
    p.setColor(QPalette::Text, c.text);
    p.setColor(QPalette::Button, c.button);
    p.setColor(QPalette::ButtonText, c.buttonText);
    p.setColor(QPalette::Highlight, c.highlight);
    p.setColor(QPalette::HighlightedText, c.highlightText);
    p.setColor(QPalette::Link, c.link);
    p.setColor(QPalette::ToolTipBase, c.tipBase);
    p.setColor(QPalette::ToolTipText, c.tipText);
    p.setColor(QPalette::PlaceholderText, c.placeholder);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setColor(QPalette::Disabled, role, c.disabled);
    return p;
}

} // namespace

void Theme::captureSystemDefaults() {
    g_systemPalette = qApp->palette();
    g_systemStyle = qApp->style()->name();
}

void Theme::apply(ThemeId id) {
    if (id == ThemeId::System) {
        if (QStyle *s = QStyleFactory::create(g_systemStyle))
            qApp->setStyle(s);
        qApp->setPalette(g_systemPalette);
        return;
    }
    // Custom palettes render consistently only on Fusion.
    if (QStyle *s = QStyleFactory::create(QStringLiteral("Fusion")))
        qApp->setStyle(s);
    qApp->setPalette(buildPalette(colorsFor(id)));
}

ThemeId Theme::current() {
    QSettings s;
    return fromKey(s.value(QStringLiteral("appearance/theme"), QStringLiteral("system")).toString());
}

void Theme::setCurrent(ThemeId id) {
    QSettings s;
    s.setValue(QStringLiteral("appearance/theme"), key(id));
}

QList<ThemeId> Theme::all() {
    return {ThemeId::System, ThemeId::Nord, ThemeId::CatppuccinFrappe,
            ThemeId::Selenized, ThemeId::Solarized};
}

QString Theme::key(ThemeId id) {
    switch (id) {
    case ThemeId::System:           return QStringLiteral("system");
    case ThemeId::Nord:             return QStringLiteral("nord");
    case ThemeId::CatppuccinFrappe: return QStringLiteral("catppuccin-frappe");
    case ThemeId::Selenized:        return QStringLiteral("selenized-dark");
    case ThemeId::Solarized:        return QStringLiteral("solarized-dark");
    }
    return QStringLiteral("system");
}

ThemeId Theme::fromKey(const QString &k) {
    for (ThemeId id : all())
        if (key(id) == k) return id;
    return ThemeId::System;
}

QString Theme::name(ThemeId id) {
    switch (id) {
    case ThemeId::System:           return tr("System");
    case ThemeId::Nord:             return tr("Nord");
    case ThemeId::CatppuccinFrappe: return tr("Catppuccin Frappé");
    case ThemeId::Selenized:        return tr("Selenized Dark");
    case ThemeId::Solarized:        return tr("Solarized Dark");
    }
    return tr("System");
}
