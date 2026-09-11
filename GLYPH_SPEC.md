# PAL/Ø Glyph Specification

PAL/Ø is **not a human-readable language**. It is a fixed-arity, prefix
(Polish-notation) opcode stream designed for maximum token density when
emitted by an LLM. There are no keywords, no identifiers, no parentheses,
no commas, and no comments in the wire format. Every opcode is exactly
**one Unicode codepoint**; every opcode has a **statically fixed arity**
(0, 1, 2, or 3), so a single recursive-descent rule — "read one glyph,
then recursively read exactly `arity(glyph)` more subtrees" — parses the
entire grammar, both expressions and types.

This table is the single source of truth and must always match
[`src/tokenizer/vocabulary.cpp`](src/tokenizer/vocabulary.cpp)'s
`init_symbols()` exactly. If you change one, change the other.

## Design principles

1. **Prefix notation, fixed arity.** No precedence tables, no associativity
   rules, no bracket matching — the parser always knows exactly how many
   child subtrees follow a given opcode.
2. **De Bruijn indices, not names.** Bound variables (`Var`) reference
   their binder by depth (0 = innermost). Top-level definitions are
   referenced positionally by declaration order (`GVar`). There are no
   identifier tokens anywhere in the language.
3. **Unified grammar.** "Type" opcodes and "expression" opcodes are
   disjoint subsets of one `Opcode` enum, parsed by the same function and
   represented by the same tree node (`ExprNode`). Type subtrees are only
   interpreted as such via `TAnnot`'s second child, converted to the
   `Type` class hierarchy by `node_to_type()`.
4. **No separators between top-level definitions.** Since every opcode's
   subtree length is fully determined by its arity, the parser always
   knows where one definition ends and the next begins.

## Leaves (arity 0)

| Glyph | Name | Opcode | Meaning |
|---|---|---|---|
| `𝟙` | TRUE | `True` | boolean literal `true` |
| `𝟘` | FALSE | `False` | boolean literal `false` |
| `∅` | NIL | `Nil` | empty array/list literal |
| `₀`–`₉` | VAR0–VAR9 | `Var` | bound variable, de Bruijn index baked into glyph (0–9) |
| `ᵥ` | VAR_ESC | `Var` | bound variable; index ≥ 10, read as inline ASCII digit run |
| `⁰`–`⁹` | GVAR0–GVAR9 | `GVar` | top-level definition reference, positional index (0–9) |
| `ᴳ` | GVAR_ESC | `GVar` | top-level reference; index ≥ 10, read as inline ASCII digit run |
| `#` | NUM | `Num` | numeric literal; followed by inline ASCII digits (+ optional `.`) |

## Unary (arity 1)

| Glyph | Name | Opcode | Meaning |
|---|---|---|---|
| `∸` | NEG | `Neg` | arithmetic negation |
| `¬` | NOT | `Not` | boolean negation |
| `↻` | FIX | `Fix` | fixed-point combinator (recursion): `fix f :: (A→A)→A` |
| `⏎` | RET | `Ret` | return/yield the child's value |
| `▢` | BOX | `Box` | wrap value in a heap box |
| `⌾` | DEREF | `Deref` | dereference a pointer or unbox a boxed value |
| `λ` | LAMBDA | `Lambda` | function abstraction; body is the single child, with a fresh de Bruijn binder at index 0 |

## Binary (arity 2)

### Arithmetic
| Glyph | Name | Opcode |
|---|---|---|
| `+` | ADD | `Add` |
| `−` | SUB | `Sub` (U+2212 minus sign, **not** ASCII hyphen) |
| `×` | MUL | `Mul` |
| `÷` | DIV | `Div` |

### Logical
| Glyph | Name | Opcode |
|---|---|---|
| `∧` | AND | `And` |
| `∨` | OR | `Or` |

### Comparison
| Glyph | Name | Opcode |
|---|---|---|
| `=` | EQ | `Eq` |
| `≠` | NEQ | `Neq` |
| `<` | LT | `Lt` |
| `≤` | LE | `Le` |
| `>` | GT | `Gt` |
| `≥` | GE | `Ge` |

