/**
 * PAL/Ø — Dense Glyph Instruction Set for the PAL compiler.
 *
 * This is NOT a human-readable language. It is a fixed-arity prefix
 * (Polish-notation) opcode stream designed for maximum token density when
 * emitted by an LLM:
 *
 *   - Every opcode is exactly ONE Unicode codepoint (one vocabulary entry
 *     once these glyphs are added as dedicated tokens to a model's
 *     tokenizer + embedding matrix).
 *   - Every opcode has a FIXED, statically known arity (0, 1, 2, or 3).
 *     There is therefore no need for parentheses, commas, whitespace,
 *     precedence tables, or associativity rules — the parser always knows
 *     exactly how many sub-expressions follow a glyph.
 *   - Variables are NOT named. Bound variables are referenced by de Bruijn
 *     index (glyph-encoded 0-9, escape+digits beyond that). Top-level
 *     definitions are referenced positionally (declaration order), also
 *     via glyph-encoded index. This removes identifier tokens entirely.
 *   - There is a single unified grammar: "type" opcodes and "expression"
 *     opcodes live in the same tree structure (ExprNode), distinguished
 *     only by which subset of the opcode space is used. A single
 *     recursive-descent function parses both.
 *
 * Why this is the right design for an LLM target language:
 *   - Prefix + fixed arity means correctness of a partially generated
 *     token stream is a strictly local property (each glyph token deterministically
 *     tells the model how many things must follow), which is exactly the kind of
 *     pattern transformer attention learns fastest and most robustly, versus infix
 *     notation with precedence, matched parens, and named identifiers requiring
 *     long-range agreement.
 *   - Removing names means the model never needs to "invent" or "remember" spellings
 *     of identifiers — it only needs to count binder depth, which is a much easier
 *     structural skill to fine-tune into 7B-scale attention/positional circuits.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace arllm {

// Reserved token ids for framing (below the opcode range).
enum class ReservedToken : int { PAD = 0, START = 1, END = 2, UNKNOWN = 3 };

// The complete opcode space. Expression opcodes and Type opcodes share one
// tree representation (ExprNode); they are simply disjoint subsets of Opcode.
enum class Opcode : int {
    // --- Leaves (arity 0) ---
    True, False, Nil, Var, GVar, Num,

    // --- Unary (arity 1) ---
    Neg, Not, Fix, Ret, Box, Deref, Lambda,

    // --- Binary (arity 2) ---
    Add, Sub, Mul, Div,
    And, Or,
    Eq, Neq, Lt, Le, Gt, Ge,
    Mem, NMem, SubsetEq, Subset, Union, Intersect, SetMinus,
    Apply, Compose, Let, Forall, Exists, Idx, Cons, TAnnot,

    // --- Ternary (arity 3) ---
    Cond, Iter,

    // --- Type opcodes (arity 0) ---
    TTop, TBot, TBool, TInt, TReal, TStr,
    // --- Type opcodes (arity 1) ---
    TArr, TPtr, TBox,
    // --- Type opcodes (arity 2) ---
    TFun, TProd, TUnion,
};

// A single vocabulary entry: exactly one opcode <-> exactly one glyph.
struct Symbol {
    std::string glyph;   // exact UTF-8 bytes for this token (1 Unicode codepoint)
    int token_id;        // stable token id (>= 4, after reserved tokens)
    std::string name;    // debug/mnemonic name, e.g. "ADD"
    Opcode op;
    int arity;
    long fixed_index;    // for Var/GVar glyphs: baked-in de Bruijn/global index.
                          // -1 means "escape": read an inline ASCII digit run instead.
};

class SymbolVocabulary {
public:
    SymbolVocabulary();

    std::optional<Symbol> get_symbol(const std::string& glyph) const;
    std::optional<Symbol> get_symbol_by_id(int token_id) const;
    static int arity_of(Opcode op);
    static bool is_type_opcode(Opcode op);

    const std::vector<Symbol>& all_symbols() const { return symbols; }
    size_t size() const { return symbols.size(); }

private:
    std::vector<Symbol> symbols;
    std::map<std::string, size_t> glyph_to_index;
    std::map<int, size_t> id_to_index;

    void init_symbols();
    void add(const std::string& glyph, const std::string& name, Opcode op, long fixed_index = 0);
};

} // namespace arllm
