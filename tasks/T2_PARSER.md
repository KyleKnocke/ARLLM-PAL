# T2: Implement Parser (Not Started)

> ❌ Status: Not Started

## Description
Implement the recursive-descent parser that converts a token stream into an AST (`ExprNode`) using **fixed-arity opcode semantics** from GLYPH_SPEC.md.

Unlike traditional parsers, PAL/Ø has no keywords, identifiers, or precedence — every glyph has a fixed arity (0, 1, 2, or 3), and the parser simply:
> "Read one token → recursively parse exactly `arity(token)` child subtrees."

This makes parsing deterministic and trivial to implement once the vocabulary is correctly mapped.

## Implementation Requirements

### Input
Token stream from T1: `[LAMBDA, LAMBDA, ADD, VAR1, VAR0]`

### Output
AST node:
```python
Application(
  Application(
    Lambda("x", Lambda("y", BinaryOp("+", Variable("y"), Variable("x")))),
    Literal(1)
  ),
  Literal(0)
)
```
Wait — correction: in PAL/Ø, `λλ+₁₀` means:
- First `λ`: binds variable at depth 0 (innermost)
- Second `λ`: binds variable at depth 0 of outer scope
- `+`: binary operator → takes two children
- `₁`: de Bruijn index 1 → refers to outer binder (x)
- `₀`: de Bruijn index 0 → refers to inner binder (y)
→ So AST should be: `Lambda(x, Lambda(y, Add(Var1, Var0)))`

### Key Logic
- Use arity mapping from vocabulary:
  - Arity 0: TRUE, FALSE, NIL, NUM, VAR{0-9}, GVAR{0-9}
  - Arity 1: NEG, NOT, FIX, RET, BOX, DEREF, LAMBDA
  - Arity 2: ADD, SUB, MUL, DIV, AND, OR, EQ, NEQ, LT, LE, GT, GE, MEM, NMEM, SUBSETEQ, SUBSET, UNION, INTERSECT, SETMINUS, APPLY, COMPOSE, LET, FORALL, EXISTS, IDX, CONS, TANNOT
  - Arity 3: COND, ITER
- Parse recursively:
  ```python
  def parse_node():
      tok = next_token()
      if tok.arity == 0:
          return make_atom(tok)
      elif tok.arity == 1:
          child = parse_node()
          return UnaryOp(tok.opcode, child)
      elif tok.arity == 2:
          left = parse_node()
          right = parse_node()
          return BinaryOp(tok.opcode, left, right)
      elif tok.arity == 3:
          a = parse_node()
          b = parse_node()
          c = parse_node()
          return TernaryOp(tok.opcode, a, b, c)
  ```
- For `LAMBDA` (arity=1): the child is the body. The variable index is embedded in the token ID — no name needed.
- For `VARx` and `GVARx`: create Variable or GlobalVariable node with de Bruijn index = glyph value.
- For `TANNOT` (`⦂`): AST node has two children: expression, type-expression. Type-expression will be converted to Type by `node_to_type()` later.

## Dependencies
- ✅ T0: SymbolVocabulary (for arity mapping)
- ✅ T1: Tokenizer (provides token stream)
- ✅ AST Nodes (to construct ExprNode tree)
- 🚧 Type System (for TANNOT → Type conversion — but can be stubbed for now)

## Acceptance Criteria

1. `Parser.parse([LAMBDA, LAMBDA, ADD, VAR1, VAR0])` returns:
   ```python
   Lambda(
       name="x", 
       body=Lambda(
           name="y",
           body=BinaryOp("+", Variable("1"), Variable("0"))
       )
   )
   ```
   → BUT: In PAL/Ø, names are NOT used — VAR1 is index 1 (outer lambda’s binder). So:
   ```python
   Lambda(  # outer λ
     body=Lambda(  # inner λ
        body=BinaryOp("+", 
          Variable(de_bruijn_index=1),  # refers to outer λ
          Variable(de_bruijn_index=0)   # refers to inner λ
        )
     )
   )
   ```
2. `Parser.parse([LAMBDA, COND, GT, VAR0, NUM, 0, VAR0, NEG, VAR0])` parses "λ⁇>₀#0₀∸₀" correctly.
3. Parser must handle any valid glyph sequence — no assumptions about order or structure beyond arity.
4. Parse errors (e.g., too few children) → raise `SyntaxError` with line/column.
5. AST nodes include source location from tokens.

## Implementation Plan
1. Replace current `Parser._parse_expression()` with arity-driven recursive descent
2. Remove all identifier/keyword logic — only use token IDs
3. Create helper: `get_arity(token_id)` → lookup in vocabulary
4. Implement `parse_node()` as above
5. Map token IDs to AST node types using opcode enum (from vocabulary)
6. Write test cases for GLYPH_SPEC.md examples:
   - "λλ+₁₀"
   - "λ⁇>₀#0₀∸₀"
   - "⦂λλ×₂₁ ℤ" → TANNOT with type annotation
7. Integrate into `examples/demo.py`

## Blocked By
- ✅ Tokenizer (T1) — must produce correct tokens

## Next Steps After Completion
→ Enable Type Checker (T3) to traverse AST and infer types using de Bruijn scope.
