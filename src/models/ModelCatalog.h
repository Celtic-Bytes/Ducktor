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

// One selectable voice inside a speech synthesis model; multi-speaker models can offer several.
struct VoiceOption
{
    QString gender;
    int speaker = 0;
};

struct ModelInfo
{
    QString id;
    QString capability;
    QString engine;
    QString name;
    QVariantMap descriptions;
    QString license;
    QUrl licenseUrl;
    QUrl homepage;
    QString released;
    QString voiceLanguage;
    QString locale;
    QString quality;
    QStringList languages;
    QList<VoiceOption> voices;
    QList<ModelFile> files;
    QVariantMap options;
    bool available = false;
    bool recommended = false;
    bool experimental = false;

    qint64 totalSize() const;
    QString description() const;
    const VoiceOption* voice(const QString& gender) const;
};

namespace ModelCatalog {

// Engines this build can run; catalog entries for other engines are ignored.
const QStringList& supportedEngines();
bool isSafeId(const QString& id);
bool isSafeFileName(const QString& name);
bool isAllowedUrl(const QUrl& url);
// Text for the current interface language, falling back to English.
QString localized(const QVariantMap& texts);
QList<ModelInfo> parse(const QByteArray& json, QStringList* warnings = nullptr);

} // namespace ModelCatalog
