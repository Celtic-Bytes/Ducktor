// Translation helper: loads one GGUF model with llama.cpp and serves TRANSLATE requests on stdin.
#include "llama.h"
#include "protocol.hpp"
#include "segmenter.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

using namespace ducktor;

constexpr std::size_t kMaxTextBytes = 64 * 1024;
constexpr std::size_t kChunkBytes = 1500;
constexpr int kMaxSplitDepth = 6;

struct Options
{
    std::string model;
    int contextSize = 4096;
    int threads = 4;
    std::string promptLanguage = "en";
    std::string sampling = "greedy";
    bool noThink = false;
};

struct ChunkResult
{
    std::string text;
    bool complete = true;
    std::string error;
};

void quietLog(ggml_log_level, const char*, void*) {}

void sendError(const std::string& id, const std::string& code, const std::string& message = std::string())
{
    proto::writeText(stdout, {"ERROR", id, code}, message);
}

bool parseOptions(int argc, char** argv, Options& options)
{
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string key = argv[i];
        const std::string value = argv[i + 1];
        if (key == "--model")
            options.model = value;
        else if (key == "--ctx")
            options.contextSize = std::clamp(std::atoi(value.c_str()), 1024, 16384);
        else if (key == "--threads")
            options.threads = std::clamp(std::atoi(value.c_str()), 1, 64);
        else if (key == "--prompt-language")
            options.promptLanguage = value;
        else if (key == "--sampling")
            options.sampling = value;
        else if (key == "--no-think")
            options.noThink = value == "1";
        else
            return false;
    }
    return !options.model.empty();
}

std::string buildInstruction(const Options& options, const std::string& source, const std::string& target,
                             const std::string& text)
{
    if (options.promptLanguage == "zh") {
        const std::string from = source.empty() ? std::string() : source;
        return "请将以下" + from + "文本翻译为" + target + "，直接输出翻译结果，不要进行任何解释。\n\n" + text;
    }
    return "Translate the following text into " + target +
           ". Note that you should only output the translated result without any additional explanation:\n\n" + text;
}

std::string stripReasoning(std::string text)
{
    const std::size_t close = text.find("</think>");
    if (close != std::string::npos)
        text.erase(0, close + std::strlen("</think>"));
    const std::size_t open = text.find("<think>");
    if (open != std::string::npos)
        text.erase(open);
    return seg::trim(text);
}

class Translator
{
public:
    Translator(const Options& options, llama_model* model, llama_context* context, llama_sampler* sampler)
        : m_options(options), m_model(model), m_context(context), m_sampler(sampler),
          m_vocab(llama_model_get_vocab(model))
    {
    }

    ChunkResult translate(const std::string& source, const std::string& target, const std::string& text, int depth)
    {
        std::string prompt;
        if (!applyTemplate(buildInstruction(m_options, source, target, text), prompt))
            return {{}, false, "unsupported_template"};

        std::vector<llama_token> tokens;
        if (!tokenize(prompt, tokens))
            return {{}, false, "decode_failed"};

        const int contextSize = static_cast<int>(llama_n_ctx(m_context));
        const int promptTokens = static_cast<int>(tokens.size());
        const int maxNew = std::min(2048, std::max(128, promptTokens * 2));
        if (promptTokens + maxNew > contextSize) {
            if (depth >= kMaxSplitDepth || text.size() < 64)
                return {{}, false, "too_long"};
            return translateParts(source, target, seg::splitLong(text, text.size() / 2 + 1), depth + 1);
        }
        return generate(tokens, maxNew);
    }

private:
    ChunkResult translateParts(const std::string& source, const std::string& target,
                               const std::vector<std::string>& parts, int depth)
    {
        ChunkResult result;
        for (std::size_t i = 0; i < parts.size(); ++i) {
            ChunkResult part = translate(source, target, parts[i], depth);
            if (!part.error.empty())
                return part;
            result.complete = result.complete && part.complete;
            if (i > 0)
                result.text += ' ';
            result.text += part.text;
        }
        return result;
    }

    bool applyTemplate(const std::string& content, std::string& prompt)
    {
        const char* tmpl = llama_model_chat_template(m_model, nullptr);
        if (!tmpl)
            return false;
        const llama_chat_message message{"user", content.c_str()};
        std::vector<char> buffer(content.size() * 2 + 1024);
        int size = llama_chat_apply_template(tmpl, &message, 1, true, buffer.data(), static_cast<int>(buffer.size()));
        if (size > static_cast<int>(buffer.size())) {
            buffer.resize(static_cast<std::size_t>(size));
            size = llama_chat_apply_template(tmpl, &message, 1, true, buffer.data(), static_cast<int>(buffer.size()));
        }
        if (size < 0)
            return false;
        prompt.assign(buffer.data(), static_cast<std::size_t>(size));
        if (m_options.noThink)
            prompt += "<think>\n\n</think>\n\n";
        return true;
    }

    bool tokenize(const std::string& text, std::vector<llama_token>& tokens)
    {
        const int needed = -llama_tokenize(m_vocab, text.c_str(), static_cast<int32_t>(text.size()), nullptr, 0,
                                           true, true);
        if (needed <= 0)
            return false;
        tokens.resize(static_cast<std::size_t>(needed));
        return llama_tokenize(m_vocab, text.c_str(), static_cast<int32_t>(text.size()), tokens.data(), needed, true,
                              true) == needed;
    }

