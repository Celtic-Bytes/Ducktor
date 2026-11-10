// Spell-check helper: serves SPELLCHECK requests on stdin with Hunspell and the bundled
// dictionaries. The dictionaries are validated by Ducktor itself, so no network access is
// ever needed.
#include "hunspell.hxx"
#include "protocol.hpp"
#include "spellcheck.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace ducktor;

// Mirrors AppSettings::kHardMaxInputChars: the interface enforces the same limit, this is
// the second line of defense before any dictionary work starts.
constexpr std::size_t kMaxTextUtf16Units = 5000;
constexpr std::size_t kMaxTextBytes = 20 * 1024; // 5000 UTF-16 units encode to at most 20 KB of UTF-8
constexpr std::size_t kMaxTokenUtf16Units = 64;  // longer runs are not worth suggesting on
constexpr int kMaxSuggestions = 8;

struct Options
{
    std::string dataDir;
};

void sendError(const std::string& id, const std::string& code, const std::string& message = std::string())
{
    proto::writeText(stdout, {"ERROR", id, code}, message);
}

bool parseOptions(int argc, char** argv, Options& options)
{
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string key = argv[i];
        const std::string value = argv[i + 1];
        if (key == "--data")
            options.dataDir = value;
        else
            return false;
    }
    return !options.dataDir.empty();
}

// Language code -> dictionary file stem in the data directory.
const std::map<std::string, std::string>& dictionaryStems()
{
    static const std::map<std::string, std::string> stems{{"es", "es_ES"},
                                                          {"en", "en_US"},
                                                          {"fr", "fr"},
                                                          {"de", "de_DE_frami"},
                                                          {"it", "it_IT"}};
    return stems;
}

std::string jsonEscape(const std::string& text)
{
    std::string out;
    out.reserve(text.size() + 8);
    for (const unsigned char c : text) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                out += buffer;
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    return out;
}

class Checker
{
public:
    explicit Checker(std::string dataDir) : m_dataDir(std::move(dataDir)) {}

    // Returns the checker for `language`, loading it on first use. Sends an ERROR frame and
    // returns false when the language or its dictionary cannot be used.
    bool checkerFor(const std::string& language, const std::string& id, Hunspell** out)
    {
        const auto stem = dictionaryStems().find(language);
        if (stem == dictionaryStems().end()) {
            sendError(id, "bad_language");
            return false;
        }
        const auto loaded = m_checkers.find(language);
        if (loaded != m_checkers.end()) {
            *out = loaded->second.get();
            return true;
        }
        const std::string aff = m_dataDir + "/" + stem->second + ".aff";
        const std::string dic = m_dataDir + "/" + stem->second + ".dic";
        std::ifstream probe(aff);
        if (!probe.good() || !std::ifstream(dic).good()) {
            sendError(id, "dict_load_failed");
            return false;
        }
        auto checker = std::make_unique<Hunspell>(aff.c_str(), dic.c_str());
        *out = checker.get();
        m_checkers.emplace(language, std::move(checker));
        return true;
    }

private:
    std::string m_dataDir;
    std::map<std::string, std::unique_ptr<Hunspell>> m_checkers;
};

void handleSpellCheck(Checker& checker, const proto::Frame& frame)
{
    const std::string id = frame.fields.size() > 1 ? frame.fields[1] : "0";
    const std::string language = frame.fields.size() > 2 ? frame.fields[2] : "";
    const std::string& text = frame.payload;
    if (language.empty() || text.empty()) {
        sendError(id, "bad_request");
        return;
    }
    if (text.size() > kMaxTextBytes || spell::utf16Length(text) > kMaxTextUtf16Units) {
        sendError(id, "too_long");
        return;
    }

    Hunspell* hunspell = nullptr;
    if (!checker.checkerFor(language, id, &hunspell))
        return;

    const std::vector<spell::Token> tokens = spell::tokenize(text);

    std::string json = "{\"issues\":[";

    bool first = true;
    for (const spell::Token& token : tokens) {
        if (token.utf16Length() > kMaxTokenUtf16Units)
            continue;
        const std::string word = text.substr(token.utf8Start, token.utf8Length());
        if (hunspell->spell(word))
            continue;
        std::vector<std::string> suggestions = hunspell->suggest(word);
        if (suggestions.size() > kMaxSuggestions)
            suggestions.resize(kMaxSuggestions);

        if (!first)
            json += ",";
        first = false;
        json += "{\"word\":\"" + jsonEscape(word) + "\"";
        json += ",\"start\":" + std::to_string(token.utf16Start);
        json += ",\"end\":" + std::to_string(token.utf16End);
        json += ",\"suggestions\":[";
        for (std::size_t i = 0; i < suggestions.size(); ++i) {
            if (i > 0)
                json += ",";
            json += "\"" + jsonEscape(suggestions[i]) + "\"";
        }
        json += "]}";
    }
    json += "]}";
    proto::writeText(stdout, {"RESULT", id}, json);
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
    if (!std::filesystem::is_directory(options.dataDir)) {
        sendError("0", "missing_data");
        return 2;
    }

    Checker checker(options.dataDir);
    proto::writeFrame(stdout, {"READY", "0"});

    proto::Frame frame;
    while (proto::readFrame(stdin, frame)) {
        if (frame.fields.empty() || frame.fields[0] == "QUIT")
            break;
        if (frame.fields[0] == "SPELLCHECK")
            handleSpellCheck(checker, frame);
        else
            sendError(frame.fields.size() > 1 ? frame.fields[1] : "0", "bad_request");
    }
    return 0;
}