### Set
| Glyph | Name | Opcode |
|---|---|---|
| `∈` | MEM | `Mem` |
| `∉` | NMEM | `NMem` |
| `⊆` | SUBSETEQ | `SubsetEq` |
| `⊂` | SUBSET | `Subset` |
| `∪` | UNION | `Union` |
| `∩` | INTERSECT | `Intersect` |
| `∖` | SETMINUS | `SetMinus` |

### Control / binding / structural
| Glyph | Name | Opcode | Meaning |
|---|---|---|---|
| `@` | APPLY | `Apply` | function application: `@ f x` |
| `∘` | COMPOSE | `Compose` | function composition: `∘ f g` = `f ∘ g` |
| `≜` | LET | `Let` | `≜ value body` — binds `value` at de Bruijn index 0 inside `body` |
| `∀` | FORALL | `Forall` | universal quantifier over an array's elements |
| `∃` | EXISTS | `Exists` | existential quantifier over an array's elements |
| `‣` | IDX | `Idx` | array indexing: `‣ arr i` |
| `∷` | CONS | `Cons` | prepend an element to an array |
| `⦂` | TANNOT | `TAnnot` | type annotation: `⦂ expr type` |

## Ternary (arity 3)

| Glyph | Name | Opcode | Meaning |
|---|---|---|---|
| `⁇` | COND | `Cond` | `⁇ cond then else` |
| `⟳` | ITER | `Iter` | fold/iterate: `⟳ iterable init body`; inside `body`, index 0 = accumulator, index 1 = current element |

## Type opcodes

Type opcodes are a disjoint subset of the same `Opcode` enum, reached in
practice through `TAnnot`'s second child and converted to the reusable
`Type` hierarchy (`src/common/types.h`) via `node_to_type()`.

### Primitive (arity 0)
| Glyph | Name | Opcode | Corresponds to |
|---|---|---|---|
| `⊤` | T_TOP | `TTop` | `PrimitiveType::TOP` |
| `⊥` | T_BOT | `TBot` | `PrimitiveType::BOTTOM` |
| `𝔹` | T_BOOL | `TBool` | `PrimitiveType::BOOL` |
| `ℤ` | T_INT | `TInt` | `PrimitiveType::INT` |
| `ℝ` | T_REAL | `TReal` | `PrimitiveType::REAL` |
| `𝕾` | T_STR | `TStr` | `PrimitiveType::STRING` |

### Unary (arity 1)
| Glyph | Name | Opcode | Corresponds to |
|---|---|---|---|
| `⟨` | T_ARR | `TArr` | `ArrayType(element)` |
| `⟡` | T_PTR | `TPtr` | `PointerType(pointee)` |
| `⧈` | T_BOX | `TBox` | `BoxedType(inner)` |

### Binary (arity 2)
| Glyph | Name | Opcode | Corresponds to |
|---|---|---|---|
| `→` | T_FUN | `TFun` | `FunctionType(param, return)` |
| `⨯` | T_PROD | `TProd` | `ProductType({left, right})` |
| `⋃` | T_UNION | `TUnion` | `UnionType({left, right})` |

## Reserved (non-opcode) tokens

These frame the token stream but are never emitted as glyphs in the
program text itself; they are synthesized by the tokenizer.

| Token | Value | Meaning |
|---|---|---|
| `PAD` | 0 | padding token |
| `START` | 1 | synthetic start-of-stream marker |
| `END` | 2 | synthetic end-of-stream marker |
| `UNKNOWN` | 3 | unrecognized glyph |

## Worked examples

**`add(x, y) = x + y`** (curried lambda):

```
λλ+₁₀
```

5 glyphs: `λ` (bind `x`), `λ` (bind `y`), `+`, `₁` (outer binder `x`, depth 1
from the innermost `y`), `₀` (innermost binder `y`, depth 0).

**`abs(x) = if x > 0 then x else -x`**:

```
λ⁇>₀#0₀∸₀
```

9 codepoints: `λ` (bind `x`), `⁇` (cond), `>` `₀` `#0` (is `x > 0`), `₀`
(then-branch: `x`), `∸` `₀` (else-branch: `-x`).

## Known v1 limitations

- `GVar` may only reference an **earlier** top-level definition
  (declaration order = index order). Self- or mutual recursion must go
  through the `Fix` opcode instead of a forward `GVar` reference.
- There is no source-level comment syntax; none is planned, since the
  format is not intended to be hand-authored or human-read in practice.
