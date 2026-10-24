// Speech synthesis helper: loads one Piper voice and serves SPEAK requests on stdin.
#include "protocol.hpp"

#include <piper.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

using namespace ducktor;

constexpr std::size_t kMaxTextBytes = 64 * 1024;

struct Options
{
    std::string model;
    std::string config;
    std::string espeakData;
    int maxSeconds = 600;
};

void sendError(const std::string& id, const std::string& code)
{
    proto::writeFrame(stdout, {"ERROR", id, code});
}

bool parseOptions(int argc, char** argv, Options& options)
{
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string key = argv[i];
        const std::string value = argv[i + 1];
        if (key == "--model")
            options.model = value;
        else if (key == "--config")
            options.config = value;
        else if (key == "--espeak-data")
            options.espeakData = value;
        else if (key == "--max-seconds")
            options.maxSeconds = std::clamp(std::atoi(value.c_str()), 1, 3600);
        else
            return false;
    }
    return !options.model.empty() && !options.config.empty() && !options.espeakData.empty();
}

void handleSpeak(piper_synthesizer* synth, const Options& options, const proto::Frame& frame)
{
    const std::string id = frame.fields.size() > 1 ? frame.fields[1] : "0";
    if (frame.payload.empty() || frame.payload.size() > kMaxTextBytes) {
        sendError(id, "bad_request");
        return;
    }
    if (piper_synthesize_start(synth, frame.payload.c_str(), nullptr) != PIPER_OK) {
        sendError(id, "synth_failed");
        return;
    }

    std::size_t produced = 0;
    bool complete = true;
    std::vector<int16_t> pcm;
    piper_audio_chunk chunk{};
    while (complete) {
        const int status = piper_synthesize_next(synth, &chunk);
        if (status == PIPER_DONE)
            break;
        if (status != PIPER_OK) {
            sendError(id, "synth_failed");
            return;
        }
        if (chunk.num_samples > 0 && chunk.samples) {
            const std::size_t limit = static_cast<std::size_t>(options.maxSeconds) * static_cast<std::size_t>(chunk.sample_rate);
            std::size_t count = chunk.num_samples;
            if (produced + count > limit) {
                count = limit > produced ? limit - produced : 0;
                complete = false;
            }
            pcm.resize(count);
            for (std::size_t i = 0; i < count; ++i) {
                const float v = std::clamp(chunk.samples[i], -1.0f, 1.0f);
                pcm[i] = static_cast<int16_t>(std::lround(v * 32767.0f));
            }
            produced += count;
            if (count > 0) {
                proto::writeFrame(stdout, {"AUDIO", id, std::to_string(chunk.sample_rate)}, pcm.data(),
                                  count * sizeof(int16_t));
            }
        }
    }
    proto::writeFrame(stdout, {"DONE", id, complete ? "1" : "0"});
}

} // namespace

int main(int argc, char** argv)
{
    proto::setBinaryStdio();
    Options options;
    if (!parseOptions(argc, argv, options)) {
        sendError("0", "bad_arguments");
        return 1;
    }

    piper_synthesizer* synth = piper_create(options.model.c_str(), options.config.c_str(), options.espeakData.c_str());
    if (!synth) {
        sendError("0", "load_failed");
        return 2;
    }
    proto::writeFrame(stdout, {"READY", "0"});

    proto::Frame frame;
    while (proto::readFrame(stdin, frame)) {
        if (frame.fields.empty() || frame.fields[0] == "QUIT")
            break;
        if (frame.fields[0] == "SPEAK")
            handleSpeak(synth, options, frame);
        else
            sendError(frame.fields.size() > 1 ? frame.fields[1] : "0", "bad_request");
    }

    piper_free(synth);
    return 0;
}
