#include "AudioRecorder.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>

#include <algorithm>

AudioRecorder::AudioRecorder(QObject* parent) : QObject(parent) {}

AudioRecorder::~AudioRecorder()
{
    release();
}

bool AudioRecorder::start(int maxSeconds, QString* error)
{
    if (m_source)
        return false;
    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        if (error)
            *error = QStringLiteral("no_device");
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(AudioDsp::kSpeechSampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format))
        format = device.preferredFormat();
    if (format.sampleRate() <= 0 || format.bytesPerSample() <= 0) {
        if (error)
            *error = QStringLiteral("unsupported_format");
        return false;
    }

    m_format = format;
    m_resampler = std::make_unique<AudioDsp::LinearResampler>(format.sampleRate(), AudioDsp::kSpeechSampleRate);
    m_maxSamples = std::size_t(maxSeconds) * AudioDsp::kSpeechSampleRate;
    m_samples.clear();
    m_samples.reserve(std::min<std::size_t>(m_maxSamples, std::size_t(AudioDsp::kSpeechSampleRate) * 30));

    m_source = new QAudioSource(device, format, this);
    m_device = m_source->start();
    if (!m_device) {
        if (error)
            *error = QStringLiteral("open_failed");
        release();
        return false;
    }
    connect(m_device, &QIODevice::readyRead, this, &AudioRecorder::onReadyRead);
    return true;
}

void AudioRecorder::stop()
{
    if (m_source)
        finish(false);
}

void AudioRecorder::cancel()
{
    release();
    m_samples.clear();
    m_samples.shrink_to_fit();
}

bool AudioRecorder::isRecording() const
{
    return m_source != nullptr;
}

double AudioRecorder::seconds() const
{
    return double(m_samples.size()) / AudioDsp::kSpeechSampleRate;
}

std::vector<float> AudioRecorder::takeSamples()
{
    return std::move(m_samples);
}

void AudioRecorder::onReadyRead()
{
    if (!m_device)
        return;
    const QByteArray data = m_device->readAll();
    m_scratch.clear();
    AudioDsp::appendMono(data.constData(), data.size(), m_format, m_scratch);
    std::vector<float> resampled;
    m_resampler->process(m_scratch.data(), m_scratch.size(), resampled);

    const std::size_t room = m_maxSamples - m_samples.size();
    const bool limitReached = resampled.size() >= room;
    m_samples.insert(m_samples.end(), resampled.begin(), resampled.begin() + std::ptrdiff_t(std::min(room, resampled.size())));
    emit progress(seconds());
    if (limitReached)
        finish(true);
}

void AudioRecorder::finish(bool limitReached)
{
    if (m_device && !limitReached)
        onReadyRead();
    if (!m_source)
        return;
    release();
    emit finished(limitReached);
}

void AudioRecorder::release()
{
    if (m_device)
        m_device->disconnect(this);
    m_device = nullptr;
    if (m_source) {
        m_source->stop();
        m_source->deleteLater();
        m_source = nullptr;
    }
    m_resampler.reset();
}
