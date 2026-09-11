/**
 * Parser for PAL/Ø.
 *
 * Because every opcode has a statically known, fixed arity (see
 * SymbolVocabulary::arity_of), parsing a prefix stream needs exactly one
 * recursive function: read a token, then recursively parse exactly
 * `arity(op)` children. There is no precedence climbing, no parenthesis
 * matching, and no separate grammar for the type sub-language — expression
 * opcodes and type opcodes are parsed by the very same function, since both
 * are just fixed-arity prefix trees over the same Opcode enum.
 */

#pragma once

#include "ast_nodes.h"
#include <vector>
#include <stdexcept>

namespace arllm {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, const SourceLocation& loc)
        : std::runtime_error(msg + " at line " + std::to_string(loc.line)) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Parse a complete program: a flat sequence of top-level expressions.
    Program parse();

private:
    const std::vector<Token>& tokens;
    size_t pos;

    bool at_end() const;
    const Token& advance();
    SourceLocation current_location() const;

    // The single recursive-descent rule for the entire language.
    ExprPtr parse_node();
};

} // namespace arllm

};

} // namespace arllm
