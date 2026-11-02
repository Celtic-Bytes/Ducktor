#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>

class AppSettings;

// The announcement bundled with the app (welcome, notices...), shown once per id and closed by
// the user. It is parsed and validated from a JSON resource and localized to the UI language.
class Announcement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString body READ body NOTIFY bodyChanged)
    Q_PROPERTY(QString urlLabel READ urlLabel NOTIFY urlLabelChanged)
    Q_PROPERTY(QString image READ image CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(bool unseen READ unseen NOTIFY unseenChanged)

public:
    Announcement(AppSettings* settings, const QByteArray& json, QObject* parent = nullptr);

    bool available() const { return m_available; }
    QString id() const { return m_id; }
    QString title() const { return m_title; }
    QString body() const { return m_body; }
    QString urlLabel() const { return m_urlLabel; }
    QString image() const { return m_image; }
    QString url() const { return m_url; }
    bool unseen() const;

    // Must be called after QLocale has been updated with the new interface language.
    void relocalize();
    Q_INVOKABLE void acknowledge();

signals:
    void titleChanged();
    void bodyChanged();
    void urlLabelChanged();
    void unseenChanged();

private:
    AppSettings* m_settings;
    bool m_available = false;
    QString m_id;
    QVariantMap m_titles;
    QVariantMap m_bodies;
    QVariantMap m_labels;
    QString m_title;
    QString m_body;
    QString m_urlLabel;
    QString m_image;
    QString m_url;
};
