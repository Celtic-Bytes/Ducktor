#include "core/AppController.h"
#include "core/AppSettings.h"
#include "core/Paths.h"
#include "models/ModelManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTranslator>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("Ducktor"));
    QGuiApplication::setApplicationName(QStringLiteral("Ducktor"));
    QGuiApplication::setApplicationVersion(QStringLiteral(DUCKTOR_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.ducktor.Ducktor"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/resources/icons/ducktor.svg")));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("ducktor"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        QGuiApplication::installTranslator(&translator);

    AppSettings settings(Paths::settingsFile());
    ModelManager models(&settings, Paths::userModelsDir(), Paths::bundledModelDirs());
    QFile catalog(QStringLiteral(":/resources/models.json"));
    if (catalog.open(QIODevice::ReadOnly))
        models.loadCatalog(catalog.readAll());
    AppController controller(&settings, &models);

    qmlRegisterSingletonInstance("Ducktor.Core", 1, 0, "App", &controller);
    qmlRegisterSingletonInstance("Ducktor.Core", 1, 0, "Models", &models);
    qmlRegisterSingletonInstance("Ducktor.Core", 1, 0, "Settings", &settings);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Ducktor", "Main");
    return QGuiApplication::exec();
}
