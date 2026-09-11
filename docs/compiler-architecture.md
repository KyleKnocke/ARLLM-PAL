# Compiler Architecture: PAL → Intermediate Representation

## Overview

The PAL Compiler transforms valid PAL (Programmatic Abstraction Language) expressions into an Intermediate Representation (IR) suitable for execution, analysis, and optimization. The compilation pipeline consists of distinct phases, each with clear responsibilities and well-defined interfaces.

## Compilation Pipeline

```
PAL Source
    ↓
[Tokenizer] → Tokens
    ↓
[Lexer] → Lexical Tokens with Metadata
    ↓
[Parser] → Abstract Syntax Tree (AST)
    ↓
[Type Checker] → Typed AST
    ↓
[Semantic Analyzer] → Annotated AST
    ↓
[GIMPLE Code Generator] → GIMPLE IR
    ↓
[GIMPLE Validator] → Valid GIMPLE
    ↓
[Emit to GCC] 
    ↓
[GCC Optimization Pipeline] → Optimized Machine Code
    ↓
Final Binary (Fastest Possible)
```

**Note:** Optimization is delegated to GCC, which leverages 60+ years of optimization research.

## Phase 1: Tokenization

**Input**: Stream of Unicode characters  
**Output**: Sequence of token IDs  
**Component**: `tokenizer/` module (see tokenizer-architecture.md)

**Responsibilities**:
- Recognize valid symbols and symbol sequences
- Inject special tokens (START, END, scope markers)
- Validate token sequence structure
- Handle context-aware tokenization

## Phase 2: Lexical Analysis (Lexer)

**Input**: Token ID sequence  
**Output**: Lexical tokens with source metadata (line, column, scope)

**Lexical Token Structure**:
```
LexToken {
    token_id: int,
    symbol: str,
    source_loc: {line: int, col: int},
    scope_depth: int,
    type_context: Type,
    raw_bytes: bytes,
}
```

**Responsibilities**:
- Attach metadata to each token
- Track scope nesting depth
- Maintain symbol table for variable scoping
- Detect lexical errors early

## Phase 3: Parser

**Input**: Lexical token stream  
**Output**: Abstract Syntax Tree (AST)

**AST Node Types**:
```
ASTNode := 
    | LiteralNode(value, type)
    | VariableNode(name, type)
    | BinaryOpNode(op, left: ASTNode, right: ASTNode)
    | UnaryOpNode(op, operand: ASTNode)
    | ApplicationNode(func: ASTNode, arg: ASTNode)
    | LambdaNode(param: Binder, body: ASTNode)
    | LetNode(binding: Binding, body: ASTNode)
    | ConditionalNode(condition: ASTNode, then_branch: ASTNode, else_branch: ASTNode)
    | QuantifierNode(op: ∀|∃, binder: Binder, body: ASTNode)
    | FixpointNode(var: str, body: ASTNode)
    | IterationNode(binder: Binder, range: ASTNode, body: ASTNode)
    | DefinitionNode(name: str, body: ASTNode)
```

**Parser Strategy**: 
- Recursive descent with operator precedence climbing
- Left-associative for most operators
- Right-associative for function composition (→)
- Symbol table integrated during parsing

**Responsibilities**:
- Convert token stream to AST structure
- Enforce grammar rules
- Report parse errors with source locations
- Build scope hierarchy

## Phase 4: Type Checking

**Input**: Untyped AST  
**Output**: Typed AST (all nodes annotated with types)

**Type Inference Algorithm**: Hindley-Milner style
```
Infer(node) →
    case node of:
        LiteralNode(v) → type_of_literal(v)
        VariableNode(x) → lookup_type(x)
        BinaryOpNode(⊕, l, r) → 
            T_l = Infer(l)
            T_r = Infer(r)
            return unify(T_l, T_r, op_signature(⊕))
        LambdaNode(x:T, e) → 
            T_body = Infer(e) with x:T in scope
            return T → T_body
        ApplicationNode(f, x) →
            T_f = Infer(f)
            T_x = Infer(x)
            check(T_f is function type T_x → T_ret)
            return T_ret
        LetNode(x=e₁; e₂) →
            T₁ = Infer(e₁)
            T₂ = Infer(e₂) with x:T₁ in scope
            return T₂
        ...
```

**Type Unification**:
- Constraint collection during inference
- Robinson's unification algorithm
- Occur check for soundness
- Error reporting on unification failure

**Responsibilities**:
- Verify type safety
- Infer missing types
- Report type errors
- Annotate AST with complete type information

## Phase 5: Semantic Analysis

**Input**: Typed AST  
**Output**: Annotated AST with semantic information

