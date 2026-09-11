#include "tokenizer.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>

namespace arllm {

Tokenizer::Tokenizer(const SymbolVocabulary& vocab)
    : vocab(vocab), pos(0), line(1), column(1) {}

std::optional<char> Tokenizer::peek(int offset) const {
    size_t idx = pos + offset;
    if (idx < input.size()) {
        return input[idx];
    }
    return std::nullopt;
}

char Tokenizer::advance() {
    if (pos >= input.size()) {
        throw std::runtime_error("Unexpected end of glyph stream");
    }
    char ch = input[pos];
    pos++;
    if (ch == '\n') { line++; column = 1; } else { column++; }
    return ch;
}

void Tokenizer::skip_whitespace() {
    // Development convenience only — never present in genuine dense output.
    while (peek() && std::isspace(static_cast<unsigned char>(*peek()))) {
        advance();
    }
}

size_t Tokenizer::utf8_seq_len(unsigned char lead) const {
    if ((lead & 0x80) == 0x00) return 1;  // 0xxxxxxx
    if ((lead & 0xE0) == 0xC0) return 2;  // 110xxxxx
    if ((lead & 0xF0) == 0xE0) return 3;  // 1110xxxx
    if ((lead & 0xF8) == 0xF0) return 4;  // 11110xxx
    return 1; // invalid lead byte; treat as 1 to avoid infinite loop
}

std::string Tokenizer::read_glyph() {
    if (pos >= input.size()) {
        throw std::runtime_error("Unexpected end of glyph stream");
    }
    size_t len = utf8_seq_len(static_cast<unsigned char>(input[pos]));
    len = std::min(len, input.size() - pos);
    std::string glyph = input.substr(pos, len);
    for (size_t i = 0; i < len; ++i) advance();
    return glyph;
}

std::string Tokenizer::read_ascii_digits() {
    std::string result;
    while (peek() && (std::isdigit(static_cast<unsigned char>(*peek())) || *peek() == '.')) {
        result += advance();
    }
    return result;
}

SourceLocation Tokenizer::current_location() const {
    return {line, column, "PAL/Ø"};
}

std::vector<Token> Tokenizer::tokenize(const std::string& src) {
    input = src;
    pos = 0;
    line = 1;
    column = 1;
    std::vector<Token> tokens;

    tokens.push_back({static_cast<int>(ReservedToken::START), Opcode::Nil, 0, 0.0, "<START>", current_location()});

    while (pos < input.size()) {
        skip_whitespace();
        if (pos >= input.size()) break;

        SourceLocation loc = current_location();
        std::string glyph = read_glyph();

        auto sym = vocab.get_symbol(glyph);
        if (!sym) {
            throw std::runtime_error(
                "Unknown glyph in PAL/O stream at line " + std::to_string(loc.line) +
                ", column " + std::to_string(loc.column));
        }

        Token tok{sym->token_id, sym->op, sym->fixed_index, 0.0, glyph, loc};

        if (sym->op == Opcode::Num) {
            // '#' escape: the following inline ASCII digit run is the value.
            std::string digits = read_ascii_digits();
            tok.value = digits.empty() ? 0.0 : std::stod(digits);
        } else if ((sym->op == Opcode::Var || sym->op == Opcode::GVar) && sym->fixed_index == -1) {
            // Escape glyph (ᵥ / ᴳ): the following inline ASCII digit run is the index.
            std::string digits = read_ascii_digits();
            tok.index = digits.empty() ? 0 : std::stol(digits);
        }

        tokens.push_back(tok);
    }

    tokens.push_back({static_cast<int>(ReservedToken::END), Opcode::Nil, 0, 0.0, "<END>", current_location()});
    return tokens;
}

} // namespace arllm
