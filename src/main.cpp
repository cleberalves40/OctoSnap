#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include "MainWindow.h"
#include "Theme.h"

namespace {

void installTranslator(QApplication &app) {
    const QString code = QSettings().value(QStringLiteral("ui/language"),
                                           QStringLiteral("system")).toString();
    const QLocale locale = (code == QLatin1String("system")) ? QLocale::system() : QLocale(code);

    auto *translator = new QTranslator(&app);
    // QTranslator falls back from pt_BR to pt and from en_US to en automatically.
    if (translator->load(locale, QStringLiteral("octosnap"), QStringLiteral("_"),
                         QStringLiteral(":/i18n"))) {
        app.installTranslator(translator);
    } else {
        delete translator;
    }
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("OctoSnap"));
    QApplication::setApplicationName(QStringLiteral("OctoSnap"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    Theme::captureSystemDefaults();
    installTranslator(app);
    Theme::apply(Theme::current());

    MainWindow w;
    w.show();
    return app.exec();
}
