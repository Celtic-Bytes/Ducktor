#include "AudioDsp.h"

#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace AudioDsp {

LinearResampler::LinearResampler(int inputRate, int outputRate)
    : m_step(outputRate > 0 ? double(inputRate) / double(outputRate) : 1.0), m_passthrough(inputRate == outputRate)
{
}

void LinearResampler::reset()
{
    m_position = 0.0;
    m_last = 0.0f;
    m_hasLast = false;
}

void LinearResampler::process(const float* input, std::size_t count, std::vector<float>& output)
{
    if (count == 0)
        return;
    if (m_passthrough) {
        output.insert(output.end(), input, input + count);
        return;
    }
    // Index -1 refers to the last sample of the previous chunk.
    const auto sampleAt = [&](long long index) { return index < 0 ? m_last : input[index]; };
    const long long last = static_cast<long long>(count) - 1;
    while (true) {
        const long long i0 = static_cast<long long>(std::floor(m_position));
        if (i0 + 1 > last)
            break;
        if (i0 < 0 && !m_hasLast) {
            m_position = 0.0;
            continue;
        }
        const double frac = m_position - double(i0);
        const float a = sampleAt(i0);
        const float b = sampleAt(i0 + 1);
        output.push_back(static_cast<float>(a + (b - a) * frac));
        m_position += m_step;
    }
    m_position -= double(count);
    m_last = input[count - 1];
    m_hasLast = true;
}

void appendMono(const char* data, qsizetype bytes, const QAudioFormat& format, std::vector<float>& output)
{
    const int channels = std::max(1, format.channelCount());
    const int sampleBytes = format.bytesPerSample();
    if (sampleBytes <= 0)
        return;
    const qsizetype frameBytes = qsizetype(sampleBytes) * channels;
    const qsizetype frames = bytes / frameBytes;
    output.reserve(output.size() + std::size_t(frames));

    for (qsizetype f = 0; f < frames; ++f) {
        const char* frame = data + f * frameBytes;
        float sum = 0.0f;
        for (int c = 0; c < channels; ++c) {
            const char* s = frame + c * sampleBytes;
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8:
                sum += (static_cast<unsigned char>(*s) - 128) / 128.0f;
                break;
            case QAudioFormat::Int16:
                sum += qFromLittleEndian<qint16>(s) / 32768.0f;
                break;
            case QAudioFormat::Int32:
                sum += float(qFromLittleEndian<qint32>(s) / 2147483648.0);
                break;
            case QAudioFormat::Float: {
                float v = 0.0f;
                std::memcpy(&v, s, sizeof(float));
                sum += v;
                break;
            }
            default:
                break;
            }
        }
        output.push_back(std::clamp(sum / float(channels), -1.0f, 1.0f));
    }
}

QByteArray fromMono(const float* samples, std::size_t count, const QAudioFormat& format)
{
    const int channels = std::max(1, format.channelCount());
    const int sampleBytes = format.bytesPerSample();
    QByteArray out;
    if (sampleBytes <= 0)
        return out;
    out.resize(qsizetype(count) * channels * sampleBytes);
    char* dst = out.data();
    for (std::size_t i = 0; i < count; ++i) {
        const float v = std::clamp(samples[i], -1.0f, 1.0f);
        for (int c = 0; c < channels; ++c) {
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8:
                *reinterpret_cast<unsigned char*>(dst) = static_cast<unsigned char>(std::lround(v * 127.0f) + 128);
                break;
            case QAudioFormat::Int16:
                qToLittleEndian<qint16>(static_cast<qint16>(std::lround(v * 32767.0f)), dst);
                break;
            case QAudioFormat::Int32:
                qToLittleEndian<qint32>(static_cast<qint32>(std::llround(v * 2147483647.0)), dst);
                break;
            case QAudioFormat::Float:
                std::memcpy(dst, &v, sizeof(float));
                break;
            default:
                std::memset(dst, 0, std::size_t(sampleBytes));
                break;
            }
            dst += sampleBytes;
        }
    }
    return out;
}

void int16ToFloat(const QByteArray& pcm, std::vector<float>& output)
{
    const qsizetype count = pcm.size() / 2;
    output.reserve(output.size() + std::size_t(count));
    for (qsizetype i = 0; i < count; ++i)
        output.push_back(qFromLittleEndian<qint16>(pcm.constData() + i * 2) / 32768.0f);
}

bool isProbablySilence(const std::vector<float>& samples)
{
    if (samples.empty())
        return true;
    double energy = 0.0;
    float peak = 0.0f;
    for (const float s : samples) {
        energy += double(s) * double(s);
        peak = std::max(peak, std::abs(s));
    }
    const double rms = std::sqrt(energy / double(samples.size()));
    return rms < 0.002 && peak < 0.02f;
}

} // namespace AudioDsp