**Semantic Checks**:
- Name resolution and scope validation
- Recursion detection (identifies recursive functions)
- Side effect detection (marks pure vs. impure)
- Termination analysis (detects infinite loops)
- Unused variable warnings

**Symbol Table Enhancement**:
```
SymbolEntry {
    name: str,
    type: Type,
    scope: ScopeLevel,
    defined_at: SourceLocation,
    used_in: [SourceLocation],
    is_recursive: bool,
    is_pure: bool,
}
```

**Responsibilities**:
- Perform deeper analysis than type checking
- Prepare data for IR generation
- Detect logical errors (unused bindings, unreachable code, etc.)
- Perform feasibility analysis

## Phase 6: GIMPLE Code Generation

**Input**: Annotated AST  
**Output**: GCC GIMPLE Intermediate Representation

See `gimple-transpiler.md` for detailed GIMPLE transpiler specification.

**Code Generation Strategy**: 
- Recursive tree-walk with control flow graph construction
- Three-address code generation (GIMPLE form)
- SSA form for variables
- Type-preserving lowering to GIMPLE constructs
- Optimization hints injected (likely branches, function attributes, etc.)

**Example Translation**:
```
PAL: λx:ℤ. x + 1

AST: LambdaNode(
    param: x:ℤ,
    body: BinaryOpNode(
        op: +,
        left: VariableNode(x),
        right: LiteralNode(1)
    )
)

GIMPLE: 
    int add_one(int x)
    {
      int D.1234;
      
      D.1234 = x + 1;
      return D.1234;
    }
```

**Responsibilities**:
- Translate AST to valid GIMPLE representation
- Map PAL types to C types (via GIMPLE)
- Build CFG from control flow constructs
- Insert optimization annotations
- Preserve semantic intent for GCC to optimize

## Phase 7: GIMPLE Validation

**Input**: Generated GIMPLE  
**Output**: Validated GIMPLE (or errors)

**Validation Rules**:
- All variables used before defined (within CFG paths)
- All labels are valid and reachable
- Type consistency across assignments
- Control flow is well-formed (CFG properties)
- Memory safety (no undefined access)
- SSA properties maintained

**Responsibilities**:
- Verify GIMPLE correctness before passing to GCC
- Detect code generation bugs early
- Report validation errors with source locations

## Phase 8: GCC Compilation Pipeline

**Input**: Valid GIMPLE (or emitted C code)  
**Output**: Optimized machine code

We delegate to GCC's optimizer which provides:
- Constant folding and propagation
- Dead code elimination
- Inlining and interprocedural analysis
- Loop unrolling and vectorization (auto-SIMD)
- Alias analysis and cache optimization
- Register allocation
- Instruction scheduling
- Target-specific optimizations

**Our Role**: Ensure we emit GIMPLE/C that GCC can optimize effectively:
- Include optimization hints (likely branches, function attributes)
- Avoid obscuring logic with unnecessary complexity
- Preserve high-level structure where possible
- Use GCC pragmas for parallelization hints (#pragma omp simd)

## Error Handling Strategy

**Error Types**:
1. **Lexical Errors**: Invalid symbol sequences
2. **Parse Errors**: Grammatical violations
3. **Type Errors**: Type mismatch or inference failure
4. **Semantic Errors**: Undefined variables, recursion issues
5. **Compilation Errors**: Code generation failures
6. **GIMPLE Validation Errors**: Generated GIMPLE correctness issues

**Error Reporting**:
```
CompilerError {
    phase: str,
    location: SourceLocation,
    code: int,
    message: str,
    context: str,
    suggestion: str,
}
```

**Recovery Strategy**:
- Collect multiple errors before stopping
- Attempt error recovery when possible (e.g., skip to next definition)
- Provide actionable suggestions for fixes

## Implementation Considerations

**Language**: Python (with optional Cython for hot paths)

**Dependencies**:
- AST library: Custom classes or AST module
- Type inference: Custom Hindley-Milner implementation or use `typing` module
- IR: Custom data structures or LLVM binding

**Performance Targets**:
- Tokenizer: 1M tokens/sec
- Parser: 100k nodes/sec
- Type checker: 50k nodes/sec
- IR generation: 50k nodes/sec
- Total E2E: 1000 PAL programs/sec (~10KB each)

**Memory**: <1GB for typical PAL program (< 100k LOC equivalent)

## Testing Strategy

1. **Unit Tests**: Each phase independently
2. **Integration Tests**: E2E compilation
3. **Property Tests**: Type soundness verification
4. **Regression Tests**: Known bug cases

## Extensibility

New language features (domain-specific symbols/operations) require:
1. Tokenizer symbol registration
2. Grammar extension in parser
3. Type signature addition
4. IR opcode definition
5. Code generation template
