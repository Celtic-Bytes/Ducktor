#include "core/AppController.h"
#include "core/AppSettings.h"
#include "core/Paths.h"
#include "models/ModelManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickStyle>
#include <QSvgRenderer>
#include <QTranslator>

namespace {

// Rendering with QtSvg directly avoids depending on the qsvg image format plugin, which some
// Linux distributions ship in a separate package and deployment tools may leave out.
QImage renderSvg(const QString& path, const QSize& requestedSize)
{
    QSvgRenderer renderer(path);
    if (!renderer.isValid())
        return {};
    QSize size = renderer.defaultSize();
    if (requestedSize.width() > 0 && requestedSize.height() > 0)
        size.scale(requestedSize, Qt::KeepAspectRatio);
    else if (requestedSize.width() > 0)
        size = QSize(requestedSize.width(), size.height() * requestedSize.width() / size.width());
    else if (requestedSize.height() > 0)
        size = QSize(size.width() * requestedSize.height() / size.height(), requestedSize.height());
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter);
    return image;
}

// Serves image://svg/<path> from the :/resources/ directory of the application resources.
class SvgImageProvider : public QQuickImageProvider
{
public:
    SvgImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        QImage image = renderSvg(QStringLiteral(":/resources/") + id, requestedSize);
        if (size)
            *size = image.size();
        return image;
    }
};

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("celticbytes"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("celticbytes.com"));
    QGuiApplication::setApplicationName(QStringLiteral("Ducktor"));
    QGuiApplication::setApplicationVersion(QStringLiteral(DUCKTOR_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("com.celticbytes.Ducktor"));
    QIcon windowIcon;
    for (int side : {16, 24, 32, 48, 64, 128, 256})
        windowIcon.addPixmap(QPixmap::fromImage(renderSvg(QStringLiteral(":/resources/icons/ducktor.svg"), QSize(side, side))));
    QGuiApplication::setWindowIcon(windowIcon);
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
    engine.addImageProvider(QStringLiteral("svg"), new SvgImageProvider);
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
