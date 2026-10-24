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

namespace ModelCatalog {
namespace {

QString localized(const QJsonValue& value)
{
    if (value.isString())
        return value.toString();
    const QJsonObject object = value.toObject();
    const QString preferred = object.value(Languages::uiIsSpanish() ? QStringLiteral("es") : QStringLiteral("en")).toString();
    return preferred.isEmpty() ? object.value(QStringLiteral("en")).toString() : preferred;
}

bool isHex64(const QString& text)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{64}$"));
    return pattern.match(text).hasMatch();
}

} // namespace

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
        info.description = localized(object.value(QStringLiteral("description")));
        info.license = object.value(QStringLiteral("license")).toString();
        info.licenseUrl = QUrl(object.value(QStringLiteral("licenseUrl")).toString());
        info.homepage = QUrl(object.value(QStringLiteral("homepage")).toString());
        info.released = object.value(QStringLiteral("released")).toString();
        info.voiceLanguage = object.value(QStringLiteral("voiceLanguage")).toString();
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

        info.available = supportedEngines().contains(info.engine) && !info.files.isEmpty();
        if (info.available && info.capability == Capability::TextToSpeech && !Languages::find(info.voiceLanguage)) {
            warn(QStringLiteral("catalog: voice '%1' has no known language").arg(info.id));
            continue;
        }
        if (!info.available) {
            info.files.clear();
            info.unavailableReason = localized(object.value(QStringLiteral("unavailableReason")));
        }
        seenIds << info.id;
        models << info;
    }
    return models;
}

} // namespace ModelCatalog
