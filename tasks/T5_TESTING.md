# T5: Implement Testing & Benchmarking Suite (Not Started)

> ❌ Status: Not Started

## Description
Build a comprehensive testing infrastructure to validate every component of the PAL/Ø compiler pipeline and measure performance.

Testing is critical because:
- The system uses non-standard syntax (Unicode glyphs, de Bruijn indices)
- Type inference must be sound and complete
- GIMPLE output must compile cleanly with GCC
- Performance targets (1M tokens/sec) require measurement

## Implementation Requirements

### Structure
```
tests/
├── unit/              # Component-level tests
│   ├── tokenizer/
│   ├── parser/
│   ├── typechecker/
│   └── gimple/
├── integration/       # End-to-end flow tests
├── benchmarks/        # Performance measurements
└── examples/          # Reference inputs and expected outputs
```

### Test Categories

#### 1. Tokenizer Tests (`tests/unit/tokenizer/`)
- Input: `"λλ+₁₀"` → Expected: `[LAMBDA, LAMBDA, ADD, VAR1, VAR0]` (token IDs from T0)
- Input: `"#42"` → Expected: `NUM(42)`
- Input: `"ᵥ15"` → Expected: `VAR15`
- Input: `"λ⁇>₀#0₀∸₀"` → validate all glyphs
- Edge case: empty string, invalid UTF-8, unknown glyph → should emit UNKNOWN or error

#### 2. Parser Tests (`tests/unit/parser/`)
- Parse "λλ+₁₀" → AST with correct nested Lambda and Var(index=1), Var(index=0)
- Parse "λ⦂λ+₁₀ ℤ" → TANNOT wrapping lambda, type annotated as Int
- Parse "⟳{#10}+₉" → Iteration over array of 10 elements, accumulator + index 9
- Invalid: too few arguments to binary op → SyntaxError
- Source locations preserved in AST nodes

#### 3. Type Checker Tests (`tests/unit/typechecker/`)
- "λλ+₁₀" → type = Int → Int → Int
- "λλ>₁₀" → type = Int → Int → Bool (correctly inferred from > operator)
- "λλ×₁₀" → type = Int → Int → Int (multiplication)
- "λλ+₂₁" → Type error: VAR2 out of bounds in scope depth 2
- "⦂λλ+₁₀ ℤ" → type = Int
- "∀x∈[1,2] x>0" → type = Bool

#### 4. GIMPLE Builder Tests (`tests/unit/gimple/`)
- Generate GIMPLE for `λλ+₁₀` → two functions, closure parameters, SSA form
- Verify no undefined variables in output
- Validate that all variables are declared before use
- Output compiles with GCC: `gcc -S -O3 -x c`
- Check generated assembly has expected optimizations (e.g., no unused temporaries)

#### 5. Integration Tests (`tests/integration/`)
- End-to-end flow:
  ```python
  input_str = "λλ+₁₀"
  tokens = tokenizer.tokenize(input_str)
  ast = parser.parse(tokens)
  typed_ast = type_checker.check(ast)
  gimple_ir = gimple_builder.generate(typed_ast)
  gcc_output = compile_to_asm(gimple_ir) # using subprocess
  assert "add" in gcc_output # verify correct instruction
  ```
- Test all GLYPH_SPEC.md examples end-to-end

#### 6. Benchmarking (`tests/benchmarks/`)
- Generate random PAL/Ø programs of increasing size (100, 1k, 10k nodes)
- Measure:
  - Tokenization speed: tokens/sec
  - Parsing speed: AST nodes/sec
  - Type checking speed: types/inferred/sec
  - GIMPLE generation: IR nodes/sec
  - GCC compilation latency (for 1k-node program)
- Target performance:
  - Tokenizer: >1M tokens/sec
  - Parser: >100k nodes/sec
  - Type checker: >50k nodes/sec
  - GIMPLE gen: >50k nodes/sec

## Dependencies
- ✅ T0–T4: All components must be implemented and working
- ✅ GLYPH_SPEC.md: source of truth for test inputs

## Acceptance Criteria

1. `pytest tests/unit/` → 100% pass rate on all defined cases
2. Integration tests run successfully for at least 5 GLYPH_SPEC.md examples
3. Benchmarks produce CSV logs with metrics and targets
4. All test data is versioned in `tests/examples/`
5. CI pipeline runs tests automatically (to be configured later)
6. Test output includes source location on failures: "Test failed at line 12: expected VAR0, got VAR1"

## Implementation Plan
1. Create directory structure and `__init__.py` files
2. Write test fixtures from GLYPH_SPEC.md examples as Python dicts
3. Implement each unit test module using `pytest`
4. Write benchmark runner with `time.perf_counter()`
5. Add test coverage report (use `coverage.py`)
6. Integrate with `examples/demo.py` for manual validation
7. Document how to run tests in README.md

## Blocked By
- ✅ All T0–T4 components must be implemented first

## Next Steps After Completion
→ Add Dockerfile for reproducible builds, create CI/CD pipeline (GitHub Actions), publish performance benchmarks.
