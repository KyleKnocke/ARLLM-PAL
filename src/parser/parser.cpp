#include "parser.h"

namespace arllm {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), pos(0) {}

bool Parser::at_end() const {
    return pos >= tokens.size() || tokens[pos].id == static_cast<int>(ReservedToken::END);
}

const Token& Parser::advance() {
    if (pos >= tokens.size()) {
        throw ParseError("Unexpected end of token stream", current_location());
    }
    return tokens[pos++];
}

SourceLocation Parser::current_location() const {
    if (pos < tokens.size()) return tokens[pos].location;
    return {0, 0, "PAL/Ø"};
}

// The entire grammar in one function: read a token, then recursively parse
// exactly `arity(op)` children. Prefix notation + fixed arity means this
// never needs lookahead, backtracking, or precedence handling.
ExprPtr Parser::parse_node() {
    if (pos < tokens.size() && tokens[pos].id == static_cast<int>(ReservedToken::START)) {
        advance(); // skip synthetic START marker
    }
    if (at_end()) {
        throw ParseError("Unexpected end of token stream while parsing a node", current_location());
    }
    const Token& tok = advance();

    auto node = std::make_shared<ExprNode>(tok.op, tok.location);
    node->index = tok.index;
    node->value = tok.value;

    int arity = SymbolVocabulary::arity_of(tok.op);
    for (int i = 0; i < arity; ++i) {
        node->children.push_back(parse_node());
    }
    return node;
}

Program Parser::parse() {
    std::vector<ExprPtr> definitions;
    while (!at_end()) {
        if (pos < tokens.size() && tokens[pos].id == static_cast<int>(ReservedToken::START)) {
            advance();
            continue;
        }
        definitions.push_back(parse_node());
    }
    return Program(definitions);
}

} // namespace arllm
