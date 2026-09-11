# T4: Implement GIMPLE Builder (Partially Implemented)

> ⚠️ Status: Partially Implemented

## Description
Extend the existing `GimpleGenerator` to fully support PAL/Ø's semantics and generate correct, optimized GIMPLE IR compatible with GCC.

Current implementation supports basic literals, binary ops, conditionals, and function calls — but lacks:
- Lambda hoisting (PAL/Ø functions are anonymous)
- Proper handling of de Bruijn indices → variable names in GIMPLE
- Iteration (`⟳`) → loop generation
- Quantification (∀∃) → reduction loops
- Global variable references (`GVar`)
- Array operations (`CONS`, `IDX`)
- Type annotations (`⦂`) → type-checked C declarations

## Implementation Requirements

### Input
Typed AST from T3:
```python
Lambda(
  body=Lambda(
    body=BinaryOp("+", Var1, Var0),
    type=(Int → Int)
  ),
  type=(Int → Int → Int)
)
```

### Output
GIMPLE IR:
```c
int64_t lambda_1(int64_t x) {
  int64_t y;
  int64_t t0;
  int64_t t1;
  goto entry;
entry:
  t0 = x;          // outer binder (x)
  t1 = y;          // inner binder (y)
  return t0 + t1;
}

int64_t lambda_2(int64_t z) {
  int64_t result;
  result = lambda_1(z);
  return result;
}
```
Wait — correction: the AST is `λλ+₁₀`, meaning a single function that returns a nested lambda.

Correct GIMPLE:
```c
// Outer function returning inner function
struct { int64_t (*func)(int64_t); } lambda_1(int64_t x) {
  struct { int64_t (*func)(int64_t); } closure;
  
  // Inner function captured with binding to x
  int64_t inner_func(int64_t y) {
    return x + y;  // x is captured from outer scope
  }
  closure.func = inner_func;
  return closure;
}
```

But GIMPLE does not support closures directly. So we must **hoist**.

### Correct Approach: Lambda Hoisting
- Each `Lambda` node → generate a separate top-level function
- Arguments are parameters of that function
- Free variables become additional parameters (closure)

So `λλ+₁₀` becomes:
```c
int64_t inner_func(int64_t y, int64_t x) {  // captured outer var passed as arg
  return x + y;
}

struct { int64_t (*func)(int64_t,int64_t); } outer_func(int64_t x) {
  struct { int64_t (*func)(int64_t,int64_t); } closure;
  closure.func = inner_func;
  return closure;
}
```

## Key Logic
1. **Lambda Hoisting**:
   - When encountering `Lambda`, generate new function name: `f_{id}`
   - Parameters → formal args to the new function
   - Body → body of the new function
   - Free variables (de Bruijn references outside scope) → passed as additional arguments (closure)
2. **GVar Support**:
   - Map GVar0–GVar9 and GVAR_ESC to top-level function names by declaration order
3. **Iteration (`⟳`)**:
   - `⟳ iterable init body` → generate for loop with accumulator
   ```c
   acc = init;
   for (elem : iterable) {
     acc = body(acc, elem);
   }
   return acc;
   ```
4. **Quantification (`∀`, `∃`)**:
   - Reduce to iteration over array + boolean accumulation
5. **Array Ops**:
   - `CONS x xs` → prepend to list (allocate new array)
   - `IDX arr i` → `arr[i]`
6. **Type Annotations (`⦂`)**:
   - Use type info to generate correct C types in declarations and casts
7. **GIMPLE Statements**:
   - Use `GimpleAssignment`, `GimpleCall`, `GimpleConditionalBranch`, `GimpleLabel`, etc.
   - Ensure SSA form: each variable assigned exactly once

## Dependencies
- ✅ T3: Type Checker (provides typed AST)
- ✅ AST Nodes (to traverse nodes and extract opcodes)
- ✅ Type System (for `.to_c_type()` calls)

## Acceptance Criteria

1. `generate_gimple(Lambda(body=Lambda(body=BinaryOp("+", Var1, Var0))))` generates:
   - Two functions: outer lambda returns inner lambda
   - Inner lambda takes 2 args: y (param), x (captured)
   - Uses correct C types (`int64_t`)
   - No undefined variables
2. `generate_gimple(Iteration(Cons(Literal(1), Nil()), Literal(0)))` → generates a loop that sums an array
3. `generate_gimple(TANNOT(BinaryOp("+", Var0, Var1), TINT))` → type-checked addition with no casts needed
4. All GIMPLE output compiles cleanly with GCC:
   ```bash
   gcc -S -O3 -x c -o out.s generated_gimple.c
   ```
5. CFG is valid: no unreachable blocks, all branches have targets
6. Local variables declared before use
7. Temporary variable naming consistent (`t_1`, `t_2`...)

## Implementation Plan
1. Review existing `GimpleGenerator` — identify gaps
2. Add `hoist_lambda(ast_node)` method that:
   - Recursively collects free variables (via de Bruijn depth analysis)
   - Generates a new function with parameters = bound vars + captured vars
3. Implement `visit_Iteration()` → generates while loop
4. Implement `visit_Quantifier()` → generate reduction over array
5. Add `GVar` mapping: track top-level definitions by order and emit as named functions
6. Ensure all AST nodes have type info → use `.typ.to_c_type()`
7. Write tests for each construct:
   - Simple lambda
   - Nested lambda with capture
   - Iteration over array
   - Conditional with function call
8. Integrate into `examples/demo.py` and verify GCC output

## Blocked By
- ✅ Type Checker (T3) — must provide typed AST
- ❌ Parser (T2) — must emit Var(index=...) correctly

## Next Steps After Completion
→ Implement testing suite, benchmark performance, integrate with GCC backend.
