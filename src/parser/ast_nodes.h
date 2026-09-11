/**
 * Unified AST node for PAL/Ø.
 *
 * A single node type (ExprNode) represents BOTH expression trees and type
 * trees — they are just disjoint subsets of the Opcode space sharing the
 * same fixed-arity prefix structure. This means there is exactly one
 * recursive-descent function in the parser (see parser.h), not two.
 *
 * Bound variables carry no names: `index` holds a de Bruijn index (for
 * Opcode::Var, counting outward from the nearest enclosing binder) or a
 * positional top-level definition index (for Opcode::GVar). `value` holds
 * the numeric payload for Opcode::Num.
 */

#pragma once

#include "../tokenizer/vocabulary.h"
#include "../tokenizer/tokenizer.h"
#include "../common/types.h"
#include <memory>
#include <string>
#include <vector>

namespace arllm {

class ExprNode;
using ExprPtr = std::shared_ptr<ExprNode>;

class ExprNode {
public:
    Opcode op;
    std::vector<ExprPtr> children; // size == SymbolVocabulary::arity_of(op)
    long index = 0;                // payload for Var / GVar
    double value = 0.0;            // payload for Num
    SourceLocation location{};

    ExprNode(Opcode o, SourceLocation loc) : op(o), location(loc) {}

    // Debug-only human-readable rendering (NOT part of the wire format).
    std::string to_string() const;
};

// A program is simply a sequence of top-level expressions in declaration
// order. There is no DEF marker glyph: since every opcode has a statically
// known arity, the parser always knows exactly where one definition's
// subtree ends, so the next token unambiguously starts the next one.
class Program {
public:
    explicit Program(std::vector<ExprPtr> defs) : definitions(std::move(defs)) {}

    std::string to_string() const;

    std::vector<ExprPtr> definitions;
};

// Bridges the "type" subset of the opcode tree to the reusable Type
// hierarchy in common/types.h (used by unify()/TypeEnvironment).
TypePtr node_to_type(const ExprNode& node);

} // namespace arllm

