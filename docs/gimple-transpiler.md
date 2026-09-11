# PAL → GIMPLE Transpiler Architecture

## Overview

The PAL → GIMPLE transpiler is the core compilation component that transforms high-level PAL expressions into GCC's GIMPLE intermediate representation. GIMPLE serves as the gateway to GCC's world-class optimization pipeline, ensuring the generated binaries are as fast as possible.

## Why GIMPLE?

**GIMPLE advantages:**
- High-level enough to preserve semantic intent from PAL
- Low-level enough for precise control over compilation
- Directly feeds into GCC's 60+ years of optimization research
- Access to all GCC backends (x86, ARM, MIPS, RISC-V, etc.)
- Mature, well-tested, battle-hardened

**Alternative considered - RTL:**
- RTL is too low-level; loses semantic information
- Would require duplicating GCC's optimization work
- GIMPLE is the sweet spot for our use case

## GIMPLE Primer

GIMPLE (GNU Intermediate Middle-End Language) is a three-address code representation with the following characteristics:

### Basic Properties

1. **Three-Address Code**: Each statement has at most 3 operands
   ```
   x = y op z     (vs arbitrary expression trees)
   ```

2. **Linear IR**: Control flow explicitly modeled via labels and conditionals
   ```
   label_1:
       x = a + b
       if (x > 0) goto label_2 else goto label_3
   ```

3. **Type Explicit**: All operations carry full type information
   ```
   int x = (int) y + (int) z
   ```

4. **SSA Form**: Each variable assigned once (with φ-nodes at merges)
   ```
   x_1 = a + b
   x_2 = c + d
   x_3 = φ(x_1, x_2)  // merge point
   ```

## GIMPLE Statement Types

### Assignment Statements

```gimple
/* Simple binary operation */
x = y + z

/* Unary operation */
x = -y
x = !y

/* Function call */
x = func(a, b, c)

/* Type cast */
x = (T) y

/* Memory access */
x = *ptr
*ptr = x
x = arr[i]
arr[i] = x
```

### Conditional Statements

```gimple
/* If-then-else */
if (condition) goto label_true else goto label_false

label_true:
    // statements

goto label_merge

label_false:
    // statements

label_merge:
    // continue
```

### Loop Statements

```gimple
/* Simple goto-based loop */
label_loop:
    // body
    if (condition) goto label_loop
    
/* Phony use (keeps variables "live" for optimization) */
__builtin_va_list _args;
```

### Other Statements

```gimple
/* Return */
return x

/* No-op (used in optimization) */
NOP

/* Artificial (compiler-generated, used for analysis) */
// phi nodes, etc.
```

## Data Types in GIMPLE

GIMPLE supports the standard C types plus extensions:

```gimple
/* Primitive Types */
void
char, short, int, long, long long
unsigned char, unsigned short, unsigned int, unsigned long
float, double, long double

/* Pointer Types */
int*
void*
T**

/* Array Types */
int[10]
int[*]  /* VLA (variable-length array) */

/* Structure Types */
struct {int x; float y;} s
typedef struct {...} MyType

/* Function Types */
int (*)(int, int)

/* Vector Types (for SIMD) */
int __attribute__((vector_size(16)))  // 4x int32
float __attribute__((vector_size(32))) // 8x float
```

## Control Flow Graph (CFG) in GIMPLE

GIMPLE code is organized as a CFG:

```
    Entry
      |
  +---+---+
  |       |
  v       v
Block_1 Block_2
  |       |
  +---+---+
      |
      v
   Block_3
      |
      v
    Exit
```

Each block:
- Has a sequence of statements (no branches in middle)
- Ends with a branching statement (if-goto, return, etc.)
- Tracks predecessors and successors

## Building GIMPLE from PAL

### Phase: PAL → GIMPLE Transformation

```
Typed AST (from PAL compiler)
    ↓
[Gimple Builder]
    - Walk AST recursively
    - Lower each PAL construct to GIMPLE statements
    - Generate temporary variables (SSA form)
    - Build CFG
    ↓
GIMPLE representation
    ↓
[Gimple Simplification]
    - Break complex expressions into 3-addr code
    - Normalize conditionals
    - Add type casts where needed
    ↓
[Gimple Verification]
    - Check SSA properties
    - Validate CFG structure
    - Verify type consistency
    ↓
Final GIMPLE
    ↓
[Emit to file or feed to GCC]
```

