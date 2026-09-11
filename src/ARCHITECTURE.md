# PAL/Ø Implementation Architecture

## Project Structure

```
ARLLM-PAL/
├── docs/                          # Architecture & design docs
│   ├── tokenizer-architecture.md
│   ├── pal-language-spec.md
│   ├── compiler-architecture.md
│   ├── gimple-transpiler.md
│   └── system-integration.md
│
├── src/                           # Main source code
│   ├── __init__.py
│   │
│   ├── common/                    # Common utilities & type system
│   │   ├── __init__.py
│   │   └── types.py              # Type system (primitive, function, array, etc.)
│   │
│   ├── tokenizer/                 # Unicode symbol tokenization
│   │   ├── __init__.py
│   │   ├── vocabulary.py          # Symbol ↔ Token ID mapping (60+ symbols)
│   │   ├── tokenizer.py           # (TODO) String → Token stream
│   │   └── detokenizer.py         # (TODO) Token stream → Unicode symbols
│   │
│   ├── parser/                    # PAL syntax parsing
│   │   ├── __init__.py
│   │   ├── ast_nodes.py           # AST node definitions (Literal, BinaryOp, etc.)
│   │   ├── parser.py              # (TODO) Token stream → AST
│   │   └── lexer.py               # (TODO) Lexical analysis
│   │
│   ├── type_system/               # Type checking and inference
│   │   ├── __init__.py
│   │   ├── type_checker.py        # (TODO) AST → Typed AST
│   │   ├── type_inference.py      # (TODO) Hindley-Milner type inference
│   │   └── unification.py         # (TODO) Type unification algorithm
│   │
│   ├── compiler/                  # Compilation pipeline
│   │   ├── __init__.py
│   │   ├── semantic_analyzer.py   # (TODO) Scope, recursion, purity analysis
│   │   ├── symbol_table.py        # (TODO) Symbol tracking & scopes
│   │   └── compiler.py            # (TODO) Orchestrate pipeline
│   │
│   └── gimple/                    # GIMPLE IR generation
│       ├── __init__.py
│       ├── builder.py             # GIMPLE AST & code generator
│       ├── validator.py           # (TODO) GIMPLE validation
│       └── emitter.py             # (TODO) Emit GIMPLE to file
│
├── tests/                         # Test suite
│   ├── test_types.py
│   ├── test_tokenizer.py
│   ├── test_parser.py
│   ├── test_gimple.py
│   └── test_integration.py
│
├── examples/                      # Usage examples
│   ├── demo.py                    # Demonstration of core concepts
│   ├── bellman_ford.py            # (TODO) Bellman-Ford algorithm example
│   └── sorting.py                 # (TODO) Sorting algorithm example
│
└── README.md
```

## Core Components

### 1. Type System (`src/common/types.py`)

**Status**: ✅ Complete

Defines all PAL types with proper C mapping:
- `PrimitiveT`: ⊤, ⊥, 𝔹, ℤ, ℝ, 𝕾
- `FunctionT`: Domain → Codomain
- `ProductT`: T₁ × T₂ (structs)
- `UnionT`: T₁ ⊔ T₂ (tagged unions)
- `ArrayT`: T[N] (arrays/VLAs)
- `BoxedT`: □T (wrapped values)
- `PointerT`: T* (pointers)

