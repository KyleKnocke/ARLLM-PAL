# PAL/Ø C++ Compiler Bootstrap

## Overview

Complete **PAL/Ø** compiler bootstrap in C++ (C++20). PAL/Ø is a dense,
fixed-arity, prefix (Polish-notation) glyph language with de Bruijn
variable indices — designed for an LLM to *emit*, not for a human to
read or write. See [GLYPH_SPEC.md](./GLYPH_SPEC.md) for the full opcode
reference.

Compiles: **English/Mermaid description → PAL/Ø glyph stream → AST → Type-Checked AST → GIMPLE IR → C Code → Binary**

## Architecture

### Components

1. **Type System** (`src/common/types.{h,cpp}`) — unchanged reusable infrastructure
   - 7 primitive types: `⊤⊥𝔹ℤℝ𝕾`
   - 6 composite types: Function, Product, Union, Array, Boxed, Pointer
   - Robinson unification algorithm
   - Scope-aware `TypeEnvironment` (retained for potential reuse, no longer used by the name-free type checker)

2. **Symbol Vocabulary** (`src/tokenizer/vocabulary.{h,cpp}`)
   - The complete PAL/Ø opcode table: one glyph = one opcode = one Unicode codepoint
   - Static, per-opcode fixed arity (`SymbolVocabulary::arity_of`) — no precedence/associativity needed
   - De Bruijn index glyphs (`₀`-`₉` + escape `ᵥ`) for bound variables, positional glyphs (`⁰`-`⁹` + escape `ᴳ`) for top-level definitions

3. **Tokenizer** (`src/tokenizer/tokenizer.{h,cpp}`)
   - Raw UTF-8 glyph stream → token stream (one codepoint per token, plus two escape forms that consume an inline ASCII digit run)
   - No identifier/keyword/comment scanning needed — that vocabulary doesn't exist in the wire format
   - Source location tracking for error messages

4. **AST** (`src/parser/ast_nodes.{h,cpp}`)
   - A single unified `ExprNode` class for both expression and type subtrees
   - `Program` = a flat, unnamed sequence of top-level definitions (positional, no `Definition` wrapper)
   - `node_to_type()` bridges type-opcode subtrees into the `Type` hierarchy
   - Debug-only `to_string()` pretty-printer (never fed back into the tokenizer/parser)

5. **Parser** (`src/parser/parser.{h,cpp}`)
   - One recursive function, `parse_node()`: read a glyph, then recursively parse exactly `arity(op)` children
   - No precedence climbing, no parenthesis matching — arity alone determines subtree boundaries
   - Error handling with source locations (`ParseError`)

6. **Type Checker** (`src/type_system/type_checker.{h,cpp}`)
   - Hindley-Milner inference over `ExprNode`, dispatching on `Opcode` (no RTTI/dynamic_cast on expression nodes)
   - De Bruijn-indexed local scope stack (`std::vector<TypePtr>`) instead of a name-based environment
   - `global_types` accumulates each top-level definition's inferred type in declaration order for `GVar` lookups (forward references are not supported — use `Fix` for recursion)
   - Operator types are hard-coded directly per-opcode in the inference switch (no name-based "prelude" indirection)

7. **GIMPLE Builder** (`src/gimple/builder.{h,cpp}`)
   - Converts typed AST to GCC GIMPLE three-address code (code generation currently a stub)
   - Function and basic block representations
   - Temporary variable management

## Building

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16+

### Build Steps

```bash
cd ARLLM-PAL
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..  # or -G "Unix Makefiles" on Linux/macOS
cmake --build . --config Release
```

### Running Demo

```bash
./examples/demo
# or on Windows:
# examples\Release\demo.exe
```

## Example Programs

All source is dense glyph streams; see [GLYPH_SPEC.md](./GLYPH_SPEC.md) for
the full opcode table. A "human PAL" rendering is shown only for
explanatory purposes — it is never part of the actual language.

### Simple function: `add(x, y) = x + y`

```
λλ+₁₀
```

### Conditional: `abs(x) = if x > 0 then x else -x`

```
λ⁇>₀#0₀∸₀
```

### Composition: `compose(f, g, x) = f(g(x))`

```
λλλ@₂@₁₀
```

(three nested `λ` bind `f`, `g`, `x` from outermost to innermost; body is
`@ f (@ g x)`, i.e. `Apply(Var(2)=f, Apply(Var(1)=g, Var(0)=x))`.)

## Next Steps

- Implement `GimpleGenerator::generate()` body (currently a stub): lower typed `ExprNode` trees to GIMPLE three-address IR
- GCC backend integration (GIMPLE → C → binary)
- Mermaid diagram integration with EN-MMD model
- Register PAL/Ø glyphs as dedicated tokens and fine-tune a model on source-code → PAL/Ø transcription
- Training data generation for fine-tuned models
- Full end-to-end pipeline validation

## Related Repositories

- **EN-MMD-Qwen-2.50-7b**: English natural language → Mermaid diagrams
- **MMD-PAL-Qwen-2.50-7b**: Mermaid ↔ PAL bidirectional translation

Both fine-tuned Qwen 2.5 Coder 7B models using QLoRA for inference/training on consumer hardware (16GB GPU).

## License

MIT

