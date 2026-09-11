# Tokenizer Architecture

## Overview

The PAL Tokenizer converts sequences of complex Unicode symbols into fixed-size token IDs suitable for LLM processing. Unlike traditional text tokenizers (e.g., BPE, WordPiece), the PAL tokenizer must handle:

- Multi-byte Unicode characters
- Symbol sequences with semantic meaning
- Variable-length constructs
- Context-aware tokenization rules

## Core Components

### 1. Symbol Vocabulary

**Purpose**: Maintain the canonical mapping of all valid symbols/constructs to token IDs.

**Structure**:
```
Symbol Vocabulary
├── Atomic Symbols (single Unicode characters)
│   ├── Mathematical operators: ∀, ∃, ∈, ∉, ∪, ∩, ...
│   ├── Logical operators: ¬, ∧, ∨, →, ↔, ...
│   ├── Type indicators: ⊤, ⊥, ⊢, ⊨, ...
│   └── Structural markers: ⟨, ⟩, ⟦, ⟧, ...
│
├── Multi-Symbol Constructs (sequences with special meaning)
│   ├── Operators: ∫∂, ∇·, ⊗⊕, ...
│   ├── Type signatures: Type[T→U], Generic[A,B], ...
│   └── Scoping markers: ⦅...⦆, ⟪...⟫, ...
│
└── Metadata
    ├── Token ID assignment
    ├── Priority/precedence
    ├── Context requirements
    └── Scope information
```

**Properties**:
- Total tokens: 4096 - 8192 (TBD based on symbol complexity)
- Token ID range: 0 - N
- Special tokens: [PAD], [START], [END], [UNKNOWN]

### 2. Tokenization Engine

**Input**: Stream of Unicode characters  
**Output**: Sequence of token IDs

**Algorithm**:
1. **Lexical Analysis** - Identify symbol boundaries
   - Greedy longest-match for multi-symbol constructs
   - Handle Unicode combining marks
   - Respect scope/context rules

2. **Token Lookup** - Map recognized symbols to IDs
   - O(1) lookup via symbol→ID hash table
   - Fallback to UNKNOWN token for unrecognized input

3. **Special Token Injection** - Insert markers
   - START token at sequence beginning
   - END token at sequence end
   - STOP tokens at logical boundaries

4. **Validation** - Check token sequence integrity
   - Ensure all scopes are balanced
   - Verify type-level consistency
   - Detect invalid symbol combinations

### 3. Context-Aware Rules

**Scoping**:
- Track open/close scope markers
- Enforce balanced nesting
- Apply scope-specific tokenization rules

**Type Context**:
- Maintain type stack during tokenization
- Validate symbol usage against current type context
- Insert type change tokens when context shifts

**Precedence**:
- Handle operator precedence in symbol sequences
- Insert precedence markers if needed
- Resolve ambiguous sequences using priority rules

## Token Categories

### Reserved Tokens (IDs 0-255)
- [PAD] - Padding token
- [START] - Sequence start
- [END] - Sequence end
- [UNKNOWN] - Unrecognized symbol
- [SCOPE_OPEN] - Scope begins
- [SCOPE_CLOSE] - Scope ends
- [TYPE_CHANGE] - Type context shift
- (others as needed)

### Symbol Tokens (IDs 256+)
- Mathematical/logical symbols
- Structural/organizational symbols
- Type system symbols
- Domain-specific symbols

## Example Tokenization

**PAL Input**:
```
∀x∈ℤ: x⊕¬x ≡ ⊥
```

**Tokens** (simplified):
```
[START] [FORALL] [VARIABLE] [ELEM] [INTEGER_TYPE] [COLON] 
[VARIABLE] [XOR] [NOT] [VARIABLE] [EQUIV] [FALSE] [END]
```

**Token IDs** (example):
```
1 256 300 257 258 259 301 300 260 301 262 263 2
```

## Challenges & Design Decisions

1. **Symbol Boundary Detection**
   - Problem: Not all Unicode symbols are single-byte
   - Decision: Use Unicode normalization (NFC) + multi-byte awareness

2. **Semantic vs. Syntactic Tokenization**
   - Problem: Same symbol may mean different things in different contexts
   - Decision: Implement context stacks, inject context-change tokens

3. **Vocabulary Size**
   - Problem: Too many symbols → large model; too few → poor expressivity
   - Decision: Start with ~4096 core symbols, expand based on training data

4. **Greedy vs. Optimal Tokenization**
   - Problem: Greedy longest-match may not minimize token count globally
   - Decision: Use greedy for inference speed; optimize offline if needed

## Implementation Considerations

- Language: Python (with optional Rust for performance)
- Dependencies: `regex`, `unicodedata`, custom symbol database
- Performance target: Tokenize 1M tokens/second on single thread
- Memory footprint: <100MB for vocabulary + state

## Integration with Model

The tokenizer output (token IDs) feeds directly into the 7B LLM:
- Input embeddings map token IDs to 4096-dim vectors
- Model processes token sequence through attention layers
- Output logits predict next token ID
- Greedy/beam search decodes token ID stream back to PAL

## Future Extensions

1. **Subword Tokenization**: Handle novel symbol combinations
2. **Adaptive Vocabulary**: Learn important symbol sequences during training
3. **Cross-Lingual Symbols**: Support symbols from different notational traditions
4. **Symbol Compression**: Merge frequent multi-symbol patterns into new tokens
