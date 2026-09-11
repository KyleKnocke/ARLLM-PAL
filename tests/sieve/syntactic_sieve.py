#!/usr/bin/env python3
"""
Syntactic Sieve: Validates that programs follow fixed-arity grammar rules.

No parentheses needed — parser must infer structure from arity.
Example: λ.λ.+ ₀ ₁  should parse as: (λ (λ (+ (Var 0) (Var 1))))
"""

import sys

# We'll define a minimal AST node
class Node:
    def __init__(self, opcode, children=None):
        self.opcode = opcode
        self.children = children or []

def arity_of(opcode):
    # Map from Opcode enum to arity — mirrored from C++ impl
    ARITY_MAP = {
        "True": 0, "False": 0, "Nil": 0, "Var": 0, "GVar": 0, "Num": 0,
        "Neg": 1, "Not": 1, "Fix": 1, "Ret": 1, "Box": 1, "Deref": 1, "Lambda": 1,
        "Add": 2, "Sub": 2, "Mul": 2, "Div": 2,
        "And": 2, "Or": 2,
        "Eq": 2, "Neq": 2, "Lt": 2, "Le": 2, "Gt": 2, "Ge": 2,
        "Mem": 2, "NMem": 2, "SubsetEq": 2, "Subset": 2, "Union": 2, "Intersect": 2, "SetMinus": 2,
        "Apply": 2, "Compose": 2, "Let": 2, "Forall": 2, "Exists": 2, "Idx": 2, "Cons": 2, "TAnnot": 2,
        "Cond": 3, "Iter": 3,
    }
    return ARITY_MAP.get(opcode, -1)

def parse_program(tokens):
    """Simple recursive descent parser based on fixed arity"""
    if not tokens:
        return None
    
    token = tokens.pop(0)
    op = token['opcode']
    arity = arity_of(op)
    
    node = Node(op)
    for _ in range(arity):
        if not tokens:
            raise ValueError(f"Expected {arity} args for {op}, got none")
        child = parse_program(tokens)
        if child is None:
            raise ValueError(f"Incomplete program — missing arg after {op}")
        node.children.append(child)
    
    return node

def test_syntactic_sieve():
    print("🧩 Syntactic Sieve: Testing arity-based parsing...")
    
    # Test case: prime sieve logic in PAL/Ø
    # Simplified example: 
    # Let f = λ.n.∀i.¬(i ∈ [2..n-1] ∧ n % i == 0)
    # We'll simulate token stream as list of {'opcode': ..., 'value': ...}
    
    test_tokens = [
        {"opcode": "Let", "value": None},
        {"opcode": "GVar", "value": 0},  # f
        {"opcode": "Lambda", "value": None},
        {"opcode": "Var", "value": 0},   # n (de Bruijn index 0)
        {"opcode": "Forall", "value": None},
        {"opcode": "Var", "value": 1},   # i
        {"opcode": "Not", "value": None},
        {"opcode": "And", "value": None},
        {"opcode": "Mem", "value": None},
          {"opcode": "Var", "value": 1},    # i
          {"opcode": "Range", "value": [2, 5]}, # Placeholder: range will be implemented later
        {"opcode": "Eq", "value": None},
          {"opcode": "Mod", "value": None},
            {"opcode": "Var", "value": 0},    # n
            {"opcode": "Var", "value": 1},    # i
          {"opcode": "Num", "value": 0},      # 0
    ]
    
    try:
        ast = parse_program(test_tokens)
        if ast and ast.opcode == "Let":
            print("✅ Syntactic Sieve: Valid AST structure")
            return True
        else:
            raise ValueError("AST not built correctly")
    except Exception as e:
        print(f"❌ Syntactic Sieve failed: {e}")
        return False

if __name__ == "__main__":
    if not test_syntactic_sieve():
        sys.exit(1)
    print("✨ Syntactic Sieve complete.")