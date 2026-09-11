# PAL/Ø Sieve-Based Compiler Test Framework

This system tests the compiler through successive "sieves" — each layer filters out incorrect programs and validates correct behavior.

## Sieve Layers (in order):

1. **Lexical Sieve** - Validates glyph tokenization matches Opcode encoding
2. **Syntactic Sieve** - Checks well-formed AST structure by arity rules
3. **Semantic Sieve** - Enforces type correctness and variable scoping
4. **Behavioral Sieve** - Executes programs against known outputs (e.g., prime sieve)
5. **Optimization Sieve** - Validates that optimizations preserve semantics
6. **Agent Reasoning Sieve** - Agent must predict output before execution

Each sieve is a standalone test module, but failures cascade upward. Passing all sieves = valid PAL/Ø program.

## Example: Prime Sieve Program

A program that computes primes ≤ 10 using PAL/Ø glyphs will be our canonical test case.