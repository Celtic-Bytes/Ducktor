#include "AppSettings.h"

#include <algorithm>

namespace {
const QString kMaxInputChars = QStringLiteral("limits/maxInputChars");
const QString kMaxRecordingSeconds = QStringLiteral("limits/maxRecordingSeconds");
const QString kAutoPlay = QStringLiteral("voice/autoPlayResults");
} // namespace

AppSettings::AppSettings(const QString& fileName, QObject* parent)
    : QObject(parent), m_settings(fileName, QSettings::IniFormat)
{
}

int AppSettings::maxInputChars() const
{
    return std::clamp(m_settings.value(kMaxInputChars, kHardMaxInputChars).toInt(), kMinInputChars, kHardMaxInputChars);
}

void AppSettings::setMaxInputChars(int value)
{
    value = std::clamp(value, kMinInputChars, kHardMaxInputChars);
    if (value == maxInputChars())
        return;
    m_settings.setValue(kMaxInputChars, value);
    emit maxInputCharsChanged();
}

int AppSettings::maxRecordingSeconds() const
{
    return std::clamp(m_settings.value(kMaxRecordingSeconds, kHardMaxRecordingSeconds).toInt(), kMinRecordingSeconds,
                      kHardMaxRecordingSeconds);
}

void AppSettings::setMaxRecordingSeconds(int value)
{
    value = std::clamp(value, kMinRecordingSeconds, kHardMaxRecordingSeconds);
    if (value == maxRecordingSeconds())
        return;
    m_settings.setValue(kMaxRecordingSeconds, value);
    emit maxRecordingSecondsChanged();
}

bool AppSettings::autoPlayVoiceResults() const
{
    return m_settings.value(kAutoPlay, true).toBool();
}

void AppSettings::setAutoPlayVoiceResults(bool value)
{
    if (value == autoPlayVoiceResults())
        return;
    m_settings.setValue(kAutoPlay, value);
    emit autoPlayVoiceResultsChanged();
}

QString AppSettings::value(const QString& key) const
{
    return m_settings.value(key).toString();
}

void AppSettings::setValue(const QString& key, const QString& value)
{
    m_settings.setValue(key, value);
}
