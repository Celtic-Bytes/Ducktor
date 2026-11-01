#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ducktor::seg {

struct Segment
{
    std::string prefix;
    std::string text;
    std::string suffix;
    bool translate = false;
};

inline bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
}

inline std::string trim(const std::string& s)
{
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && isSpace(s[begin]))
        ++begin;
    while (end > begin && isSpace(s[end - 1]))
        --end;
    return s.substr(begin, end - begin);
}

// True when position `end` (exclusive) closes a sentence.
inline bool endsSentence(const std::string& s, std::size_t end)
{
    if (end == 0)
        return false;
    const char c = s[end - 1];
    if ((c == '.' || c == '!' || c == '?' || c == ';' || c == ':') && (end == s.size() || isSpace(s[end])))
        return true;
    if (end >= 3) {
        const auto a = static_cast<unsigned char>(s[end - 3]);
        const auto b = static_cast<unsigned char>(s[end - 2]);
        const auto d = static_cast<unsigned char>(s[end - 1]);
        if (a == 0xE3 && b == 0x80 && d == 0x82)
            return true;
        if (a == 0xEF && b == 0xBC && (d == 0x81 || d == 0x9F || d == 0x9B))
            return true;
    }
    return false;
}

inline std::size_t utf8Boundary(const std::string& s, std::size_t pos, std::size_t floor)
{
    while (pos > floor && pos < s.size() && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80)
        --pos;
    return pos;
}

// Splits text into pieces of at most maxBytes, preferring sentence ends, then spaces.
inline std::vector<std::string> splitLong(const std::string& s, std::size_t maxBytes)
{
    if (maxBytes < 8)
        maxBytes = 8;
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start < s.size() && s.size() - start > maxBytes) {
        const std::size_t limit = start + maxBytes;
        const std::size_t floor = start + maxBytes / 3;
        std::size_t cut = 0;
        for (std::size_t i = limit; i > floor; --i) {
            if (endsSentence(s, i)) {
                cut = i;
                break;
            }
        }
        if (cut == 0) {
            for (std::size_t i = limit; i > floor; --i) {
                if (isSpace(s[i - 1])) {
                    cut = i;
                    break;
                }
            }
        }
        if (cut == 0)
            cut = utf8Boundary(s, limit, start + 1);
        std::string piece = trim(s.substr(start, cut - start));
        if (!piece.empty())
            parts.push_back(std::move(piece));
        start = cut;
        while (start < s.size() && isSpace(s[start]))
            ++start;
    }
    if (start < s.size()) {
        std::string piece = trim(s.substr(start));
        if (!piece.empty())
            parts.push_back(std::move(piece));
    }
    return parts;
}

// Splits text by lines, keeping layout (indentation, blank lines) outside the translated pieces.
inline std::vector<Segment> split(const std::string& text, std::size_t maxBytes)
{
    std::vector<Segment> out;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        const std::size_t newline = text.find('\n', pos);
        const bool last = newline == std::string::npos;
        const std::string line = text.substr(pos, (last ? text.size() : newline) - pos);

        std::size_t begin = 0;
        while (begin < line.size() && isSpace(line[begin]))
            ++begin;
        std::size_t end = line.size();
        while (end > begin && isSpace(line[end - 1]))
            --end;

        const std::string prefix = line.substr(0, begin);
        const std::string body = line.substr(begin, end - begin);
        const std::string trailing = line.substr(end) + (last ? "" : "\n");

        if (body.empty()) {
            if (!prefix.empty() || !trailing.empty())
                out.push_back({prefix, std::string(), trailing, false});
        } else if (body.size() <= maxBytes) {
            out.push_back({prefix, body, trailing, true});
        } else {
            const std::vector<std::string> parts = splitLong(body, maxBytes);
            for (std::size_t i = 0; i < parts.size(); ++i) {
                out.push_back({i == 0 ? prefix : std::string(), parts[i],
                               i + 1 == parts.size() ? trailing : std::string(" "), true});
            }
        }

        if (last)
            break;
        pos = newline + 1;
    }
    return out;
}

} // namespace ducktor::seg
