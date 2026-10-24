#include "core/AppController.h"
#include "core/AppSettings.h"
#include "core/Paths.h"
#include "models/ModelManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTranslator>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("celticbytes"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("celticbytes.com"));
    QGuiApplication::setApplicationName(QStringLiteral("Ducktor"));
    QGuiApplication::setApplicationVersion(QStringLiteral(DUCKTOR_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("com.celticbytes.Ducktor"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/resources/icons/ducktor.svg")));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    AppSettings settings(Paths::settingsFile());
    QTranslator appTranslator;
    QTranslator qtTranslator;
    // English is the source language, so it needs no translation files.
    const auto installLanguage = [&](const QString& code) {
        QGuiApplication::removeTranslator(&appTranslator);
        QGuiApplication::removeTranslator(&qtTranslator);
        QLocale::setDefault(QLocale(code));
        if (code == QLatin1String("en"))
            return;
        if (appTranslator.load(QStringLiteral(":/i18n/ducktor_") + code))
            QGuiApplication::installTranslator(&appTranslator);
        if (qtTranslator.load(QLocale(code), QStringLiteral("qtbase"), QStringLiteral("_"),
                              QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
            QGuiApplication::installTranslator(&qtTranslator);
    };
    installLanguage(settings.uiLanguage());

    ModelManager models(&settings, Paths::userModelsDir(), Paths::bundledModelDirs());
    QFile catalog(QStringLiteral(":/resources/models.json"));
    if (catalog.open(QIODevice::ReadOnly))
        models.loadCatalog(catalog.readAll());
    AppController controller(&settings, &models);

    qmlRegisterSingletonInstance("Ducktor.Core", 1, 0, "App", &controller);
    qmlRegisterSingletonInstance("Ducktor.Core", 1, 0, "Models", &models);
    qmlRegisterSingletonInstance("Ducktor.Core", 1, 0, "Settings", &settings);

    QQmlApplicationEngine engine;
    QObject::connect(&settings, &AppSettings::uiLanguageChanged, &engine, [&]() {
        installLanguage(settings.uiLanguage());
        models.relocalize();
        emit controller.languagesChanged();
        engine.retranslate();
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Ducktor", "Main");
    return QGuiApplication::exec();
}
