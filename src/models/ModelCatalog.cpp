#include "ModelCatalog.h"

#include "core/Languages.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

qint64 ModelInfo::totalSize() const
{
    qint64 total = 0;
    for (const ModelFile& file : files)
        total += file.size;
    return total;
}

QString ModelInfo::description() const
{
    return ModelCatalog::localized(descriptions);
}

const VoiceOption* ModelInfo::voice(const QString& gender) const
{
    for (const VoiceOption& option : voices) {
        if (option.gender == gender)
            return &option;
    }
    return nullptr;
}

namespace ModelCatalog {
namespace {

constexpr int kMaxSpeakerId = 9999;

QVariantMap texts(const QJsonValue& value)
{
    if (value.isString())
        return {{QStringLiteral("en"), value.toString()}};
    return value.toObject().toVariantMap();
}

bool isGender(const QString& text)
{
    return text == QLatin1String("female") || text == QLatin1String("male");
}

// "gender" for single-speaker voices, or "speakers": {"female": id, "male": id} for multi-speaker ones.
bool parseVoices(const QJsonObject& object, QList<VoiceOption>& voices)
{
    const QJsonValue gender = object.value(QStringLiteral("gender"));
    if (!gender.isUndefined())
        voices << VoiceOption{gender.toString(), 0};
    const QJsonObject speakers = object.value(QStringLiteral("speakers")).toObject();
    for (auto it = speakers.begin(); it != speakers.end(); ++it)
        voices << VoiceOption{it.key(), it.value().toInt(-1)};
    if (voices.isEmpty())
        return false;
    QStringList seen;
    for (const VoiceOption& voice : voices) {
        if (!isGender(voice.gender) || seen.contains(voice.gender) || voice.speaker < 0 || voice.speaker > kMaxSpeakerId)
            return false;
        seen << voice.gender;
    }
    return true;
}

bool isHex64(const QString& text)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{64}$"));
    return pattern.match(text).hasMatch();
}

} // namespace

QString localized(const QVariantMap& texts)
{
    const QString preferred = texts.value(Languages::uiLanguage()).toString();
    return preferred.isEmpty() ? texts.value(QStringLiteral("en")).toString() : preferred;
}

const QStringList& supportedEngines()
{
    static const QStringList engines{QStringLiteral("llama"), QStringLiteral("whisper"), QStringLiteral("piper")};
    return engines;
}

bool isSafeId(const QString& id)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9][a-z0-9._-]{1,63}$"));
    return pattern.match(id).hasMatch() && !id.contains(QStringLiteral(".."));
}

bool isSafeFileName(const QString& name)
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$"));
    return pattern.match(name).hasMatch() && !name.contains(QStringLiteral("..")) &&
           !name.endsWith(QStringLiteral(".part"));
}

bool isAllowedUrl(const QUrl& url)
{
    return url.isValid() && url.scheme() == QLatin1String("https") && url.host() == QLatin1String("huggingface.co") &&
           url.userInfo().isEmpty() && url.port() == -1;
}

QList<ModelInfo> parse(const QByteArray& json, QStringList* warnings)
{
    QList<ModelInfo> models;
    const auto warn = [warnings](const QString& message) {
        if (warnings)
            warnings->append(message);
    };

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        warn(QStringLiteral("catalog: %1").arg(error.errorString()));
        return models;
    }

    const QStringList capabilities{Capability::Translation, Capability::SpeechToText, Capability::TextToSpeech};
    QStringList seenIds;
    for (const QJsonValue& value : document.object().value(QStringLiteral("models")).toArray()) {
        const QJsonObject object = value.toObject();
        ModelInfo info;
        info.id = object.value(QStringLiteral("id")).toString();
        info.capability = object.value(QStringLiteral("capability")).toString();
        info.engine = object.value(QStringLiteral("engine")).toString();
        info.name = object.value(QStringLiteral("name")).toString();
        info.descriptions = texts(object.value(QStringLiteral("description")));
        info.license = object.value(QStringLiteral("license")).toString();
        info.licenseUrl = QUrl(object.value(QStringLiteral("licenseUrl")).toString());
        info.homepage = QUrl(object.value(QStringLiteral("homepage")).toString());
        info.released = object.value(QStringLiteral("released")).toString();
        info.voiceLanguage = object.value(QStringLiteral("voiceLanguage")).toString();
        info.locale = object.value(QStringLiteral("locale")).toString();
        info.quality = object.value(QStringLiteral("quality")).toString();
        info.recommended = object.value(QStringLiteral("recommended")).toBool();
        info.experimental = object.value(QStringLiteral("experimental")).toBool();
        info.options = object.value(QStringLiteral("options")).toObject().toVariantMap();
        for (const QJsonValue& language : object.value(QStringLiteral("languages")).toArray())
            info.languages << language.toString();

        if (!isSafeId(info.id) || seenIds.contains(info.id) || !capabilities.contains(info.capability) ||
            info.name.isEmpty()) {
            warn(QStringLiteral("catalog: invalid entry '%1'").arg(info.id));
            continue;
        }

        bool filesValid = true;
        for (const QJsonValue& fileValue : object.value(QStringLiteral("files")).toArray()) {
            const QJsonObject fileObject = fileValue.toObject();
            ModelFile file;
            file.name = fileObject.value(QStringLiteral("name")).toString();
            file.url = QUrl(fileObject.value(QStringLiteral("url")).toString());
            const QString sha = fileObject.value(QStringLiteral("sha256")).toString();
            file.sha256 = sha.toLatin1();
            file.size = fileObject.value(QStringLiteral("size")).toInteger(-1);
            if (!isSafeFileName(file.name) || !isAllowedUrl(file.url) || !isHex64(sha) || file.size <= 0) {
                filesValid = false;
                break;
            }
            info.files << file;
        }
        if (!filesValid) {
            warn(QStringLiteral("catalog: invalid files in '%1'").arg(info.id));
            continue;
        }
        if (!supportedEngines().contains(info.engine))
            continue;
        if (info.files.isEmpty()) {
            warn(QStringLiteral("catalog: '%1' has no files").arg(info.id));
            continue;
        }
        if (info.capability == Capability::TextToSpeech &&
            (!Languages::find(info.voiceLanguage) || !parseVoices(object, info.voices))) {
            warn(QStringLiteral("catalog: voice '%1' has no known language or gender").arg(info.id));
            continue;
        }
        info.available = true;
        seenIds << info.id;
        models << info;
    }
    return models;
}

} // namespace ModelCatalog