    ChunkResult generate(std::vector<llama_token>& tokens, int maxNew)
    {
        llama_memory_clear(llama_get_memory(m_context), true);
        llama_sampler_reset(m_sampler);

        const int batchSize = static_cast<int>(llama_n_batch(m_context));
        for (std::size_t i = 0; i < tokens.size(); i += static_cast<std::size_t>(batchSize)) {
            const int count = static_cast<int>(std::min<std::size_t>(batchSize, tokens.size() - i));
            if (llama_decode(m_context, llama_batch_get_one(tokens.data() + i, count)) != 0)
                return {{}, false, "decode_failed"};
        }

        ChunkResult result;
        result.complete = false;
        for (int i = 0; i < maxNew; ++i) {
            llama_token token = llama_sampler_sample(m_sampler, m_context, -1);
            if (llama_vocab_is_eog(m_vocab, token)) {
                result.complete = true;
                break;
            }
            char piece[256];
            const int n = llama_token_to_piece(m_vocab, token, piece, sizeof(piece), 0, false);
            if (n > 0) {
                result.text.append(piece, static_cast<std::size_t>(n));
            } else if (n < 0) {
                std::string large(static_cast<std::size_t>(-n), '\0');
                llama_token_to_piece(m_vocab, token, &large[0], -n, 0, false);
                result.text += large;
            }
            if (llama_decode(m_context, llama_batch_get_one(&token, 1)) != 0)
                return {{}, false, "decode_failed"};
        }
        result.text = stripReasoning(result.text);
        return result;
    }

    const Options& m_options;
    llama_model* m_model;
    llama_context* m_context;
    llama_sampler* m_sampler;
    const llama_vocab* m_vocab;
};

std::vector<std::string> splitFields(const std::string& payload, char separator)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = payload.find(separator, start);
        if (pos == std::string::npos || parts.size() == 2) {
            parts.push_back(payload.substr(start));
            break;
        }
        parts.push_back(payload.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

void handleTranslate(Translator& translator, const proto::Frame& frame)
{
    const std::string id = frame.fields.size() > 1 ? frame.fields[1] : "0";
    const std::vector<std::string> parts = splitFields(frame.payload, '\x1f');
    if (parts.size() != 3 || parts[1].empty()) {
        sendError(id, "bad_request");
        return;
    }
    const std::string& source = parts[0];
    const std::string& target = parts[1];
    const std::string& text = parts[2];
    if (text.size() > kMaxTextBytes) {
        sendError(id, "too_long");
        return;
    }

    const std::vector<seg::Segment> segments = seg::split(text, kChunkBytes);
    const auto total = std::count_if(segments.begin(), segments.end(), [](const seg::Segment& s) { return s.translate; });
    int done = 0;
    bool complete = true;
    std::string output;
    for (const seg::Segment& segment : segments) {
        output += segment.prefix;
        if (segment.translate) {
            const ChunkResult result = translator.translate(source, target, segment.text, 0);
            if (!result.error.empty()) {
                sendError(id, result.error);
                return;
            }
            complete = complete && result.complete;
            output += result.text;
            ++done;
            proto::writeFrame(stdout, {"PROGRESS", id, std::to_string(done), std::to_string(total)});
        }
        output += segment.suffix;
    }
    proto::writeText(stdout, {"RESULT", id, complete ? "1" : "0"}, output);
}

llama_sampler* createSampler(const Options& options)
{
    llama_sampler* chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
    if (options.sampling == "hymt2") {
        llama_sampler_chain_add(chain, llama_sampler_init_top_k(20));
        llama_sampler_chain_add(chain, llama_sampler_init_top_p(0.6f, 1));
        llama_sampler_chain_add(chain, llama_sampler_init_temp(0.7f));
        llama_sampler_chain_add(chain, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    } else {
        llama_sampler_chain_add(chain, llama_sampler_init_greedy());
    }
    return chain;
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

    llama_log_set(quietLog, nullptr);
    llama_backend_init();

    llama_model_params modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = 0;
    llama_model* model = llama_model_load_from_file(options.model.c_str(), modelParams);
    if (!model) {
        sendError("0", "load_failed");
        llama_backend_free();
        return 2;
    }

    llama_context_params contextParams = llama_context_default_params();
    contextParams.n_ctx = static_cast<uint32_t>(options.contextSize);
    contextParams.n_batch = static_cast<uint32_t>(options.contextSize);
    contextParams.n_threads = options.threads;
    contextParams.n_threads_batch = options.threads;
    contextParams.no_perf = true;
    llama_context* context = llama_init_from_model(model, contextParams);
    if (!context) {
        sendError("0", "load_failed");
        llama_model_free(model);
        llama_backend_free();
        return 2;
    }

    llama_sampler* sampler = createSampler(options);
    Translator translator(options, model, context, sampler);
    proto::writeFrame(stdout, {"READY", "0"});

    proto::Frame frame;
    while (proto::readFrame(stdin, frame)) {
        if (frame.fields.empty() || frame.fields[0] == "QUIT")
            break;
        if (frame.fields[0] == "TRANSLATE")
            handleTranslate(translator, frame);
        else
            sendError(frame.fields.size() > 1 ? frame.fields[1] : "0", "bad_request");
    }

    llama_sampler_free(sampler);
    llama_free(context);
    llama_model_free(model);
    llama_backend_free();
    return 0;
}
