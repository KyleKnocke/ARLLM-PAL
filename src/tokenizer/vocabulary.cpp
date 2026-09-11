#include "vocabulary.h"
#include <array>

namespace arllm {

SymbolVocabulary::SymbolVocabulary() {
    init_symbols();
}

void SymbolVocabulary::add(const std::string& glyph, const std::string& name, Opcode op, long fixed_index) {
    int token_id = static_cast<int>(ReservedToken::UNKNOWN) + 1 + static_cast<int>(symbols.size());
    Symbol sym{glyph, token_id, name, op, arity_of(op), fixed_index};
    size_t idx = symbols.size();
    symbols.push_back(sym);
    glyph_to_index[glyph] = idx;
    id_to_index[token_id] = idx;
}

void SymbolVocabulary::init_symbols() {
    // Initialize all symbols in fixed order.
    // Token IDs are assigned sequentially starting from ReservedToken::UNKNOWN + 1.
    // Do not reorder or insert new symbols without updating dependent logic.

    // --- Leaves (arity 0) ---
    add("𝟙", "TRUE", Opcode::True);
    add("𝟘", "FALSE", Opcode::False);
    add("∅", "NIL", Opcode::Nil);

    // Bound variable references: de Bruijn index 0-9 baked directly into the glyph.
    static const std::array<const char*, 10> var_glyphs = {
        "₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"
    };
    for (long i = 0; i < 10; ++i) {
        add(var_glyphs[i], "VAR" + std::to_string(i), Opcode::Var, i);
    }
    add("ᵥ", "VAR_ESC", Opcode::Var, -1); // followed by inline ASCII digits for index >= 10

    // Global (top-level definition) references, by declaration order.
    static const std::array<const char*, 10> gvar_glyphs = {
        "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"
    };
    for (long i = 0; i < 10; ++i) {
        add(gvar_glyphs[i], "GVAR" + std::to_string(i), Opcode::GVar, i);
    }
    add("ᴳ", "GVAR_ESC", Opcode::GVar, -1); // followed by inline ASCII digits for index >= 10

    add("#", "NUM", Opcode::Num, -1); // followed by inline ASCII digit run (+ optional '.')

    // --- Unary (arity 1) ---
    add("∸", "NEG", Opcode::Neg);
    add("¬", "NOT", Opcode::Not);
    add("↻", "FIX", Opcode::Fix);
    add("⏎", "RET", Opcode::Ret);
    add("▢", "BOX", Opcode::Box);
    add("⌾", "DEREF", Opcode::Deref);
    add("λ", "LAMBDA", Opcode::Lambda);

    // --- Binary (arity 2): arithmetic ---
    add("+", "ADD", Opcode::Add);
    add("−", "SUB", Opcode::Sub);
    add("×", "MUL", Opcode::Mul);
    add("÷", "DIV", Opcode::Div);

    // --- Binary: logical ---
    add("∧", "AND", Opcode::And);
    add("∨", "OR", Opcode::Or);

    // --- Binary: comparison ---
    add("=", "EQ", Opcode::Eq);
    add("≠", "NEQ", Opcode::Neq);
    add("<", "LT", Opcode::Lt);
    add("≤", "LE", Opcode::Le);
    add(">", "GT", Opcode::Gt);
    add("≥", "GE", Opcode::Ge);

    // --- Binary: set ---
    add("∈", "MEM", Opcode::Mem);
    add("∉", "NMEM", Opcode::NMem);
    add("⊆", "SUBSETEQ", Opcode::SubsetEq);
    add("⊂", "SUBSET", Opcode::Subset);
    add("∪", "UNION", Opcode::Union);
    add("∩", "INTERSECT", Opcode::Intersect);
    add("∖", "SETMINUS", Opcode::SetMinus);

    // --- Binary: control/binding/structural ---
    add("@", "APPLY", Opcode::Apply);
    add("∘", "COMPOSE", Opcode::Compose);
    add("≜", "LET", Opcode::Let);
    add("∀", "FORALL", Opcode::Forall);
    add("∃", "EXISTS", Opcode::Exists);
    add("‣", "IDX", Opcode::Idx);
    add("∷", "CONS", Opcode::Cons);
    add("⦂", "TANNOT", Opcode::TAnnot);

    // --- Ternary (arity 3) ---
    add("⁇", "COND", Opcode::Cond);
    add("⟳", "ITER", Opcode::Iter);

    // --- Type opcodes (arity 0) ---
    add("⊤", "T_TOP", Opcode::TTop);
    add("⊥", "T_BOT", Opcode::TBot);
    add("𝔹", "T_BOOL", Opcode::TBool);
    add("ℤ", "T_INT", Opcode::TInt);
    add("ℝ", "T_REAL", Opcode::TReal);
    add("𝕾", "T_STR", Opcode::TStr);

    // --- Type opcodes (arity 1) ---
    add("⟨", "T_ARR", Opcode::TArr);
    add("⟡", "T_PTR", Opcode::TPtr);
    add("⧈", "T_BOX", Opcode::TBox);

    // --- Type opcodes (arity 2) ---
    add("→", "T_FUN", Opcode::TFun);
    add("⨯", "T_PROD", Opcode::TProd);
    add("⋃", "T_UNION", Opcode::TUnion);
}

int SymbolVocabulary::arity_of(Opcode op) {
    switch (op) {
        // Leaves
        case Opcode::True: case Opcode::False: case Opcode::Nil:
        case Opcode::Var: case Opcode::GVar: case Opcode::Num:
        case Opcode::TTop: case Opcode::TBot: case Opcode::TBool:
        case Opcode::TInt: case Opcode::TReal: case Opcode::TStr:
            return 0;
        // Unary
        case Opcode::Neg: case Opcode::Not: case Opcode::Fix:
        case Opcode::Ret: case Opcode::Box: case Opcode::Deref:
        case Opcode::Lambda:
        case Opcode::TArr: case Opcode::TPtr: case Opcode::TBox:
            return 1;
        // Binary
        case Opcode::Add: case Opcode::Sub: case Opcode::Mul: case Opcode::Div:
        case Opcode::And: case Opcode::Or:
        case Opcode::Eq: case Opcode::Neq: case Opcode::Lt: case Opcode::Le:
        case Opcode::Gt: case Opcode::Ge:
        case Opcode::Mem: case Opcode::NMem: case Opcode::SubsetEq: case Opcode::Subset:
        case Opcode::Union: case Opcode::Intersect: case Opcode::SetMinus:
        case Opcode::Apply: case Opcode::Compose: case Opcode::Let:
        case Opcode::Forall: case Opcode::Exists: case Opcode::Idx: case Opcode::Cons:
        case Opcode::TAnnot:
        case Opcode::TFun: case Opcode::TProd: case Opcode::TUnion:
            return 2;
        // Ternary
        case Opcode::Cond: case Opcode::Iter:
            return 3;
    }
    return 0;
}

bool SymbolVocabulary::is_type_opcode(Opcode op) {
    switch (op) {
        case Opcode::TTop: case Opcode::TBot: case Opcode::TBool:
        case Opcode::TInt: case Opcode::TReal: case Opcode::TStr:
        case Opcode::TArr: case Opcode::TPtr: case Opcode::TBox:
        case Opcode::TFun: case Opcode::TProd: case Opcode::TUnion:
            return true;
        default:
            return false;
    }
}

std::optional<Symbol> SymbolVocabulary::get_symbol(const std::string& glyph) const {
    auto it = glyph_to_index.find(glyph);
    if (it != glyph_to_index.end()) {
        return symbols[it->second];
    }
    return std::nullopt;
}

std::optional<Symbol> SymbolVocabulary::get_symbol_by_id(int token_id) const {
    auto it = id_to_index.find(token_id);
    if (it != id_to_index.end()) {
        return symbols[it->second];
    }
    return std::nullopt;
}

} // namespace arllm

