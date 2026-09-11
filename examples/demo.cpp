/**
 * Demo: PAL/Ø — dense glyph bootstrap.
 *
 * Shows the full pipeline over a genuinely dense (non-human-readable) wire
 * format: raw UTF-8 glyph stream -> tokens -> AST -> type inference, plus a
 * density comparison against an equivalent human-readable source form.
 */

#include <iostream>
#include <iomanip>
#include <memory>

#include "tokenizer/tokenizer.h"
#include "tokenizer/vocabulary.h"
#include "parser/parser.h"
#include "parser/ast_nodes.h"
#include "type_system/type_checker.h"
#include "gimple/builder.h"
#include "common/types.h"

using namespace arllm;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(64, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(64, '=') << "\n";
}

// Counts Unicode codepoints in a UTF-8 string (a rough proxy for "token
// count once these glyphs are registered as single tokens").
size_t utf8_codepoint_count(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = s[i];
        size_t len = 1;
        if ((c & 0x80) == 0x00) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        i += len;
        count++;
    }
    return count;
}

void demo_opcode_table() {
    print_separator("DEMO 1: Opcode Table (PAL/O ISA)");

    SymbolVocabulary vocab;
    std::cout << "Total opcodes: " << vocab.size()
              << " (every one is exactly ONE Unicode codepoint, fixed arity, no precedence needed)\n\n";

    std::cout << std::left << std::setw(6) << "Glyph" << std::setw(12) << "Name"
              << std::setw(8) << "Arity" << "\n";
    std::cout << std::string(30, '-') << "\n";
    for (const auto& sym : vocab.all_symbols()) {
        std::cout << std::left << std::setw(6) << sym.glyph
                   << std::setw(12) << sym.name
                   << std::setw(8) << sym.arity << "\n";
    }
}

void demo_density() {
    print_separator("DEMO 2: Density -- dense glyph form vs. human-readable form");

    // add(x, y) = x + y
    // Dense: λλ(+ ₁ ₀)   i.e. λ λ + ₁ ₀   (curried; ₁=x outer, ₀=y inner)
    std::string dense_add = "λλ+₁₀";
    std::string human_add = "add x y \xE2\x89\x9C x + y"; // "add x y ≜ x + y"

    std::cout << "add(x, y) = x + y\n";
    std::cout << "  Human PAL  : \"" << human_add << "\" (" << human_add.size() << " bytes, "
              << utf8_codepoint_count(human_add) << " codepoints)\n";
    std::cout << "  Dense PAL/O: \"" << dense_add << "\" (" << dense_add.size() << " bytes, "
              << utf8_codepoint_count(dense_add) << " codepoints/opcodes)\n\n";

    // abs(x) = if x > 0 then x else -x
    // Dense: λ ⁇ (> ₀ #0) ₀ (∸ ₀)
    std::string dense_abs = "λ⁇>₀#0₀∸₀";
    std::string human_abs = "abs x \xE2\x89\x9C if x > 0 then x else -x";

    std::cout << "abs(x) = if x > 0 then x else -x\n";
    std::cout << "  Human PAL  : \"" << human_abs << "\" (" << human_abs.size() << " bytes, "
              << utf8_codepoint_count(human_abs) << " codepoints)\n";
    std::cout << "  Dense PAL/O: \"" << dense_abs << "\" (" << dense_abs.size() << " bytes, "
              << utf8_codepoint_count(dense_abs) << " codepoints/opcodes)\n";
    std::cout << "\nNote: codepoint count above upper-bounds the *token* count once these\n"
              << "glyphs are registered as single added tokens in the model's vocabulary --\n"
              << "every opcode collapses to exactly one token, with no identifiers, no\n"
              << "parens, and no keywords ever appearing in the stream.\n";
}

