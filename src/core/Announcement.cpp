#include "Announcement.h"

#include "AppSettings.h"
#include "Languages.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

namespace {

const QString kSeenKey = QStringLiteral("ui/lastSeenAnnouncement");

// A text is either a single string (treated as English) or an object mapping language codes to strings.
bool parseTexts(const QJsonValue& value, QVariantMap& out)
{
    if (value.isString()) {
        out.insert(QStringLiteral("en"), value.toString());
        return true;
    }
    const QJsonObject object = value.toObject();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (!it.value().isString() || it.value().toString().isEmpty())
            return false;
        out.insert(it.key(), it.value().toString());
    }
    return !out.isEmpty();
}

QString resolve(const QVariantMap& texts)
{
    const QString preferred = texts.value(Languages::uiLanguage()).toString();
    return preferred.isEmpty() ? texts.value(QStringLiteral("en")).toString() : preferred;
}

bool isSafeId(const QString& id)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9][a-z0-9._-]{1,63}$"));
    return pattern.match(id).hasMatch() && !id.contains(QStringLiteral(".."));
}

// Relative path to an image inside the bundled resources. The window loads it through the
// image://svg/ provider, so only SVG files are supported.
bool isSafeImage(const QString& path)
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._/-]{0,255}$"));
    return pattern.match(path).hasMatch() && !path.contains(QStringLiteral("..")) && !path.startsWith(QLatin1Char('/'))
        && path.endsWith(QStringLiteral(".svg"));
}

bool isAllowedUrl(const QString& text)
{
    const QUrl url(text);
    return url.isValid() && url.userInfo().isEmpty()
        && (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
}

} // namespace

Announcement::Announcement(AppSettings* settings, const QByteArray& json, QObject* parent)
    : QObject(parent), m_settings(settings)
{
    QJsonParseError error{};
    const QJsonObject object = QJsonDocument::fromJson(json, &error).object();
    QVariantMap titles, bodies, labels;
    const bool hasUrl = object.contains(QStringLiteral("url"));
    const bool hasLabel = object.contains(QStringLiteral("urlLabel"));
    const bool hasImage = object.contains(QStringLiteral("image"));

    m_id = object.value(QStringLiteral("id")).toString();
    m_url = object.value(QStringLiteral("url")).toString();
    m_image = object.value(QStringLiteral("image")).toString();

    // Like the model catalog, an invalid announcement is rejected as a whole instead of being fixed up.
    m_available = error.error == QJsonParseError::NoError
        && isSafeId(m_id)
        && parseTexts(object.value(QStringLiteral("title")), titles)
        && parseTexts(object.value(QStringLiteral("body")), bodies)
        && hasUrl == hasLabel
        && (!hasUrl || isAllowedUrl(m_url))
        && (!hasLabel || parseTexts(object.value(QStringLiteral("urlLabel")), labels))
        && (!hasImage || isSafeImage(m_image))
        && !titles.value(QStringLiteral("en")).toString().isEmpty()
        && !bodies.value(QStringLiteral("en")).toString().isEmpty()
        && (!hasLabel || !labels.value(QStringLiteral("en")).toString().isEmpty());

    if (m_available) {
        m_titles = titles;
        m_bodies = bodies;
        m_labels = labels;
    } else {
        qWarning("announcement: the bundled announcement is missing or invalid; it will not be shown");
    }

    relocalize();
}

bool Announcement::unseen() const
{
    return m_settings->value(kSeenKey) != m_id;
}

void Announcement::relocalize()
{
    if (!m_available)
        return;
    const QString title = resolve(m_titles);
    const QString body = resolve(m_bodies);
    const QString urlLabel = resolve(m_labels);
    if (title != m_title) {
        m_title = title;
        emit titleChanged();
    }
    if (body != m_body) {
        m_body = body;
        emit bodyChanged();
    }
    if (urlLabel != m_urlLabel) {
        m_urlLabel = urlLabel;
        emit urlLabelChanged();
    }
}

void Announcement::acknowledge()
{
    if (!m_available || !unseen())
        return;
    m_settings->setValue(kSeenKey, m_id);
    m_settings->sync();
    emit unseenChanged();
}

void Announcement::showOnNextLaunch()
{
    if (!m_available || unseen())
        return;
    m_settings->remove(kSeenKey);
    m_settings->sync();
    emit unseenChanged();
}
