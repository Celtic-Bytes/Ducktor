#pragma once

#include "AudioDsp.h"

#include <QAudioFormat>
#include <QElapsedTimer>
#include <QObject>

#include <memory>
#include <vector>

class QAudioSource;
class QIODevice;

// Captures the default microphone as 16 kHz mono float, never exceeding the configured duration.
class AudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit AudioRecorder(QObject* parent = nullptr);
    ~AudioRecorder() override;

    bool start(int maxSeconds, QString* error);
    void stop();
    void cancel();
    bool isRecording() const;
    double seconds() const;
    std::vector<float> takeSamples();

signals:
    void progress(double seconds);
    void finished(bool limitReached);

private:
    void onReadyRead();
    void finish(bool limitReached);
    void release();

    QAudioSource* m_source = nullptr;
    QIODevice* m_device = nullptr;
    QAudioFormat m_format;
    std::unique_ptr<AudioDsp::LinearResampler> m_resampler;
    std::vector<float> m_samples;
    std::vector<float> m_scratch;
    std::size_t m_maxSamples = 0;
};
