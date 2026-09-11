# Intermediate Representation (IR) Design

## Overview

The PAL Intermediate Representation is a low-level, type-safe, SSA-form instruction set designed for:
- Efficient execution on both CPU and GPU
- Straightforward compilation to machine code
- Analysis and optimization
- Debugging and profiling
- Hardware-agnostic specification of computation

## Design Philosophy

1. **Single Static Assignment (SSA)**: Each variable assigned exactly once
2. **Type Explicit**: All operations carry full type information
3. **Control Flow Graph**: Clear dataflow and control dependencies
4. **Memory Safe**: No undefined behavior (bounds checking, type safety)
5. **Extensible**: Easy to add new opcodes and types for domain-specific operations

## IR Structure

```
Program := GlobalDecl* Function*

GlobalDecl := 
    | type_decl TypeName = TypeDef
    | const_decl ConstName : Type = Literal

Function :=
    | func_def FuncName(Param*) -> ReturnType {
        Label* Instruction*
    }

Param := ParamName : Type

Label := label_name:

Instruction :=
    | ValueInst       (assigns to virtual register)
    | EffectInst      (has side effects or control flow)
```

## Basic Type System

```
Type :=
    | i32, i64               # Signed integers
    | u32, u64               # Unsigned integers
    | f32, f64               # Floating point
    | bool                   # Boolean
    | void                   # No value
    | T[N]                   # Array of N elements of type T
    | {name₁:T₁, ..., nameₙ:Tₙ}  # Struct/record type
    | (T₁, ..., Tₙ) -> U     # Function type
```

## Value Instructions

Value instructions produce a result and assign to a virtual register (SSA).

### Arithmetic Operations

```
INST add   %result = add %a, %b          : (T, T) -> T
INST sub   %result = sub %a, %b          : (T, T) -> T
INST mul   %result = mul %a, %b          : (T, T) -> T
INST div   %result = div %a, %b          : (T, T) -> T  (error if b=0)
INST rem   %result = rem %a, %b          : (T, T) -> T  (remainder/modulo)
INST neg   %result = neg %a              : T -> T
```

### Bitwise Operations

```
INST and   %result = and %a, %b          : (T, T) -> T
INST or    %result = or %a, %b           : (T, T) -> T
INST xor   %result = xor %a, %b          : (T, T) -> T
INST not   %result = not %a              : T -> T
INST shl   %result = shl %a, %b          : (T, u32) -> T  (left shift)
INST shr   %result = shr %a, %b          : (T, u32) -> T  (right shift)
```

### Comparison Operations

```
INST eq    %result = eq %a, %b           : (T, T) -> bool
INST ne    %result = ne %a, %b           : (T, T) -> bool
INST lt    %result = lt %a, %b           : (T, T) -> bool
INST le    %result = le %a, %b           : (T, T) -> bool
INST gt    %result = gt %a, %b           : (T, T) -> bool
INST ge    %result = ge %a, %b           : (T, T) -> bool
```

### Type Operations

```
INST cast  %result = cast<T> %a          : Any -> T
INST sext  %result = sext %a             : (i32) -> i64  (sign extend)
INST zext  %result = zext %a             : (u32) -> u64  (zero extend)
INST trunc %result = trunc %a            : (i64) -> i32  (truncate)
```

### Memory Operations

```
INST load  %result = load %ptr           : (ptr<T>) -> T
INST alloc %result = alloc T [%size]     : () -> ptr<T>  (allocate array)
```

### Function Calls

```
INST call  %result = call %func(%args*)  : (fn_type) -> ReturnType
```

## Effect Instructions

Effect instructions don't produce values but affect program state or control flow.

### Store

```
INST store %ptr, %value                  : (ptr<T>, T) -> void
```

### Terminator Instructions (Control Flow)

```
INST br    br %label                     : Unconditional jump

INST br_if br_if %cond, %label_true, %label_false
           : Conditional branch (if cond then goto label_true else label_false)

INST ret   ret %value                    : Return from function

INST ret_void ret_void                   : Return void
```

## Data Operations

### Constants & Literals

```
INST const %result = const <Type> <Value>

Examples:
    %x = const i32 42
    %f = const f64 3.14
    %b = const bool true
    %a = const i32[3] [1, 2, 3]
```

### Aggregate Operations

