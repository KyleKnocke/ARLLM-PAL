/**
 * Tokenizer for PAL/Ø.
 *
 * The wire format has NO whitespace, NO keywords, NO identifiers, and NO
 * comments: it is a raw concatenation of one-glyph opcodes (see
 * vocabulary.h). Tokenizing is therefore just "read the next UTF-8
 * codepoint, look it up in the opcode table", plus two escape forms that
 * consume an inline ASCII digit run as a payload:
 *   - '#' (NUM)      -> reads following digits (+ optional '.') as a value
 *   - 'ᵥ' (VAR_ESC)  -> reads following digits as a de Bruijn index >= 10
 *   - 'ᴳ' (GVAR_ESC) -> reads following digits as a global index >= 10
 *
 * ASCII whitespace is skipped only as a development convenience (to allow
 * hand-written test fixtures to be laid out readably); it never appears in
 * genuine model output and carries no meaning.
 */

#pragma once

#include "vocabulary.h"
#include <string>
#include <vector>
#include <optional>

namespace arllm {

struct SourceLocation {
    int line;
    int column;
    std::string file;
};

struct Token {
    int id;
    Opcode op;
    long index;     // de Bruijn / global index payload (Var, GVar)
    double value;   // numeric payload (Num)
    std::string glyph;
    SourceLocation location;
};

class Tokenizer {
public:
    Tokenizer(const SymbolVocabulary& vocab = SymbolVocabulary());
    
    // Tokenize input string
    std::vector<Token> tokenize(const std::string& input);
    
private:
    const SymbolVocabulary& vocab;
    std::string input;
    size_t pos;
    int line;
    int column;
    
    std::optional<char> peek(int offset = 0) const;
    char advance();
    void skip_whitespace();
    size_t utf8_seq_len(unsigned char lead) const;
    std::string read_glyph();       // consumes and returns the next full UTF-8 codepoint
    std::string read_ascii_digits(); // consumes a run of ASCII digits (+ optional '.')
    SourceLocation current_location() const;
};

} // namespace arllm
