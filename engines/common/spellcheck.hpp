// Word tokenization for the spell-check helper: a word is a maximal run of letters,
// with ASCII apostrophes and hyphens kept between letters ("co-autor" and "l'ortografía"
// stay one word). Tokens carry offsets in both UTF-8 bytes and UTF-16 code units,
// because the Hunspell dictionary works on UTF-8 while QML's text.length counts UTF-16.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ducktor::spell {

struct Token
{
    std::size_t utf8Start = 0;
    std::size_t utf8End = 0;
    std::size_t utf16Start = 0;
    std::size_t utf16End = 0;

    std::size_t utf8Length() const { return utf8End - utf8Start; }
    std::size_t utf16Length() const { return utf16End - utf16Start; }
};

// Letters in the scripts Ducktor's bundled dictionaries cover (Latin, including all its
// extensions) plus the other major writing systems, so text pasted in any language is
// tokenized sanely. Characters outside the table simply split a run.
inline bool isLetter(uint32_t cp)
{
    return (cp >= 0x0041 && cp <= 0x005A) // A-Z
           || (cp >= 0x0061 && cp <= 0x007A) // a-z
           || (cp >= 0x00C0 && cp <= 0x00D6) // Latin-1, before ÷
           || (cp >= 0x00D8 && cp <= 0x00F6)
           || (cp >= 0x00F8 && cp <= 0x00FF)
           || (cp >= 0x0100 && cp <= 0x024F) // Latin Extended-A/B
           || (cp >= 0x0370 && cp <= 0x0374) // Greek
           || (cp >= 0x0376 && cp <= 0x0377)
           || (cp >= 0x037A && cp <= 0x037F)
           || (cp == 0x0386)
           || (cp >= 0x0388 && cp <= 0x038A)
           || (cp == 0x038C)
           || (cp >= 0x038E && cp <= 0x03A1)
           || (cp >= 0x03A3 && cp <= 0x03CE)
           || (cp >= 0x03D0 && cp <= 0x03D7)
           || (cp >= 0x03DA && cp <= 0x03F3)
           || (cp >= 0x0400 && cp <= 0x0483) // Cyrillic
           || (cp >= 0x048D && cp <= 0x052F)
           || (cp >= 0x0531 && cp <= 0x0556) // Armenian
           || (cp == 0x0559)
           || (cp >= 0x0561 && cp <= 0x0587)
           || (cp >= 0x05D0 && cp <= 0x05EA) // Hebrew
           || (cp >= 0x05F0 && cp <= 0x05F2)
           || (cp >= 0x0620 && cp <= 0x064A) // Arabic
           || (cp >= 0x0671 && cp <= 0x06D3)
           || (cp >= 0x0900 && cp <= 0x097F) // Devanagari
           || (cp >= 0x0E00 && cp <= 0x0E7F) // Thai
           || (cp >= 0x1100 && cp <= 0x11FF) // Hangul Jamo
           || (cp >= 0x3040 && cp <= 0x30FF) // Hiragana and Katakana
           || (cp >= 0x3100 && cp <= 0x312F)
           || (cp >= 0x4E00 && cp <= 0x9FFF) // CJK unified ideographs
           || (cp >= 0xAC00 && cp <= 0xD7A3) // Hangul syllables
           || (cp >= 0xF900 && cp <= 0xFAFF) // CJK compatibility ideographs
           || (cp >= 0xFF66 && cp <= 0xFF9F) // halfwidth Katakana
           || (cp >= 0xFFA0 && cp <= 0xFFDC) // fullwidth Latin
           || (cp >= 0x2C00 && cp <= 0x2C2E) // Glagolitic
           || (cp >= 0x2C30 && cp <= 0x2C5E)
           || (cp >= 0x2C60 && cp <= 0x2CE4)
           || (cp >= 0x2CEB && cp <= 0x2CF3)
           || (cp >= 0x2D00 && cp <= 0x2D25); // Tifinagh
}

// Number of UTF-16 code units a UTF-8 string occupies (surrogate pairs count as two).
inline std::size_t utf16Length(const std::string& text)
{
    std::size_t units = 0;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        std::size_t len = 1;
        uint32_t cp = 0;
        if (c < 0x80) {
            cp = c;
        } else if ((c >> 5) == 0x6 && i + 1 < text.size()) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c >> 4) == 0xE && i + 2 < text.size()) {
            cp = c & 0x0F;
            len = 3;
        } else if ((c >> 3) == 0x1E && i + 3 < text.size()) {
            cp = c & 0x07;
            len = 4;
        } else {
            ++i; // invalid byte: not a code point
            continue;
        }
        bool valid = true;
        for (std::size_t k = 1; k < len; ++k) {
            const unsigned char cont = static_cast<unsigned char>(text[i + k]);
            if ((cont >> 6) != 0x2) {
                valid = false;
                break;
            }
            cp = (cp << 6) | (cont & 0x3F);
        }
        if (valid && !(cp >= 0xD800 && cp <= 0xDFFF))
            units += cp > 0xFFFF ? 2 : 1;
        i += len;
    }
    return units;
}

// All word tokens in `text` (UTF-8), in reading order.
inline std::vector<Token> tokenize(const std::string& text)
{
    struct CodePoint
    {
        uint32_t value;
        std::size_t utf8;
        std::size_t utf16;
    };
    std::vector<CodePoint> points;
    std::size_t utf16 = 0;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        std::size_t len = 1;
        uint32_t cp = 0;
        if (c < 0x80) {
            cp = c;
        } else if ((c >> 5) == 0x6 && i + 1 < text.size()) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c >> 4) == 0xE && i + 2 < text.size()) {
            cp = c & 0x0F;
            len = 3;
        } else if ((c >> 3) == 0x1E && i + 3 < text.size()) {
            cp = c & 0x07;
            len = 4;
        } else {
            ++i;
            continue;
        }
        bool valid = true;
        for (std::size_t k = 1; k < len; ++k) {
            const unsigned char cont = static_cast<unsigned char>(text[i + k]);
            if ((cont >> 6) != 0x2) {
                valid = false;
                break;
            }
            cp = (cp << 6) | (cont & 0x3F);
        }
        if (!valid || (cp >= 0xD800 && cp <= 0xDFFF)) {
            i += valid ? len : 1;
            continue;
        }
        points.push_back({cp, i, utf16});
        utf16 += cp > 0xFFFF ? 2 : 1;
        i += len;
    }

    std::vector<Token> tokens;
    for (std::size_t i = 0; i < points.size();) {
        if (!isLetter(points[i].value)) {
            ++i;
            continue;
        }
        std::size_t start = i;
        std::size_t j = i;
        while (j + 1 < points.size()) {
            const uint32_t next = points[j + 1].value;
            if (isLetter(next)) {
                ++j;
            } else if ((next == 0x27 || next == 0x2D) && j + 2 < points.size()
                       && isLetter(points[j + 2].value)) {
                j += 2; // keep the apostrophe/hyphen and the letter after it
            } else {
                break;
            }
        }
        Token token;
        token.utf8Start = points[start].utf8;
        token.utf16Start = points[start].utf16;
        if (j + 1 < points.size()) {
            token.utf8End = points[j + 1].utf8;
            token.utf16End = points[j + 1].utf16;
        } else {
            token.utf8End = text.size();
            token.utf16End = utf16;
        }
        tokens.push_back(token);
        i = j + 1;
    }
    return tokens;
}

} // namespace ducktor::spell
