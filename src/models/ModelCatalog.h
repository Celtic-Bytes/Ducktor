#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

namespace Capability {
inline const QString Translation = QStringLiteral("translation");
inline const QString SpeechToText = QStringLiteral("speech-to-text");
inline const QString TextToSpeech = QStringLiteral("text-to-speech");
} // namespace Capability

struct ModelFile
{
    QString name;
    QUrl url;
    QByteArray sha256;
    qint64 size = 0;
};

struct ModelInfo
{
    QString id;
    QString capability;
    QString engine;
    QString name;
    QString description;
    QString license;
    QUrl licenseUrl;
    QUrl homepage;
    QString released;
    QString unavailableReason;
    QString voiceLanguage;
    QStringList languages;
    QList<ModelFile> files;
    QVariantMap options;
    bool available = false;
    bool recommended = false;
    bool experimental = false;

    qint64 totalSize() const;
};

namespace ModelCatalog {

// Engines this build can run; catalog entries for other engines are listed as not yet integrated.
const QStringList& supportedEngines();
bool isSafeId(const QString& id);
bool isSafeFileName(const QString& name);
bool isAllowedUrl(const QUrl& url);
QList<ModelInfo> parse(const QByteArray& json, QStringList* warnings = nullptr);

} // namespace ModelCatalog
