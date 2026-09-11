# PAL Language Specification

## Overview

PAL (Programmatic Abstraction Language) is a formal notation for expressing computational intent through complex Unicode symbols. It operates at a higher level of abstraction than traditional programming languages, focusing on:

- Declarative specification of logic and computation
- Type-safe, formally verifiable constructs
- Domain-agnostic representation of algorithms
- Efficient compilation to intermediate representation (IR)

## Language Principles

1. **Symbolic Formalism**: Use Unicode symbols instead of ASCII keywords
2. **Type Discipline**: All expressions have explicitly inferrable types
3. **Scope Explicitness**: Nesting and scoping are syntactically clear
4. **Compilation Efficiency**: Design for fast, deterministic compilation to IR

## Core Constructs

### 1. Type System

**Basic Types**:
```
⊤        : Top type (universal type, supertype of all)
⊥        : Bottom type (empty type, subtype of all)
𝔹        : Boolean type
ℤ        : Integer type
ℝ        : Real number type
𝕾        : String type
□T       : Type T (boxed/lifted)
T₁ → T₂  : Function type from T₁ to T₂
T₁ × T₂  : Product type (tuple)
T₁ ⊔ T₂  : Union type (disjoint union)
```

**Type Operations**:
```
T ⊆ U    : T is subtype of U
T ≡ U    : T equivalent to U (structural equality)
∀T. φ    : Universal type quantification
∃T. φ    : Existential type quantification
```

### 2. Terms & Expressions

**Variables**:
```
x, y, z                : Lowercase identifiers (terms/values)
T, U, V                : Uppercase identifiers (types/type vars)
_                      : Wildcard/don't-care
```

**Literals**:
```
𝟙                      : Boolean true (⊤)
𝟘                      : Boolean false (⊥)
42                     : Integer literal
3.14                   : Real literal
"text"                 : String literal
```

**Operators - Logical**:
```
¬φ                     : Negation (NOT)
φ ∧ ψ                  : Conjunction (AND)
φ ∨ ψ                  : Disjunction (OR)
φ → ψ                  : Implication (implies)
φ ↔ ψ                  : Biconditional (iff)
```

**Operators - Set/Collection**:
```
x ∈ S                  : Membership
x ∉ S                  : Non-membership
S ⊆ T                  : Subset
S ⊃ T                  : Superset
S ∪ T                  : Union
S ∩ T                  : Intersection
S \ T                  : Difference
{}                     : Empty set
{x, y, z}              : Set literal
{x ∈ S | φ(x)}         : Set comprehension
```

**Operators - Arithmetic**:
```
x ⊕ y                  : XOR (for bits/bools)
x ⊗ y                  : Generic multiplication/composition
x ⊙ y                  : Hadamard product (element-wise)
x ⊘ y                  : Generic division
```

### 3. Quantification & Binding

**Universal Quantification**:
```
∀x ∈ S. φ(x)           : For all x in S, property φ holds
∀T. ∀x : T. φ(x)       : Polymorphic: for all types T and x of type T
```

**Existential Quantification**:
```
∃x ∈ S. φ(x)           : There exists x in S such that φ(x)
∃T. ∃x : T. φ(x)       : There exists a type T and x of that type
```

**Lambda Abstraction**:
```
λx. e                  : Anonymous function: takes x, returns e
λT. e                  : Type-level abstraction
λ(x,y). e              : Multi-argument function
```

### 4. Control Flow & Structure

**Conditional**:
```
⟨ φ → e₁ ∣ e₂ ⟩         : If φ then e₁ else e₂
⟨ φ₁ → e₁ ∣ φ₂ → e₂ ∣ e₃ ⟩ : Multi-way conditional
```

**Iteration & Recursion**:
```
⟦ x ∈ S ∶ e(x) ⟧       : Map/iteration (apply e to each x in S)
𝜇f. e                  : Fixpoint/recursion (μ = minimal fixpoint)
```

**Scope & Binding**:
```
⦅ x ≜ e₁; φ(x) ⦆       : Let binding (x is defined as e₁ in scope φ)
⟪ x₁ ≜ e₁, x₂ ≜ e₂ ⟫   : Parallel definitions
```

### 5. Function Application & Composition

```
f(x)                   : Direct application
f·g                    : Composition (f then g)
f ⊙ g                  : Parallel/pointwise combination
f ⊕ g                  : Function union/overload
```

## Example: Factorial

```
factorial ≜ λn:ℤ. ⦅
  μf. λn. ⟨ n ≤ 1 → 1 ∣ n × f(n-1) ⟩
⦆ (n)
```

**Translation**:
- Define `factorial` as a function taking integer n
- Use fixpoint recursion (μf) to define recursive function
- If n ≤ 1 return 1, else return n × factorial(n-1)

## Example: List Operations

```
length ≜ λL:List[T]. ⟦ x ∈ L ∶ 1 ⟧ ⊕ 0

map ≜ λf:(T₁→T₂). λL:List[T₁]. ⟦ x ∈ L ∶ f(x) ⟧

filter ≜ λp:(T→𝔹). λL:List[T]. {x ∈ L | p(x)}
```

## Semantic Properties

### Type Soundness
Every well-formed PAL expression has a unique, verifiable type. Type errors are detected at compile-time.

### Determinism
PAL expressions have deterministic semantics. Given the same inputs, they always produce the same outputs.

### Compositionality
Complex expressions are built from simpler ones via explicit composition operators. The meaning of a complex expression depends only on the meaning of its parts.

## Formal Grammar (Abstract Syntax)

```
Program := Definition*

Definition := 
    | Identifier ≜ Expression
    | ∀T. Definition

Expression :=
    | Literal
    | Identifier
    | PrimaryExpr Op PrimaryExpr
    | λBinder. Expression
    | ∀Binder. Expression
    | ∃Binder. Expression
    | ⟨ Guard → Expression ∣ Expression ⟩
    | ⟦ Binder ∶ Expression ⟧
    | ⦅ Binding; Expression ⦆
    | 𝜇Identifier. Expression

Binder := Identifier | (Identifier : Type)

Binding := Identifier ≜ Expression [, Binding]*

PrimaryExpr := Literal | Identifier | (Expression)

Type := 
    | ⊤ | ⊥ | 𝔹 | ℤ | ℝ | 𝕾
    | Type → Type
    | Type × Type
    | Type ⊔ Type
    | □Type
```

## Extensibility

New domain-specific symbols can be added to PAL while maintaining:
- Type safety guarantees
- Compilation determinism
- Core language consistency

Extensions are managed in a registry and can be version-specific.

## Relationship to LLM Output

The 7B PAL Transcriber model learns to output valid PAL expressions given:
- Source code in a traditional language
- Mathematical specifications
- Natural language descriptions
- Hybrid multi-modal inputs

The model is trained with PAL's formal grammar as constraints, ensuring output is always compilable.