```
INST struct_mk    %s = struct_mk {f1:%v1, f2:%v2, ...}
INST struct_get   %v = struct_get %s, field_name
INST struct_set   %s' = struct_set %s, field_name, %v
INST array_mk     %a = array_mk [%v1, %v2, ..., %vn]
INST array_get    %v = array_get %a, %idx
INST array_set    %a' = array_set %a, %idx, %v
```

## Special Operations

### Iteration (Parallel Map)

```
INST for_each %result = for_each %iterable, %lambda
           : Maps lambda over iterable, returns result array
```

### Fixpoint/Recursion Marker

```
INST mark_rec @label                     : Marks label as recursive entry point
```

### Quantification

```
INST forall %result = forall %set, %predicate
            : Returns bool (true if predicate holds for all elements)

INST exists %result = exists %set, %predicate
            : Returns bool (true if predicate holds for some element)
```

## Control Flow Graph

Functions are organized as basic blocks connected by control flow:

```
Function body:
  LABEL %entry:
    ...instructions without jumps...
    br_if %cond, %label_true, %label_false

  LABEL %label_true:
    ...instructions...
    br %merge

  LABEL %label_false:
    ...instructions...
    br %merge

  LABEL %merge:
    ...instructions...
    ret %result
```

**Properties**:
- Each basic block ends with a terminator (br, br_if, ret, ret_void)
- No jumps in middle of block
- Clear predecessors and successors for each block
- SSA form ensures no variable reassignment within block

## Example: Factorial

**PAL**:
```
factorial = λn:ℤ. 𝜇f. λn. ⟨ n ≤ 1 → 1 ∣ n × f(n-1) ⟩
```

**IR**:
```
func factorial(n: i32) -> i32 {
  label %entry:
    %cond = le %n, 1
    br_if %cond, %base_case, %recursive_case

  label %base_case:
    ret 1

  label %recursive_case:
    %n_minus_1 = sub %n, 1
    %rec_result = call @factorial(%n_minus_1)
    %result = mul %n, %rec_result
    ret %result
}
```

## Type Checking in IR

All IR instructions are fully typed. Type checker validates:

```
1. Operand types match instruction requirements
2. All type casts are valid (no lossy casts without explicit instruction)
3. All function calls match signature
4. Return type matches function declaration
5. All control flow paths return correct type
```

## Optimization Intermediate Forms

Before final IR emission, several forms may be generated:

1. **Unoptimized IR**: Direct translation from AST
2. **SSA IR**: Converted to full SSA form
3. **Optimized IR**: After applying optimization passes
4. **Final IR**: Target-specific lowering (GPU/CPU)

## Memory Model

**Stack vs Heap**:
- Automatic variables in function: stack-allocated
- `alloc` instruction: heap-allocated
- Lifetimes explicitly tracked (no garbage collection, but RAII-style)

**Borrowing** (future extension):
- Borrowed references for temporary access
- Ensures memory safety

## Extensibility Points

1. **New Opcodes**: Add INST entries for domain-specific operations
2. **New Types**: Extend type system with custom types
3. **New Attributes**: Annotate instructions with optimization hints
4. **Calling Conventions**: Support different call ABIs
5. **Target-Specific IR**: Generate IR for specific hardware

## IR Validation

Before execution, IR is validated for:

```
✓ All registers defined before use
✓ Type consistency across instructions
✓ Control flow termination (all paths have exit)
✓ No undefined jumps
✓ SSA property maintained
✓ Memory safety (bounds checking)
✓ Function call signature matching
```

## Serialization

IR can be serialized to text or binary format for:
- Debugging and inspection
- Caching compiled programs
- Transmission across systems

**Text Format** (human-readable):
```
module arllm_v1

type MyType = {field1: i32, field2: f64}

func process(x: i32) -> i32 {
  ...
}
```

**Binary Format** (efficient storage):
```
[Magic Bytes] [Version] [Module Data] [Functions] ...
```

## Performance Considerations

- IR is close to machine code (1 IR instruction ≈ 1-5 machine instructions)
- Minimal overhead for execution on CPU
- Directly translatable to GPU kernels (CUDA/OpenCL)
- Optimization passes can reduce IR size 10-30%

## Relationship to Execution

The IR serves as the interface between:
- **Compiler**: Produces IR from PAL source
- **Optimizer**: Improves IR efficiency
- **Backends**: Compile IR to machine code (LLVM, GPU drivers, etc.)
- **Interpreter**: Direct IR execution (slow but useful for debugging)
