#include "token_counter.h"
#include <cctype>
#include <string>

namespace pal {

// Helper to detect if a byte sequence starts a known PAL glyph
bool is_pal_glyph_start(char c) {
    // Common single-byte glyphs (ASCII)
    if (c == '+' || c == '-' || c == '*' || c == '/' ||
        c == '(' || c == ')' || c == ',' || c == '.' ||
        c == 'λ' || c == '∸' || c == '¬' || c == '×' || c == '÷' ||
        c == '>' || c == '<' || c == '=' || c == '!' || c == '#' ||
        c == '?' || c == ':' || c == '@') {
        return true;
    }
    // Multi-byte glyphs (UTF-8)
    if ((unsigned char)c >= 0xE2) { // likely UTF-8 start
        return true;
    }
    return false;
}

int count_pal_tokens(const std::string& stream) {
    int count = 0;
    for (size_t i = 0; i < stream.size(); ) {
        unsigned char c = (unsigned char)stream[i];

        // Single-byte ASCII glyphs
        if (c <= 127 && is_pal_glyph_start(c)) {
            ++count;
            ++i;
            continue;
        }

        // UTF-8 multi-byte sequences:
        // - 2-byte: starts with 110xxxxx → 2 bytes total
        // - 3-byte: starts with 1110xxxx → 3 bytes
        // - 4-byte: starts with 11110xxx → 4 bytes
        if (c >= 0xC2 && c <= 0xDF) { // 2-byte UTF-8
            ++count;
            i += 2;
        } else if (c >= 0xE0 && c <= 0xEF) { // 3-byte UTF-8
            ++count;
            i += 3;
        } else if (c >= 0xF0 && c <= 0xF4) { // 4-byte UTF-8
            ++count;
            i += 4;
        } else {
            // Skip invalid or non-glyph bytes
            ++i;
        }
    }
    return count;
}

} // namespace pal