### Translation Examples

#### Example 1: Binary Operation

**PAL:**
```
λx:ℤ. λy:ℤ. x + y
```

**AST (simplified):**
```
LambdaNode(x:int, 
  LambdaNode(y:int,
    BinaryOpNode(+, VariableNode(x), VariableNode(y))
  )
)
```

**GIMPLE:**
```gimple
int add_func(int x, int y)
{
  int D.1234;
  
  entry:
    D.1234 = x + y;
    return D.1234;
}
```

#### Example 2: Conditional

**PAL:**
```
⟨ x > 0 → y ∣ z ⟩
```

**GIMPLE:**
```gimple
{
  int D.1235;
  
  if (x > 0) goto then_label else goto else_label
  
  then_label:
    D.1235 = y;
    goto merge_label;
    
  else_label:
    D.1235 = z;
    
  merge_label:
    return D.1235;
}
```

#### Example 3: Loop (from Bellman-Ford example)

**PAL:**
```
loop|V|-1 { ... body ... }
```

**GIMPLE:**
```gimple
{
  int i;
  
  i = 0;
  
  loop_header:
    if (i >= |V| - 1) goto loop_exit
    
    // loop body here
    
    i = i + 1;
    goto loop_header;
    
  loop_exit:
    // continue
}
```

#### Example 4: Quantification (∀)

**PAL:**
```
∀u∈g, ∀v,w∈g[u] { ... }
```

**GIMPLE:**
```gimple
{
  struct node *u_ptr;
  struct edge *edge_ptr;
  
  for (u_ptr = g; u_ptr != NULL; u_ptr = u_ptr->next) {
    for (edge_ptr = u_ptr->edges; edge_ptr != NULL; 
         edge_ptr = edge_ptr->next) {
      int v = edge_ptr->dest;
      int w = edge_ptr->weight;
      // loop body
    }
  }
}
```

## Optimization Hints for GCC

To help GCC generate even better code, we can add pragmas and attributes:

```gimple
/* Likely branches (helps branch prediction) */
if (__builtin_expect(condition, 1)) goto likely_label

/* Function attributes */
int func(...) __attribute__((const))     // pure function
int func(...) __attribute__((pure))      // reads globals but no side effects
int func(...) __attribute__((inline))    // request inlining
int func(...) __attribute__((hot))       // function is in hot path
int func(...) __attribute__((cold))      // function is rarely called

/* Loop optimization hints */
#pragma omp parallel for   // parallelize loop
#pragma omp simd           // vectorize loop

/* Variable attributes */
int x __attribute__((aligned(32)))  // align for SIMD
```

## Type System Mapping: PAL → GIMPLE

| PAL Type | GIMPLE Type | Notes |
|----------|------------|-------|
| ⊤ | void* | universal type → opaque pointer |
| ⊥ | (error) | bottom type doesn't exist at runtime |
| 𝔹 | unsigned char (or bool) | 1-byte boolean |
| ℤ | int (or int64_t) | depends on target |
| ℝ | double | IEEE 754 double |
| 𝕾 | const char* | C string |
| □T | struct { T value; } | boxed/lifted type → wrapper struct |
| T₁ → T₂ | T₂(*)(T₁) | function pointer |
| T₁ × T₂ | struct { T₁ f1; T₂ f2; } | product type → struct |
| T₁ ⊔ T₂ | tagged_union_t | union type → tagged union struct |

## Memory Management Strategy

**Stack vs Heap:**
- Local variables: stack-allocated (automatic)
- `alloc` instruction: heap-allocated (malloc wrapper)
- Lifetimes: explicit in GIMPLE (no GC, RAII semantics)

**Example:**
```gimple
int* arr = (int*) __builtin_malloc(n * sizeof(int));
// ...use arr...
__builtin_free(arr);
```

## Compilation to Binary

Once GIMPLE is generated, GCC takes over:

```
GIMPLE
  ↓
[GCC Frontend (GIMPLE verification)]
  ↓
[GCC Middle-end Optimizations]
  - Constant folding
  - Dead code elimination
  - Inlining
  - Loop unrolling
  - Vectorization (auto-SIMD)
  - Alias analysis
  - many more...
  ↓
RTL (Register Transfer Language)
  ↓
[GCC Back-end]
  - Register allocation
  - Instruction selection
  - Scheduling
  ↓
Assembly (x86, ARM, MIPS, etc.)
  ↓
[Assembler & Linker]
  ↓
Final Binary
```

