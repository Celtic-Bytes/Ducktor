// Framing shared by the engine helpers: "FIELD FIELD ... <payloadBytes>\n" followed by the raw payload.
#pragma once

#include <cstddef>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace ducktor::proto {

constexpr std::size_t kMaxHeaderBytes = 256;
constexpr std::size_t kMaxPayloadBytes = 32u * 1024u * 1024u;

struct Frame
{
    std::vector<std::string> fields;
    std::string payload;
};

inline void setBinaryStdio()
{
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
}

inline bool parseSize(const std::string& text, std::size_t& value)
{
    if (text.empty() || text.size() > 12)
        return false;
    std::size_t result = 0;
    for (const char c : text) {
        if (c < '0' || c > '9')
            return false;
        result = result * 10 + static_cast<std::size_t>(c - '0');
    }
    value = result;
    return true;
}

inline bool readFrame(std::FILE* in, Frame& frame)
{
    std::string header;
    int c = 0;
    while ((c = std::fgetc(in)) != EOF && c != '\n') {
        if (header.size() >= kMaxHeaderBytes)
            return false;
        header.push_back(static_cast<char>(c));
    }
    if (c == EOF)
        return false;

    frame.fields.clear();
    std::size_t start = 0;
    while (start <= header.size()) {
        const std::size_t end = header.find(' ', start);
        const std::size_t stop = end == std::string::npos ? header.size() : end;
        if (stop > start)
            frame.fields.push_back(header.substr(start, stop - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }

    std::size_t size = 0;
    if (frame.fields.size() < 2 || !parseSize(frame.fields.back(), size) || size > kMaxPayloadBytes)
        return false;
    frame.fields.pop_back();
    frame.payload.assign(size, '\0');
    return size == 0 || std::fread(&frame.payload[0], 1, size, in) == size;
}

inline void writeFrame(std::FILE* out, std::initializer_list<std::string> fields,
                       const void* data = nullptr, std::size_t size = 0)
{
    std::string header;
    for (const std::string& field : fields) {
        header += field;
        header += ' ';
    }
    header += std::to_string(size);
    header += '\n';
    std::fwrite(header.data(), 1, header.size(), out);
    if (size > 0)
        std::fwrite(data, 1, size, out);
    std::fflush(out);
}

inline void writeText(std::FILE* out, std::initializer_list<std::string> fields, const std::string& text)
{
    writeFrame(out, fields, text.data(), text.size());
}

} // namespace ducktor::proto
