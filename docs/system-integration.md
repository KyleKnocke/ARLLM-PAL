# System Integration & Model Training

## Overview

This document describes how the tokenizer, PAL language, compiler, and 7B LLM model work together as an integrated system.

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│ Input Sources                                                    │
│ - Source code (Python, C++, etc.)                              │
│ - Mathematical specifications                                  │
│ - Natural language descriptions                                │
│ - Hybrid multi-modal inputs                                    │
└────────────┬────────────────────────────────────────────────────┘
             │
             ↓
┌─────────────────────────────────────────────────────────────────┐
│ PAL Tokenizer (Fixed)                                           │
│ - Unicode symbol vocabulary (4096-8192 tokens)                 │
│ - Context-aware tokenization                                   │
│ - Scope and type tracking                                      │
└────────────┬────────────────────────────────────────────────────┘
             │
             ↓
        Token Stream
             │
             ↓
┌─────────────────────────────────────────────────────────────────┐
│ 7B LLM (ARLLM-PAL Transcriber)                                  │
│ - Input Embedding Layer (4096-8192 → 4096 dim)                │
│ - 32 Transformer Blocks                                        │
│ - Output Projection (4096 → 4096-8192 logits)                 │
│ - Constrained Decoding (only valid PAL tokens)                │
└────────────┬────────────────────────────────────────────────────┘
             │
             ↓
        PAL Token Stream (Model Output)
             │
             ↓
┌─────────────────────────────────────────────────────────────────┐
│ Detokenizer                                                     │
│ - Reconstruct Unicode symbol sequences from token IDs          │
│ - Validate symbol boundaries                                   │
└────────────┬────────────────────────────────────────────────────┘
             │
             ↓
        PAL Source Code
             │
             ↓
┌─────────────────────────────────────────────────────────────────┐
│ PAL Compiler                                                    │
│ - Tokenizer (token validation)                                 │
│ - Parser (AST generation)                                      │
│ - Type Checker (type inference & verification)                │
│ - Semantic Analyzer (scope, recursion, purity)                │
│ - GIMPLE Generator (three-address code)                       │
│ - GIMPLE Validator (correctness checking)                     │
└────────────┬────────────────────────────────────────────────────┘
             │
             ↓
        GIMPLE Intermediate Form
             │
             ↓
┌─────────────────────────────────────────────────────────────────┐
│ GCC Compilation Pipeline                                        │
│ - Middle-end Optimizations (inlining, loop unroll, vectorize)  │
│ - Back-end (register allocation, instruction scheduling)       │
│ - Target-specific codegen (x86, ARM, MIPS, RISC-V, etc.)      │
└────────────┬────────────────────────────────────────────────────┘
             │
             ↓
    Optimized Machine Code (Binary)
```

## Component Interactions

### 1. Tokenizer ↔ Model

**Forward (Tokenization)**:
- Input: Arbitrary Unicode sequence
- Process: Symbol recognition, context tracking, token mapping
- Output: Token ID sequence (e.g., [1, 256, 257, 300, 2])
- Loss: Some symbols may map to UNKNOWN token if not in vocabulary

**Reverse (Detokenization)**:
- Input: Token ID sequence from model (logits → argmax)
- Process: Token ID → symbol lookup, symbol boundary resolution
- Output: Unicode symbol sequence
- Constraint: Only tokens in vocabulary range 256-8191 are "real" PAL tokens

### 2. Model ↔ Constrained Decoding

**During Inference**:
```
For each timestep t:
    logits = model(input_tokens[:t])
    # Constrain to only valid next tokens
    valid_tokens = [tid for tid in range(vocab_size) 
                    if is_valid_next_token(input_tokens[:t], tid)]
    constrained_logits = logits.clone()
    constrained_logits[~valid_tokens] = -inf
    
    next_token = argmax(constrained_logits)  # or sample with temperature
    input_tokens.append(next_token)
```

**Validity Checking**:
- Type context: Only allow tokens consistent with current type
- Scope depth: Disallow close-scope if at top level
- Grammar rules: Use cached grammar automaton

### 3. Compiler ↔ IR

**Compilation Flow**:
```
PAL Source (unicode string)
    ↓ [detokenized from model]
    ↓
