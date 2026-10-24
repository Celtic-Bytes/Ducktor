#pragma once

#include <QString>
#include <QStringList>

namespace Paths {

// Portable mode: a "portable.txt" file next to the executable (or next to the macOS .app)
// keeps settings and downloaded models in a sibling "data" folder.
bool isPortable();
QString dataDir();
QString settingsFile();
QString userModelsDir();
QStringList bundledModelDirs();
QString engineExecutable(const QString& name);
QString espeakDataDir();

} // namespace Paths
