#pragma once

#include "AudioDsp.h"

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QTimer>

#include <memory>

class QAudioSink;
class QIODevice;

// Streams 16-bit mono PCM to the default output, converting to a supported device format if needed.
class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer() override;

    bool begin(int sampleRate);
    void append(const QByteArray& int16Mono);
    void endOfStream();
    void stop();
    bool isActive() const;

signals:
    void activeChanged();

private:
    void pump();

    QAudioSink* m_sink = nullptr;
    QIODevice* m_device = nullptr;
    QAudioFormat m_format;
    std::unique_ptr<AudioDsp::LinearResampler> m_resampler;
    QByteArray m_pending;
    QTimer m_timer;
    bool m_endOfStream = false;
};
