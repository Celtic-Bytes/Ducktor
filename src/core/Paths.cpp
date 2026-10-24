#include "Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace Paths {
namespace {

QString appDir()
{
    return QCoreApplication::applicationDirPath();
}

QStringList portableBaseDirs()
{
    QStringList dirs{appDir()};
#ifdef Q_OS_MACOS
    dirs << QDir::cleanPath(appDir() + QStringLiteral("/../../.."));
#endif
    return dirs;
}

QString portableDataDir()
{
    for (const QString& base : portableBaseDirs()) {
        if (!QFileInfo::exists(QDir(base).filePath(QStringLiteral("portable.txt"))))
            continue;
        const QString data = QDir(base).filePath(QStringLiteral("data"));
        if (QDir().mkpath(data) && QFileInfo(data).isWritable())
            return data;
    }
    return {};
}

QStringList engineDirs()
{
    QStringList dirs{appDir(), QDir::cleanPath(appDir() + QStringLiteral("/../libexec/ducktor"))};
    const QString fromEnv = qEnvironmentVariable("DUCKTOR_ENGINE_DIR");
    if (!fromEnv.isEmpty())
        dirs << fromEnv;
    return dirs;
}

} // namespace

bool isPortable()
{
    return !portableDataDir().isEmpty();
}

QString dataDir()
{
    QString path = portableDataDir();
    if (path.isEmpty())
        path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(path);
    return path;
}

QString settingsFile()
{
    return QDir(dataDir()).filePath(QStringLiteral("settings.ini"));
}

QString userModelsDir()
{
    return QDir(dataDir()).filePath(QStringLiteral("models"));
}

QStringList bundledModelDirs()
{
    QStringList dirs{QDir(appDir()).filePath(QStringLiteral("models")),
                     QDir::cleanPath(appDir() + QStringLiteral("/../share/ducktor/models")),
                     QDir::cleanPath(appDir() + QStringLiteral("/../Resources/models"))};
    const QString fromEnv = qEnvironmentVariable("DUCKTOR_MODELS_DIR");
    if (!fromEnv.isEmpty())
        dirs << fromEnv;
    return dirs;
}

QString engineExecutable(const QString& name)
{
#ifdef Q_OS_WIN
    const QString fileName = name + QStringLiteral(".exe");
#else
    const QString fileName = name;
#endif
    for (const QString& dir : engineDirs()) {
        const QFileInfo info(QDir(dir).filePath(fileName));
        if (info.isFile() && info.isExecutable())
            return info.absoluteFilePath();
    }
    return {};
}

QString espeakDataDir()
{
    QStringList candidates;
    for (const QString& dir : engineDirs())
        candidates << QDir(dir).filePath(QStringLiteral("espeak-ng-data"));
    candidates << QDir::cleanPath(appDir() + QStringLiteral("/../share/ducktor/espeak-ng-data"))
               << QDir::cleanPath(appDir() + QStringLiteral("/../Resources/espeak-ng-data"));
    for (const QString& candidate : candidates) {
        if (QFileInfo(candidate).isDir())
            return candidate;
    }
    return {};
}

} // namespace Paths