void demo_tokenizer() {
    print_separator("DEMO 3: Tokenizer (raw glyph stream -> tokens)");

    SymbolVocabulary vocab;
    Tokenizer tokenizer(vocab);

    std::string source = "λλ+₁₀"; // add(x,y) = x + y
    std::cout << "Source glyph stream: " << source << "\n\n";

    auto tokens = tokenizer.tokenize(source);
    std::cout << "Tokens:\n";
    for (const auto& tok : tokens) {
        if (tok.glyph == "<START>" || tok.glyph == "<END>") continue;
        std::cout << "  glyph='" << tok.glyph << "' id=" << tok.id;
        if (tok.op == Opcode::Var || tok.op == Opcode::GVar) {
            std::cout << " index=" << tok.index;
        }
        std::cout << "\n";
    }
}

void demo_parser() {
    print_separator("DEMO 4: Parser (fixed-arity prefix, no precedence tables)");

    SymbolVocabulary vocab;
    Tokenizer tokenizer(vocab);

    // Two definitions back-to-back with NO separator glyph — the parser
    // never needs one, because arity alone determines where each subtree ends.
    std::string source = "λλ+₁₀" "λ⁇>₀#0₀∸₀";
    std::cout << "Source (2 definitions, no separators): " << source << "\n\n";

    auto tokens = tokenizer.tokenize(source);

    try {
        Parser parser(tokens);
        Program program = parser.parse();

        std::cout << "Parsed " << program.definitions.size() << " definitions:\n";
        for (size_t i = 0; i < program.definitions.size(); ++i) {
            std::cout << "  def" << i << " = " << program.definitions[i]->to_string()
                       << "   (debug rendering only, not the wire format)\n";
        }
    } catch (const ParseError& e) {
        std::cout << "Parse error: " << e.what() << "\n";
    }
}

void demo_type_system() {
    print_separator("DEMO 5: Type System (shared Type hierarchy + unification)");

    auto int_t = std::make_shared<PrimitiveType>(PrimitiveType::INT);
    auto real_t = std::make_shared<PrimitiveType>(PrimitiveType::REAL);
    auto bool_t = std::make_shared<PrimitiveType>(PrimitiveType::BOOL);

    auto func_t = std::make_shared<FunctionType>(int_t, std::make_shared<FunctionType>(int_t, real_t));
    std::cout << "Function type (ℤ → ℤ → ℝ): " << func_t->to_string() << "\n";

    auto array_t = std::make_shared<ArrayType>(int_t);
    std::cout << "Array type [ℤ]: " << array_t->to_string() << "\n";

    auto tv1 = std::make_shared<TypeVariable>("α1");
    auto result = unify(*tv1, *int_t);
    if (result) {
        std::cout << "Unification α1 ~ ℤ succeeded: α1 -> " << result->at("α1")->to_string() << "\n";
    }
}

void demo_type_checker() {
    print_separator("DEMO 6: Type Checker (de Bruijn scope stack, opcode dispatch)");

    SymbolVocabulary vocab;
    Tokenizer tokenizer(vocab);

    std::string source = "λλ+₁₀"; // add(x,y) = x + y
    std::cout << "Source: " << source << "  (add(x,y) = x + y)\n\n";

    auto tokens = tokenizer.tokenize(source);

    try {
        Parser parser(tokens);
        Program program = parser.parse();

        TypeChecker checker;
        auto def_types = checker.check_program(program);

        std::cout << "Inferred " << def_types.size() << " top-level type(s) (pre-substitution):\n";
        for (size_t i = 0; i < def_types.size(); ++i) {
            std::cout << "  def" << i << " :: " << def_types[i]->to_string() << "\n";
        }

        std::cout << "\nGenerated " << checker.get_constraints().size() << " type constraints\n";
        auto solution = checker.solve_constraints();
        if (solution) {
            std::cout << "Constraint solving succeeded (" << solution->size() << " substitutions)\n";
        } else {
            std::cout << "Constraint solving failed\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}

int main() {
    try {
        std::cout << "\n";
        std::cout << "==============================================================\n";
        std::cout << "  PAL/O -- Dense Glyph Bootstrap (C++)\n";
        std::cout << "  Not human-readable by design: prefix, fixed-arity, nameless\n";
        std::cout << "==============================================================\n";

        demo_opcode_table();
        demo_density();
        demo_tokenizer();
        demo_parser();
        demo_type_system();
        demo_type_checker();

        print_separator("Complete!");
        std::cout << "\nAll demos completed successfully.\n";

    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
