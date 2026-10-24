#pragma once

#include <QAudioFormat>
#include <QByteArray>

#include <cstddef>
#include <vector>

namespace AudioDsp {

constexpr int kSpeechSampleRate = 16000;

// Streaming linear resampler; keeps state between calls so chunk borders do not click.
class LinearResampler
{
public:
    LinearResampler(int inputRate, int outputRate);
    void process(const float* input, std::size_t count, std::vector<float>& output);
    void reset();

private:
    double m_step;
    double m_position = 0.0;
    float m_last = 0.0f;
    bool m_hasLast = false;
    bool m_passthrough;
};

// Converts interleaved PCM in `format` to mono float samples appended to `output`.
void appendMono(const char* data, qsizetype bytes, const QAudioFormat& format, std::vector<float>& output);
// Converts mono float samples to interleaved PCM in `format` (same rate assumed).
QByteArray fromMono(const float* samples, std::size_t count, const QAudioFormat& format);
void int16ToFloat(const QByteArray& pcm, std::vector<float>& output);
bool isProbablySilence(const std::vector<float>& samples);

} // namespace AudioDsp
