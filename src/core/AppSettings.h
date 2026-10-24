#pragma once

#include <QObject>
#include <QSettings>

class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int maxInputChars READ maxInputChars WRITE setMaxInputChars NOTIFY maxInputCharsChanged)
    Q_PROPERTY(int maxRecordingSeconds READ maxRecordingSeconds WRITE setMaxRecordingSeconds NOTIFY maxRecordingSecondsChanged)
    Q_PROPERTY(bool autoPlayVoiceResults READ autoPlayVoiceResults WRITE setAutoPlayVoiceResults NOTIFY autoPlayVoiceResultsChanged)
    Q_PROPERTY(int minInputChars READ minInputChars CONSTANT)
    Q_PROPERTY(int hardMaxInputChars READ hardMaxInputChars CONSTANT)
    Q_PROPERTY(int minRecordingSeconds READ minRecordingSeconds CONSTANT)
    Q_PROPERTY(int hardMaxRecordingSeconds READ hardMaxRecordingSeconds CONSTANT)

public:
    // Ceilings are provisional until measured on 8 GB CPU-only machines; users may only lower them.
    static constexpr int kMinInputChars = 500;
    static constexpr int kHardMaxInputChars = 5000;
    static constexpr int kMinRecordingSeconds = 10;
    static constexpr int kHardMaxRecordingSeconds = 120;

    explicit AppSettings(const QString& fileName, QObject* parent = nullptr);

    int maxInputChars() const;
    void setMaxInputChars(int value);
    int maxRecordingSeconds() const;
    void setMaxRecordingSeconds(int value);
    bool autoPlayVoiceResults() const;
    void setAutoPlayVoiceResults(bool value);

    int minInputChars() const { return kMinInputChars; }
    int hardMaxInputChars() const { return kHardMaxInputChars; }
    int minRecordingSeconds() const { return kMinRecordingSeconds; }
    int hardMaxRecordingSeconds() const { return kHardMaxRecordingSeconds; }

    QString value(const QString& key) const;
    void setValue(const QString& key, const QString& value);

signals:
    void maxInputCharsChanged();
    void maxRecordingSecondsChanged();
    void autoPlayVoiceResultsChanged();

private:
    mutable QSettings m_settings;
};