Tokenize (validate & convert to token stream)
    ↓
Parse (AST)
    ↓
Type Check (annotated AST)
    ↓
Semantic Analysis (scope, recursion, purity)
    ↓
Generate GIMPLE (three-address code form)
    ↓
Validate GIMPLE (correctness)
    ↓
[GCC Compilation]
    - Optimization (constant folding, inlining, vectorization, etc.)
    - Code generation (register allocation, scheduling)
    ↓
Machine Code (optimized binary)
```

**Bidirectional Information**:
- Type information flows forward (parser → type checker → code gen)
- Error information flows backward (GIMPLE validator → semantic analyzer → parser)
- Debugging info attached to GIMPLE from all phases

## Model Architecture Details

### 7B ARLLM-PAL Configuration

```
Model Hyperparameters:
├── Parameters: 7,000,000,000 (7B)
├── Vocabulary Size: 4096-8192 (depends on final symbol count)
├── Hidden Dimension: 4096
├── Attention Heads: 32
├── Head Dimension: 128 (4096 / 32)
├── Num Layers: 32
├── Intermediate Dim: 11008 (4 * hidden_dim)
├── Max Sequence Length: 4096 tokens
├── Positional Encoding: Rotary (RoPE)
└── Activation: SwiGLU

Compute Profile (A100 GPU):
├── Tokens/sec (inference): ~1000-2000
├── Batch size (max): 16 @ fp16
├── Memory (weights): 14GB @ fp16
├── Memory (activations): 1-2GB @ fp16 + cache
├── Total GPU memory used: 16GB (fits in your GPU!)
```

### Training Strategy

**Phase 1: Architecture Validation**
- Train on synthetic PAL programs
- Verify tokenizer/detokenizer correctness
- Tune hyperparameters

**Phase 2: Supervised Transcription**
- Collect training data:
  - Source code → PAL via human annotation or symbolic translation
  - Specifications → PAL via symbolic translation
  - Natural language → PAL via human annotation
- Train on {input, target_PAL} pairs
- Supervised fine-tuning until convergence

**Phase 3: Constrained Decoding**
- Validate model outputs are always compilable
- Implement grammar-constrained beam search
- Test on diverse input domains

**Phase 4: Optimization**
- Distillation (if larger teacher model available)
- Quantization for inference speedup (int8/fp16)
- Knowledge distillation from 70B version

### Data Pipeline

```
Raw Input Data
    ↓
[Preprocessing]
    - Convert to standard format
    - Verify input/output pairs
    - Augmentation (if applicable)
    ↓
Cleaned Dataset
    ↓
[Tokenization]
    - Tokenize inputs (various modalities)
    - Tokenize target PAL
    - Create attention masks
    ↓
Token Dataset
    ↓
[DataLoader]
    - Batching (samples by input length)
    - Shuffling
    - Caching
    ↓
Model Training
```

## Training Data Sources

1. **Synthetic Data**
   - Generated from grammar rules
   - Algorithmic program synthesis
   - Random PAL expressions with known semantics

2. **Real Code Translation**
   - Existing codebases (Python, C++, etc.)
   - Algorithmic problems (LeetCode, Project Euler)
   - Academic algorithm descriptions

3. **Mathematical Specifications**
   - Formal theorem specifications (Lean, Coq)
   - Mathematical papers in formal notation
   - Type signatures and contracts

4. **Hybrid**
   - Multi-modal inputs (code + docs + type hints)
   - Annotated examples (code + description + PAL)
   - Chain-of-thought reasoning

## Inference Pipeline

```
User Input (code, spec, or description)
    ↓
[Tokenize Input]
    - Normalize and tokenize
    ↓
Input Tokens
    ↓
[Model Forward Pass]
    - Process through 32 transformer layers
    ↓
Output Logits
    ↓
[Constrained Decoding]
    - Apply grammar constraints
    - Sample next token (or greedy)
    ↓
Output Token
    ↓