**Features**:
- Type unification (Robinson's algorithm)
- Subtype checking
- C type conversion (for GIMPLE)
- Type environments (scopes)

### 2. Tokenizer Vocabulary (`src/tokenizer/vocabulary.py`)

**Status**: ✅ Complete

Manages 60+ Unicode symbols:
- Logical: ∀, ∃, ¬, ∧, ∨, →, ↔
- Set operations: ∈, ∉, ⊆, ⊃, ∪, ∩
- Type system: ⊤, ⊥, 𝔹, ℤ, ℝ, 𝕾, □
- Comparisons: =, ≠, <, ≤, >, ≥, ≡
- Arithmetic: +, -, ×, ÷, ⊕, ⊗, ⊙
- Structural: ⟨⟩, ⟦⟧, ⦅⦆, ⟪⟫, {}, []
- Binding: λ, μ, ≜, :, ∣, ,, ;, .
- Control: ?, ↩, loop, break, continue

**Token ID Ranges**:
- 0-255: Reserved (PAD, START, END, UNKNOWN, etc.)
- 256+: Symbol tokens

**Features**:
- Symbol ↔ Token ID mapping
- Precedence & associativity
- Category-based filtering
- Vocabulary size: ~4300 tokens

### 3. AST Nodes (`src/parser/ast_nodes.py`)

**Status**: ✅ Complete

Complete AST node hierarchy:
- **Expressions**: Literal, Variable, BinaryOp, UnaryOp, Application
- **Abstractions**: Lambda, Fixpoint
- **Control**: Conditional, Quantifier, Iteration
- **Binding**: Let, Definition
- **Advanced**: SetLiteral, Composition, TypeAnnotation, Return

**Features**:
- `SourceLocation` for error reporting
- Free variable detection
- AST pretty-printing for debugging

### 4. GIMPLE IR Builder (`src/gimple/builder.py`)

**Status**: ✅ Complete (Core)

Converts typed AST to GIMPLE (three-address code):
- `GimpleStatement` hierarchy (Assignment, Call, Branch, Return, etc.)
- `BasicBlock` for control flow grouping
- `GimpleFunction` for function representation
- `GimpleModule` for complete program
- `GimpleGenerator` class that walks AST and emits GIMPLE

**Supported Expressions**:
- ✅ Literals
- ✅ Variables
- ✅ Binary operations (+, -, ×, ÷, ∧, ∨, etc.)
- ✅ Unary operations (¬, -)
- ✅ Conditionals (if-then-else)
- ✅ Function calls
- ⚠️ Lambdas (placeholder)
- 🔲 Quantification (map to loops - TODO)
- 🔲 Iteration (TODO)
- 🔲 Fixpoints (TODO)

**Output**: Valid C-like GIMPLE that GCC can compile

## Implementation Roadmap

### Phase 1: Core Pipeline (30% complete)
- [x] Type system
- [x] Tokenizer vocabulary  
- [x] AST nodes
- [x] Basic GIMPLE generator
- [ ] Tokenizer (string → tokens)
- [ ] Parser (tokens → AST)
- [ ] Type checker (AST → typed AST)

### Phase 2: Advanced Compilation (0% complete)
- [ ] Semantic analyzer (scope, recursion, purity)
- [ ] Full GIMPLE code generation for complex features
- [ ] GIMPLE validator
- [ ] Integration with GCC backend

### Phase 3: Integration & Optimization (0% complete)
- [ ] Model inference integration
- [ ] Constrained decoding
- [ ] Performance benchmarking
- [ ] Optimization pass experiments

## Usage Example

```python
from src.parser.ast_nodes import *
from src.gimple.builder import GimpleGenerator

# Manually create AST (parser will do this automatically)
program = Program(definitions=[
    Definition(
        name="add",
        parameters=[Parameter("x", INT_TYPE), Parameter("y", INT_TYPE)],
        body=BinaryOp("+", Variable("x"), Variable("y")),
        return_type=INT_TYPE,
    )
])

# Generate GIMPLE
generator = GimpleGenerator()
generator.generate(program)
print(generator.emit_to_string())

# Output:
# int64_t add(int64_t x, int64_t y)
# {
#   t_1 = x + y;
#   return t_1;
# }
```

## Dependencies

- Python 3.9+
- No external dependencies (pure implementation)
- GCC 10+ (for final compilation to binary)

## Testing Strategy

1. **Unit Tests** (`tests/`)
   - Type system operations
   - Tokenizer vocabulary lookup
   - AST construction
   - GIMPLE generation correctness

2. **Integration Tests**
   - End-to-end compilation pipeline
   - GIMPLE output validation
   - GCC compilation of generated code

3. **Benchmark Tests**
   - Performance on standard algorithms
   - Compilation speed
   - Generated binary performance

## Next Development Priorities

1. **Implement Tokenizer** (week 1)
   - String → Token stream conversion
   - Unicode normalization
   - Scope/context tracking

2. **Implement Parser** (week 2-3)
   - Recursive descent with operator precedence
   - Error recovery
   - AST construction

3. **Implement Type Checker** (week 3-4)
   - Hindley-Milner inference
   - Type unification
   - Error messages

4. **Expand GIMPLE Generator** (week 4-5)
   - Loops and quantification
   - Complex control flow
   - Memory operations

5. **Testing & Validation** (week 5-6)
   - Test suite build-out
   - Integration testing
   - Performance tuning

## Architecture Principles

1. **Separation of Concerns**: Each module has single responsibility
2. **Immutable Types**: Use `dataclass(frozen=True)` for IR structures
3. **Explicit Errors**: No silent failures; all errors reported
4. **Preserve Semantics**: GIMPLE generation maintains meaning of PAL
5. **Target Optimization**: Generate GIMPLE that GCC can optimize well
