# T1: Implement Tokenizer (Not Started)

> ❌ Status: Not Started

## Description
Implement the tokenizer to convert a raw UTF-8 glyph stream into a sequence of token IDs using the vocabulary defined in T0.

PAL/Ø’s wire format is a continuous Unicode stream with no whitespace or delimiters. The tokenizer must:
- Scan input byte-by-byte and match against known glyphs (e.g., `λ`, `∈`, `→`)
- Handle multi-byte Unicode codepoints correctly
- Recognize inline numeric literals (`#42` → NUM token with value 42)
- Map de Bruijn indices: `₀`–`₉` → Var0–Var9, `ᵥ` followed by digits → Var(n≥10), etc.
- Output sequence of tokens: `[LAMBDA, LAMBDA, ADD, VAR1, VAR0]`

## Implementation Requirements

### Input
Raw UTF-8 string: `"λλ+₁₀"`

### Output
List of `Token` objects:
```python
[
  Token(id=28, value="λ", line=1, column=1),
  Token(id=28, value="λ", line=1, column=2),
  Token(id=15, value="+", line=1, column=3),
  Token(id=10, value="₁", line=1, column=4), # VAR1
  Token(id=9,  value="₀", line=1, column=5)  # VAR0
]
```

### Key Logic
- Use `SymbolVocabulary.get_symbol()` to match glyphs (supports multi-char for future extensibility)
- Handle numeric literals: `#` followed by ASCII digits → `NUM` with value parsed as int/float
- Support de Bruijn index encoding:
  - Single glyph `₀`–`₉` → Var0–Var9 (arity=0)
  - `ᵥ` + digit string → Var(n≥10) (e.g., `ᵥ15` → Var15)
  - `⁰`–`⁹`, `ᴳ` + digits → GVar equivalents
- Skip comments (`#`) and whitespace
- Emit `START` and `END` tokens at boundaries

## Dependencies
- ✅ T0: SymbolVocabulary (must be imported and used)
- ✅ AST Nodes (for understanding token semantics — but not directly required)

## Acceptance Criteria

1. `Tokenizer.tokenize("λλ+₁₀")` returns exactly 5 tokens with correct IDs from `vocabulary.py`
2. `Tokenizer.tokenize("λ⁇>₀#0₀∸₀")` correctly parses all glyphs including numeric literal `#0`
3. Tokenizer handles edge cases:
   - Empty input → `[START, END]`
   - Unknown glyph → `UNKNOWN` token
   - Malformed de Bruijn (`ᵥabc`) → treat as unknown or raise error
4. Source location (line/column) tracked accurately
5. No false positives: ASCII `-` ≠ Unicode `−` (U+2212)

## Implementation Plan
1. Fork current `tokenizer.py` — it’s skeletal and assumes identifiers/keywords
2. Rewrite `_read_identifier()`, `_read_number()` to recognize glyphs via vocabulary
3. Implement glyph matching loop using `vocabulary.get_symbol()`
4. Add logic for de Bruijn index decoding (`ᵥ`, `ᴳ`)
5. Write unit tests in `tests/unit/tokenizer/`
6. Integrate with demo: `examples/demo.py` should now print token stream

## Testing Script (to verify)
```python
from src.tokenizer.tokenizer import Tokenizer

t = Tokenizer()
tokens = t.tokenize("λλ+₁₀")
assert len(tokens) == 5, f"Expected 5 tokens, got {len(tokens)}"
assert tokens[0].value == "λ" and tokens[0].id == 28  # LAMBDA
assert tokens[3].value == "₁" and tokens[3].id == 10   # VAR1
assert tokens[4].value == "₀" and tokens[4].id == 9    # VAR0
print("Tokenizer passed basic test.")
```

## Blocked By
- None — T0 is complete.

## Next Steps After Completion
→ Enable Parser (T2) to consume token stream instead of character stream.