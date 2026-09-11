# PAL/Ø Compiler Context Graph

## Overview
This document maps the architecture, dependencies, and development tasks for the PAL/Ø compiler pipeline.
Each node represents a component or task. Arrows indicate data flow and dependency order.

```
                          [Input Glyph Stream]
                                     |
                                     v
                         +---------------------+
                         |   Tokenizer (T1)    | ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
                         +----------+----------+                                          |
                                    |                                                     |
                                    v                                                     |
                          +---------------------+                                         |
                          |     Parser (T2)     | ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
                          +----------+----------+                                          |
                                    |                                                     |
                                    v                                                     |
                          +---------------------+                                         |
                          | Type Checker (T3)   | ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
                          +----------+----------+                                          |
                                    |                                                     |
                                    v                                                     |
                          +---------------------+                                         |
                          |  GIMPLE Builder (T4)| ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
                          +----------+----------+                                          |
                                    |                                                     |
                                    v                                                     |
                          +---------------------+                                         |
                          |     GCC Backend     | → Optimized Binary                      |
                          +---------------------+                                         |
                                     |                                                    |
                                     v                                                    |
                                [Optimized Machine Code]                                  |
                                                                                          |
                    ┌─────────────────────────────────────────────────────────────────────┐
                    │                                                                     │
       ┌────────────▼───────────┐     ┌──────────────────────┐      ┌─────────────────────┐
       |  Vocabulary (T0)         |     |  Type System (TS)    |      |  AST Nodes (AST)     |
       | * Glyph ↔ Token ID map   |     | * Primitive types    |      | * ExprNode hierarchy |
       | * Arity lookup           |     | * Composite types    |      | * node_to_type()     |
       | * Reserved tokens        |     | * Unification logic  |      | * Source location    |
       +────────────┬───────────+     +──────────────────────+      +─────────────────────+
                    |                                                         |
                    └───────────────┬───────────────────────────────────────┘
                                    v
                          [GLYPH_SPEC.md - Single Source of Truth]
```

## Dependency Order
1. **Vocabulary (T0)** → Required by Tokenizer & Parser  
2. **Type System (TS)** → Required by Type Checker & GIMPLE Builder  
3. **AST Nodes (AST)** → Required by Parser, Type Checker, GIMPLE Builder  
4. **Tokenizer (T1)** → First pipeline step, depends on Vocabulary  
5. **Parser (T2)** → Depends on Tokenizer and AST Nodes  
6. **Type Checker (T3)** → Depends on Parser and Type System  
7. **GIMPLE Builder (T4)** → Depends on Type Checker and AST Nodes  
8. **GCC Backend** → Final step, depends on GIMPLE output

## Task Legend
- ✅ Completed
- 🚧 In Progress
- ❌ Not Started
- ⚠️ Partially Implemented
```