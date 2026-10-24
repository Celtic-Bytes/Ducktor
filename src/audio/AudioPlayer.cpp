#include "AudioPlayer.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>

#include <algorithm>
#include <vector>

AudioPlayer::AudioPlayer(QObject* parent) : QObject(parent)
{
    m_timer.setInterval(20);
    connect(&m_timer, &QTimer::timeout, this, &AudioPlayer::pump);
}

AudioPlayer::~AudioPlayer()
{
    stop();
}

bool AudioPlayer::begin(int sampleRate)
{
    stop();
    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull() || sampleRate <= 0)
        return false;

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!device.isFormatSupported(format))
        format = device.preferredFormat();
    if (format.bytesPerSample() <= 0 || format.sampleRate() <= 0)
        return false;

    m_format = format;
    m_resampler = std::make_unique<AudioDsp::LinearResampler>(sampleRate, format.sampleRate());
    m_sink = new QAudioSink(device, format, this);
    m_device = m_sink->start();
    if (!m_device) {
        stop();
        return false;
    }
    m_endOfStream = false;
    m_timer.start();
    emit activeChanged();
    return true;
}

void AudioPlayer::append(const QByteArray& int16Mono)
{
    if (!m_sink)
        return;
    std::vector<float> mono;
    AudioDsp::int16ToFloat(int16Mono, mono);
    std::vector<float> resampled;
    m_resampler->process(mono.data(), mono.size(), resampled);
    m_pending.append(AudioDsp::fromMono(resampled.data(), resampled.size(), m_format));
}

void AudioPlayer::endOfStream()
{
    m_endOfStream = true;
}

void AudioPlayer::stop()
{
    m_timer.stop();
    m_pending.clear();
    m_endOfStream = false;
    m_device = nullptr;
    if (m_sink) {
        m_sink->stop();
        m_sink->deleteLater();
        m_sink = nullptr;
        emit activeChanged();
    }
    m_resampler.reset();
}

bool AudioPlayer::isActive() const
{
    return m_sink != nullptr;
}

void AudioPlayer::pump()
{
    if (!m_sink || !m_device)
        return;
    const int frameBytes = m_format.bytesPerFrame();
    qsizetype writable = std::min<qsizetype>(m_sink->bytesFree(), m_pending.size());
    writable -= writable % frameBytes;
    if (writable > 0) {
        const qint64 written = m_device->write(m_pending.constData(), writable);
        if (written > 0)
            m_pending.remove(0, written);
    }
    if (m_endOfStream && m_pending.isEmpty() && m_sink->bytesFree() >= m_sink->bufferSize())
        stop();
}
