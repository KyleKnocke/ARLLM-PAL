#include <catch2/catch_test_macros.hpp>
#include "../src/tokenizer/vocabulary.h"

TEST_CASE("Vocabulary: Symbol uniqueness and arity correctness", "[vocabulary]") {
    arllm::SymbolVocabulary vocab;

    auto& symbols = vocab.all_symbols();

    // Check that we have exactly one symbol per Opcode
    std::map<arllm::Opcode, int> opcode_count;
    for (const auto& sym : symbols) {
        opcode_count[sym.op]++;
    }

    SECTION("Each opcode appears exactly once") {
        REQUIRE(opcode_count[arllm::Opcode::True] == 1);
        REQUIRE(opcode_count[arllm::Opcode::False] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Nil] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Var] == 1);
        REQUIRE(opcode_count[arllm::Opcode::GVar] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Num] == 1);

        REQUIRE(opcode_count[arllm::Opcode::Neg] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Not] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Fix] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Ret] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Box] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Deref] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Lambda] == 1);

        REQUIRE(opcode_count[arllm::Opcode::Add] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Sub] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Mul] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Div] == 1);
        REQUIRE(opcode_count[arllm::Opcode::And] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Or] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Eq] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Neq] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Lt] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Le] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Gt] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Ge] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Mem] == 1);
        REQUIRE(opcode_count[arllm::Opcode::NMem] == 1);
        REQUIRE(opcode_count[arllm::Opcode::SubsetEq] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Subset] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Union] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Intersect] == 1);
        REQUIRE(opcode_count[arllm::Opcode::SetMinus] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Apply] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Compose] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Let] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Forall] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Exists] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Idx] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Cons] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TAnnot] == 1);

        REQUIRE(opcode_count[arllm::Opcode::Cond] == 1);
        REQUIRE(opcode_count[arllm::Opcode::Iter] == 1);

        // Type opcodes
        REQUIRE(opcode_count[arllm::Opcode::TTop] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TBot] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TBool] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TInt] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TReal] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TStr] == 1);

        REQUIRE(opcode_count[arllm::Opcode::TArr] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TPtr] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TBox] == 1);

        REQUIRE(opcode_count[arllm::Opcode::TFun] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TProd] == 1);
        REQUIRE(opcode_count[arllm::Opcode::TUnion] == 1);
    }

    SECTION("All token IDs are unique and >=4") {
        std::set<int> seen_ids;
        for (const auto& sym : symbols) {
            REQUIRE(sym.token_id >= 4); // Reserved tokens: PAD=0, START=1, END=2, UNKNOWN=3
            REQUIRE(seen_ids.find(sym.token_id) == seen_ids.end());
            seen_ids.insert(sym.token_id);
        }
    }

    SECTION("All glyphs are unique") {
        std::set<std::string> seen_glyphs;
        for (const auto& sym : symbols) {
            REQUIRE(sym.glyph.size() > 0); // Must have at least one UTF-8 byte
            REQUIRE(seen_glyphs.find(sym.glyph) == seen_glyphs.end());
            seen_glyphs.insert(sym.glyph);
        }
    }

    SECTION("Arity matches opcode definitions") {
        for (const auto& sym : symbols) {
            int expected_arity = arllm::SymbolVocabulary::arity_of(sym.op);
            REQUIRE(sym.arity == expected_arity);

            // Validate arity categories manually
            switch (sym.op) {
                case arllm::Opcode::True: case arllm::Opcode::False:
                case arllm::Opcode::Nil: case arllm::Opcode::Var:
                case arllm::Opcode::GVar: case arllm::Opcode::Num:
                case arllm::Opcode::TTop: case arllm::Opcode::TBot:
                case arllm::Opcode::TBool: case arllm::Opcode::TInt:
                case arllm::Opcode::TReal: case arllm::Opcode::TStr:
                    REQUIRE(sym.arity == 0);
                    break;

                case arllm::Opcode::Neg: case arllm::Opcode::Not:
                case arllm::Opcode::Fix: case arllm::Opcode::Ret:
                case arllm::Opcode::Box: case arllm::Opcode::Deref:
                case arllm::Opcode::Lambda: case arllm::Opcode::TArr:
                case arllm::Opcode::TPtr: case arllm::Opcode::TBox:
                    REQUIRE(sym.arity == 1);
                    break;

                case arllm::Opcode::Add: case arllm::Opcode::Sub:
                case arllm::Opcode::Mul: case arllm::Opcode::Div:
                case arllm::Opcode::And: case arllm::Opcode::Or:
                case arllm::Opcode::Eq: case arllm::Opcode::Neq:
                case arllm::Opcode::Lt: case arllm::Opcode::Le:
                case arllm::Opcode::Gt: case arllm::Opcode::Ge:
                case arllm::Opcode::Mem: case arllm::Opcode::NMem:
                case arllm::Opcode::SubsetEq: case arllm::Opcode::Subset:
                case arllm::Opcode::Union: case arllm::Opcode::Intersect:
                case arllm::Opcode::SetMinus: case arllm::Opcode::Apply:
                case arllm::Opcode::Compose: case arllm::Opcode::Let:
                case arllm::Opcode::Forall: case arllm::Opcode::Exists:
                case arllm::Opcode::Idx: case arllm::Opcode::Cons:
                case arllm::Opcode::TAnnot: case arllm::Opcode::TFun:
                case arllm::Opcode::TProd: case arllm::Opcode::TUnion:
                    REQUIRE(sym.arity == 2);
                    break;

                case arllm::Opcode::Cond: case arllm::Opcode::Iter:
                    REQUIRE(sym.arity == 3);
                    break;

                default:
                    FAIL("Unhandled opcode in arity test");
            }
        }
    }
}