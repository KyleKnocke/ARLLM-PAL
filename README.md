# PAL/Ø — a Dense Glyph Language for LLMs, Compiled to GIMPLE

## Overview

PAL/Ø is a **deterministic C++ compiler bootstrap** for a glyph-based
instruction language designed from the ground up to be maximally dense and
token-efficient for a large language model to *emit* — it is **not intended
to be human-readable**. There are no identifiers, no keywords, no
parentheses/commas, no comments, and no operator precedence in the wire
format: every opcode is exactly one Unicode codepoint with a statically
fixed arity (0, 1, 2, or 3), and every bound variable is referenced by
de Bruijn index rather than by name. See [GLYPH_SPEC.md](./GLYPH_SPEC.md)
for the complete, canonical opcode reference.

**Bootstrap Pipeline:** English/Mermaid description → PAL/Ø glyph stream → Tokenizer → AST (`ExprNode`) → Hindley-Milner Type Checker → GIMPLE IR → C Code → Binary

### Why prefix + fixed-arity + de Bruijn?

- **Prefix, fixed arity** eliminates parentheses, commas, precedence
  tables, and associativity rules entirely — the parser always knows
  exactly how many subtrees follow any given opcode, so correctness of a
  partially generated stream is a purely local, per-token property. That
  is precisely the kind of pattern transformer attention learns fastest,
  compared to infix notation with matched parens and long-range
  precedence agreement.
- **De Bruijn indices instead of names** remove identifier tokens
  entirely — the model never has to invent or remember a spelling; it
  only has to count binder depth, a much more local and learnable
  structural skill than name tracking.
- **One glyph = one opcode = one Unicode codepoint**, intended to become
  exactly one token once these glyphs are registered as dedicated added
  tokens in an LLM's tokenizer vocabulary (with a corresponding embedding
  resize) during a future fine-tuning phase.

### Core Components

1. **Type System** (`src/common/types.h/.cpp`) — 7 primitives + 6 composites with Robinson unification (unchanged infrastructure, reused as-is)
2. **Symbol Vocabulary** (`src/tokenizer/vocabulary.h/.cpp`) — the full PAL/Ø opcode table: glyph ↔ opcode ↔ arity
3. **Tokenizer** (`src/tokenizer/tokenizer.h/.cpp`) — raw UTF-8 glyph stream → token stream (no whitespace/keyword handling needed in the true wire format)
4. **AST** (`src/parser/ast_nodes.h/.cpp`) — a single unified `ExprNode` class (expression and type opcodes share one tree structure); `node_to_type()` bridges type subtrees to the `Type` hierarchy
5. **Parser** (`src/parser/parser.h/.cpp`) — one recursive-descent function (`parse_node()`) for the entire grammar, driven purely by each opcode's fixed arity
6. **Type Checker** (`src/type_system/type_checker.h/.cpp`) — Hindley-Milner inference over `ExprNode`, using a de Bruijn-indexed scope stack instead of a name-based environment
7. **GIMPLE Builder** (`src/gimple/builder.h/.cpp`) — Typed AST → GCC GIMPLE IR (code generation still a stub)

## Architecture

See the following documents for detailed architecture:

- [Glyph Specification (canonical opcode table)](./GLYPH_SPEC.md)
- [C++ Build Instructions](./README_CPP.md)
- [Tokenizer Architecture](./docs/tokenizer-architecture.md)
- [PAL Language Specification](./docs/pal-language-spec.md)
- [Compiler Architecture](./docs/compiler-architecture.md)
- [PAL → GIMPLE Transpiler](./docs/gimple-transpiler.md)
- [System Integration](./docs/system-integration.md)

## Hardware Target

- GPU: 16GB AMD
- RAM: 64GB
- Model Size: 7B parameters (optimized for this hardware configuration)

## Project Structure

```
ARLLM-PAL/
├── CMakeLists.txt
├── GLYPH_SPEC.md                        (canonical PAL/Ø opcode reference)
├── src/
│   ├── common/
│   │   ├── types.h / types.cpp          (Type system + unification)
│   ├── tokenizer/
│   │   ├── vocabulary.h / .cpp          (PAL/Ø opcode table: glyph <-> opcode <-> arity)
│   │   └── tokenizer.h / .cpp           (raw UTF-8 glyph stream -> tokens)
│   ├── parser/
│   │   ├── ast_nodes.h / .cpp           (unified ExprNode + node_to_type)
│   │   └── parser.h / .cpp              (single arity-driven parse_node())
│   ├── type_system/
│   │   └── type_checker.h / .cpp        (Hindley-Milner inference, de Bruijn scope stack)
│   └── gimple/
│       └── builder.h / .cpp             (AST → GIMPLE IR)
├── examples/
│   └── demo.cpp                         (Complete pipeline demo over dense glyph strings)
├── docs/                                (Original documentation)
├── .gitignore
└── README_CPP.md                        (C++ build instructions)
```

## Compilation Pipeline

```
Input (code, spec, description)
  ↓
7B Model (ARLLM-PAL/Ø Transcriber)
  ↓
PAL/Ø (dense glyph stream, e.g. "λλ+₁₀")
  ↓
Tokenizer (raw UTF-8 codepoints → tokens)
  ↓
Parser → ExprNode AST (single arity-driven rule)
  ↓
Type Checker & Semantic Analyzer (de Bruijn scope stack)
  ↓
GIMPLE Code Generator
  ↓
GIMPLE IR (three-address code)
  ↓
GCC Compilation Pipeline
  ├─ Optimization (inlining, vectorization, etc.)
  ├─ Code generation (register allocation, scheduling)
  └─ Backend (x86, ARM, MIPS, RISC-V, etc.)
  ↓
Optimized Machine Code
```

## Key Concepts

### Glyphs vs. traditional source text

PAL/Ø is not meant to be written or read by humans. Every opcode is one
Unicode codepoint with a fixed arity; there is no operator precedence, no
punctuation, and no named identifiers anywhere in the stream — see
[GLYPH_SPEC.md](./GLYPH_SPEC.md) for the complete opcode-by-opcode
reference and worked examples (e.g. `add(x,y)=x+y` compiles to the
5-codepoint stream `λλ+₁₀`).

### Token mapping

Each glyph maps to exactly one token id in `SymbolVocabulary`. Once these
glyphs are added as dedicated tokens to a target LLM's tokenizer (with a
corresponding embedding-matrix resize), every opcode becomes exactly one
model token — this is the density property the whole design optimizes for.

### Compilation to GIMPLE

PAL/Ø provides an extremely dense abstract syntax, while GIMPLE (GCC
Intermediate Middle-End Language) provides the gateway to world-class
binary optimization:

```
PAL/Ø (dense, glyph-based) → GIMPLE (three-address, type-explicit) → GCC (60+ years of optimization)
```

This strategy ensures we generate binaries with performance comparable to hand-written C with `-O3` optimization.

## Next Steps

1. **GIMPLE Code Generation** — implement the `GimpleGenerator::generate()` body (currently a stub) to lower typed `ExprNode` trees to GIMPLE three-address IR
2. **Integration Testing** — verify tokenizer/parser/type-checker pipeline against a larger glyph corpus
3. **Benchmark Suite** — test on algorithms (sorting, graph algorithms, numerical compute) expressed in PAL/Ø
4. **Model Training** — register PAL/Ø glyphs as dedicated tokens and fine-tune a 7B model on source-code → PAL/Ø transcription
5. **End-to-End System** — integration with the GCC backend
6. **Optimization & Profiling** — fine-tune GIMPLE generation for performance

