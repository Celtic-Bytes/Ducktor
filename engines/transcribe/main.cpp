// Speech recognition helper: loads one whisper.cpp model and serves TRANSCRIBE requests on stdin.
#include "protocol.hpp"
#include "segmenter.hpp"
#include "whisper.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

using namespace ducktor;

constexpr float kNoSpeechThreshold = 0.6f;

struct Options
{
    std::string model;
    int threads = 4;
    int maxSeconds = 130;
};

void quietLog(ggml_log_level, const char*, void*) {}

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
        else if (key == "--threads")
            options.threads = std::clamp(std::atoi(value.c_str()), 1, 64);
        else if (key == "--max-seconds")
            options.maxSeconds = std::clamp(std::atoi(value.c_str()), 1, 600);
        else
            return false;
    }
    return !options.model.empty();
}

void handleTranscribe(whisper_context* context, const Options& options, const proto::Frame& frame)
{
    const std::string id = frame.fields.size() > 1 ? frame.fields[1] : "0";
    const std::string language = frame.fields.size() > 2 ? frame.fields[2] : "auto";
    if (frame.payload.size() % sizeof(float) != 0) {
        sendError(id, "bad_request");
        return;
    }
    const std::size_t sampleCount = frame.payload.size() / sizeof(float);
    if (sampleCount > static_cast<std::size_t>(options.maxSeconds) * WHISPER_SAMPLE_RATE) {
        sendError(id, "too_long");
        return;
    }
    if (sampleCount < WHISPER_SAMPLE_RATE / 4) {
        sendError(id, "too_short");
        return;
    }

    std::vector<float> samples(sampleCount);
    std::memcpy(samples.data(), frame.payload.data(), frame.payload.size());

    const bool autoDetect = language == "auto" || whisper_lang_id(language.c_str()) < 0;
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = options.threads;
    params.translate = false;
    params.no_context = true;
    params.no_timestamps = true;
    params.print_special = false;
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.suppress_blank = true;
    params.suppress_nst = true;
    params.language = whisper_is_multilingual(context) ? (autoDetect ? "auto" : language.c_str()) : "en";

    if (whisper_full(context, params, samples.data(), static_cast<int>(samples.size())) != 0) {
        sendError(id, "decode_failed");
        return;
    }

    std::string text;
    const int segments = whisper_full_n_segments(context);
    for (int i = 0; i < segments; ++i) {
        if (whisper_full_get_segment_no_speech_prob(context, i) > kNoSpeechThreshold)
            continue;
        const char* piece = whisper_full_get_segment_text(context, i);
        if (piece)
            text += piece;
    }
    text = seg::trim(text);
    if (text.empty()) {
        sendError(id, "no_speech");
        return;
    }

    const char* detected = whisper_lang_str(whisper_full_lang_id(context));
    proto::writeText(stdout, {"TRANSCRIPT", id, detected ? detected : "und"}, text);
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

    whisper_log_set(quietLog, nullptr);
    whisper_context_params contextParams = whisper_context_default_params();
    contextParams.use_gpu = false;
    whisper_context* context = whisper_init_from_file_with_params(options.model.c_str(), contextParams);
    if (!context) {
        sendError("0", "load_failed");
        return 2;
    }
    proto::writeFrame(stdout, {"READY", "0"});

    proto::Frame frame;
    while (proto::readFrame(stdin, frame)) {
        if (frame.fields.empty() || frame.fields[0] == "QUIT")
            break;
        if (frame.fields[0] == "TRANSCRIBE")
            handleTranscribe(context, options, frame);
        else
            sendError(frame.fields.size() > 1 ? frame.fields[1] : "0", "bad_request");
    }

    whisper_free(context);
    return 0;
}