[Accumulate]
    - Add to output sequence
    - Check stopping conditions ([END] token)
    ↓
PAL Token Sequence
    ↓
[Detokenize]
    - Reconstruct Unicode
    ↓
PAL Source
    ↓
[Compile to GIMPLE]
    - Tokenize, parse, type check, semantic analysis
    - Generate GIMPLE three-address code
    ↓
GIMPLE Intermediate Form
    ↓
[GCC Compilation]
    - Optimization and code generation
    ↓
Optimized Binary / Result
```

## Error Handling & Feedback Loop

**During Training**:
1. Model outputs invalid PAL → loss signal
2. Compiler fails → provides error location
3. GIMPLE validation fails → semantic error signal
4. Gradients backpropagate through losses

**During Inference**:
1. Constrained decoding prevents invalid tokens
2. If PAL is invalid (compilation fails) → human in the loop or re-sample
3. If GIMPLE is invalid → report compiler error to user
4. If GCC compilation fails → report optimization/codegen error
5. Can retry with different temperature or beam size

**Continuous Improvement**:
- Log all failures for analysis
- Retrain on failure cases
- Expand vocabulary if needed symbols are common
- Fine-tune on specific domains

## Performance Targets

```
Model Inference (AMD 16GB GPU):
├── Latency: 100-500ms per PAL generation
├── Throughput: 2-10 programs/sec
├── Memory: ~14-15GB (fits exactly)

Transpilation (PAL → GIMPLE):
├── Latency: 1-5ms for typical program
├── Size: ~10KB average GIMPLE code
├── Validation: <1ms

GCC Compilation (GIMPLE → Binary):
├── Latency: 100ms - 5sec (depends on -O level and program size)
├── Binary size: Optimized for speed (not minimal)

E2E System:
├── Input → Optimized Binary: 200ms - 10sec
├── Parallelizable stages: tokenization, model, detokenization
├── Bottleneck: GCC optimization (can be parallelized with -j)
```

## Hardware Considerations

**Your Setup (16GB AMD GPU + 64GB RAM)**:
✓ Runs full model in fp16
✓ Can do batch inference (batch_size=2-4)
✓ Can cache intermediate results in RAM
✓ Suitable for development and experimentation
✗ Not optimal for large-scale production (would need multiple GPUs or larger models)

**Future Scaling**:
- Quantization to int8 (halves memory: 7GB)
- Model parallelism across GPUs
- Mixture of Experts (MoE) for sparse computation
- Distilled 1B version for edge devices

## Integration Testing

```
Test Suite:
├── Unit Tests
│   ├── Tokenizer (symbol boundaries, edge cases)
│   ├── Parser (grammar coverage)
│   ├── Type Checker (type inference correctness)
│   └── IR Generator (semantic preservation)
├── Integration Tests
│   ├── E2E compilation (PAL → IR)
│   ├── Model + Compiler (output is compilable)
│   └── Detokenizer + Tokenizer (round-trip)
├── Property Tests
│   ├── Type safety (well-typed PAL → well-typed IR)
│   ├── Semantic preservation (input PAL ≡ compiled IR)
│   └── Determinism (same input → same output)
└── Performance Tests
    ├── Latency benchmarks
    ├── Memory profiling
    └── Throughput testing
```

## Version Management

```
Version Format: ARLLM-PAL-7b-v{major}.{minor}.{patch}

Breaking Changes (major):
- Vocabulary changes (new symbols or removal)
- Grammar changes (new constructs)
- IR opcode changes

Backward Compatible (minor):
- New optimization passes
- Performance improvements
- Bug fixes

Patch:
- Typo fixes
- Documentation updates
```

## Deployment Checklist

- [ ] Tokenizer fully tested and validated
- [ ] PAL grammar formalized and documented
- [ ] Compiler phases complete and tested
- [ ] IR design finalized
- [ ] Model architecture selected and validated
- [ ] Training data collected and preprocessed
- [ ] Training pipeline set up
- [ ] Inference pipeline with constraints implemented
- [ ] E2E testing passed
- [ ] Documentation complete
- [ ] Performance benchmarks meet targets
