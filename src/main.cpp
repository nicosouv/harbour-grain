#include <QtQuick>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <sailfishapp.h>

#include "engine/AppId.h"
#include "engine/GrainController.h"

int main(int argc, char* argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    // Language: an explicit choice in Settings overrides the system locale. (Qt 5.6 can't
    // retranslate live, so a change takes effect on the next launch.)
    QSettings settings(QLatin1String(grain::AppId::kOrganization),
                       QLatin1String(grain::AppId::kApplication));
    const QString chosen = settings.value(QStringLiteral("language")).toString();
    const QString locale = chosen.isEmpty() ? QLocale::system().name() : chosen;

    QTranslator translator;
    const QString trDir = SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile();
    if (translator.load(QStringLiteral("harbour-grain-") + locale, trDir)
        || translator.load(QStringLiteral("harbour-grain-") + locale.left(2), trDir)) {
        app->installTranslator(&translator);
    }

    grain::GrainController controller;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("Game"), &controller);
    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-grain.qml")));
    view->show();

    return app->exec();
}