## Emitting GIMPLE

### Strategy 1: Generate C code (simplest)

```python
def emit_as_c_code(gimple_ast):
    """Generate valid C code from GIMPLE"""
    c_code = []
    for func in gimple_ast.functions:
        c_code.append(emit_function(func))
    return "\n".join(c_code)

# Write to file
with open("output.c", "w") as f:
    f.write(emit_as_c_code(gimple))

# Compile via GCC
os.system("gcc -O3 output.c -o output")
```

**Pros:** Simple, portable, maximum optimization (GCC parses from source)  
**Cons:** Slight round-trip overhead

### Strategy 2: Generate GIMPLE dump format (more direct)

GCC's GIMPLE dump format is human-readable:

```gimple
int add_func(int x, int y)
{
  int D.1234;

  <bb 2>:
  D.1234 = x + y;
  return D.1234;
}
```

Can be fed directly back to GCC via `-fgimple` flag (experimental in GCC 9+).

### Strategy 3: Use GCC plugins (most control)

```c
/* GCC plugin that receives GIMPLE directly */
int plugin_init(struct plugin_name_args *args, 
                struct plugin_gcc_version *version) {
    // Register callbacks
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, 
                      setup_pass, NULL);
}
```

Allows direct GIMPLE manipulation before optimization.

## Error Handling in Transpilation

**Type Errors in PAL → GIMPLE:**
- Should not occur (PAL type checker ensures validity)
- Fallback: emit as generic pointer cast, let GCC warn

**Undefined Variables:**
- PAL semantic analyzer catches these
- If somehow missed: emit GIMPLE that GCC will reject

**Control Flow Issues:**
- Unreachable code: GCC removes it
- Infinite loops: tagged with `__attribute__((noreturn))`

## Performance Expectations

**Transpilation Speed:**
- PAL AST → GIMPLE: ~100k nodes/sec
- Typical program: <1ms

**Binary Quality:**
- Code size: within 5-10% of hand-written C
- Runtime speed: 90-95% of hand-written C (with -O3)
- With profile-guided optimization (PGO): 95-99% of hand-written

**Bottleneck:** GCC optimization (not our transpiler)

## Testing Strategy

```
Unit Tests:
  ✓ Each PAL construct → correct GIMPLE
  ✓ Type mappings correct
  ✓ Control flow CFG correct
  
Integration Tests:
  ✓ Generated GIMPLE compiles with GCC
  ✓ Runtime behavior matches PAL semantics
  ✓ Performance within target range
  
Benchmark Suite:
  ✓ Bellman-Ford (graph algorithms)
  ✓ Sorting (array manipulation)
  ✓ Numerical (math operations)
  ✓ Recursive (tree/recursion handling)
  ✓ Parallel (if using pragmas)
```

## Implementation Roadmap

1. **Phase 1: Core Transpiler**
   - Basic expressions (arithmetic, logic, comparisons)
   - Simple functions and variables
   - If-then-else conditionals
   - Output valid C code → GCC

2. **Phase 2: Control Flow**
   - Loops and iteration
   - Quantification (∀, ∃)
   - Recursion and fixpoints
   - Proper CFG generation

3. **Phase 3: Advanced Features**
   - Arrays and records (structs)
   - Pointers and memory management
   - Function pointers
   - Type casts and conversions

4. **Phase 4: Optimization Hints**
   - Likelihood annotations
   - Function attributes (pure, const, hot, cold)
   - Inline hints
   - Loop pragmas (omp simd, etc.)

5. **Phase 5: Direct GIMPLE Emission** (optional)
   - Generate GIMPLE dump format
   - Use GCC plugins for direct manipulation
   - Maximize optimization potential

## Dependencies & Tools

- **Python 3.9+** (transpiler implementation)
- **GCC 10+** (GIMPLE backend, optimizations)
- **libgcc-dev** (if using GCC plugins)
- **graphviz** (for CFG visualization during development)

## Next Steps

1. Finalize PAL type system and ensure complete GIMPLE type mapping
2. Implement core transpiler (expressions → GIMPLE)
3. Test with simple programs (factorial, fibonacci)
4. Expand to real algorithms (Bellman-Ford, sorting)
5. Benchmark and optimize transpilation pipeline
