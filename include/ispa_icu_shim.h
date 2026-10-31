// Minimal replacement for the few ICU facilities ISPA uses, for builds
// (WebAssembly) where ICU is not available. Mirrors ICU's U8_NEXT and the
// Unicode names of printable ASCII; other code points have no name.
#pragma once
#include <cstdint>
#include <cstring>

using UChar32 = std::int32_t;

#define U8_NEXT(s, i, length, c) do {                                          \
    (c) = static_cast<std::uint8_t>((s)[(i)++]);                               \
    if ((c) >= 0x80) {                                                         \
        int ispa_n = (c) >= 0xF0 ? 3 : (c) >= 0xE0 ? 2 : (c) >= 0xC2 ? 1 : -1; \
        if (ispa_n < 0 || (i) + ispa_n > (length)) { (c) = -1; }               \
        else {                                                                 \
            UChar32 ispa_cp = (c) & (0x3F >> ispa_n);                          \
            for (int ispa_k = 0; ispa_k < ispa_n && (c) >= 0; ++ispa_k) {      \
                auto ispa_b = static_cast<std::uint8_t>((s)[(i)++]);           \
                if ((ispa_b & 0xC0) != 0x80) { (c) = -1; }                     \
                else ispa_cp = (ispa_cp << 6) | (ispa_b & 0x3F);               \
            }                                                                  \
            if ((c) >= 0) (c) = ispa_cp;                                       \
        }                                                                      \
    }                                                                          \
} while (0)

namespace ispa_icu_shim {
inline const char *asciiName(UChar32 cp) {
    static const char *const names[] = {
        "SPACE", "EXCLAMATION MARK", "QUOTATION MARK", "NUMBER SIGN", "DOLLAR SIGN",
        "PERCENT SIGN", "AMPERSAND", "APOSTROPHE", "LEFT PARENTHESIS", "RIGHT PARENTHESIS",
        "ASTERISK", "PLUS SIGN", "COMMA", "HYPHEN-MINUS", "FULL STOP", "SOLIDUS",
        "DIGIT ZERO", "DIGIT ONE", "DIGIT TWO", "DIGIT THREE", "DIGIT FOUR", "DIGIT FIVE",
        "DIGIT SIX", "DIGIT SEVEN", "DIGIT EIGHT", "DIGIT NINE", "COLON", "SEMICOLON",
        "LESS-THAN SIGN", "EQUALS SIGN", "GREATER-THAN SIGN", "QUESTION MARK", "COMMERCIAL AT"};
    static const char *const tail[] = {
        "LEFT SQUARE BRACKET", "REVERSE SOLIDUS", "RIGHT SQUARE BRACKET", "CIRCUMFLEX ACCENT",
        "LOW LINE", "GRAVE ACCENT"};
    static const char *const high[] = {"LEFT CURLY BRACKET", "VERTICAL LINE", "RIGHT CURLY BRACKET", "TILDE"};
    static char letter[40];
    if (cp >= 0x20 && cp <= 0x40) return names[cp - 0x20];
    if (cp >= 'A' && cp <= 'Z') { std::strcpy(letter, "LATIN CAPITAL LETTER _"); letter[21] = char(cp); return letter; }
    if (cp >= 0x5B && cp <= 0x60) return tail[cp - 0x5B];
    if (cp >= 'a' && cp <= 'z') { std::strcpy(letter, "LATIN SMALL LETTER _"); letter[19] = char(cp - 32); return letter; }
    if (cp >= 0x7B && cp <= 0x7E) return high[cp - 0x7B];
    return nullptr;
}
}
