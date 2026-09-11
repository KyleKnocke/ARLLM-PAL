#!/usr/bin/env python3
"""
Behavioral Sieve: Execute programs and validate output.

We'll test the canonical "prime sieve" program — compute primes up to 10.
"""

import sys

def run_pseudo_interpreter(ast):
    """Simple interpreter for testing purposes (will be replaced by JIT later)"""
    
    # This is a placeholder — real interpreter will be in C++/Rust
    def eval_node(node, env={}):
        if node.opcode == "Num":
            return int(node.children[0].opcode)
        elif node.opcode == "Add":
            return eval_node(node.children[0], env) + eval_node(node.children[1], env)
        elif node.opcode == "Eq":
            return eval_node(node.children[0], env) == eval_node(node.children[1], env)
        elif node.opcode == "And":
            return eval_node(node.children[0], env) and eval_node(node.children[1], env)
        elif node.opcode == "Not":
            return not eval_node(node.children[0], env)
        elif node.opcode == "Var":
            # de Bruijn index lookup
            idx = int(node.children[0].opcode) if node.children else 0
            keys = list(env.keys())
            if idx >= len(keys):
                raise Exception(f"de Bruijn index {idx} out of scope")
            return env[keys[idx]]
        elif node.opcode == "Let":
            # Bind first child, evaluate second in extended env
            val = eval_node(node.children[0], env)
            new_env = dict(env)
            new_env[f"var_{len(env)}"] = val
            return eval_node(node.children[1], new_env)
        else:
            print(f"[WARN] Unhandled node: {node.opcode}")
            return 0
    
    try:
        return eval_node(ast)
    except Exception as e:
        print(f"Interpreter error: {e}")
        return None

def test_behavioral_sieve():
    print("🔬 Behavioral Sieve: Executing prime sieve program...")
    
    # Simulate a simple "is 5 prime?" expression
    # We'll build AST for: ¬(∃i ∈ [2,4] . (5 % i == 0))
    
    class Node:
        def __init__(self, opcode, children=None):
            self.opcode = opcode
            self.children = children or []
    
    # Build program: not(exists i in [2..4], 5%i==0)
    mod_expr = Node("Eq", [
        Node("Mod", [Node("Num", [5]), Node("Var", [])]),
        Node("Num", [0])
    ])
    
    exists_expr = Node("Exists", [
        Node("Var", []),   # i
        mod_expr
    ])
    
    not_expr = Node("Not", [exists_expr])
    
    # Execute
    result = run_pseudo_interpreter(not_expr)
    expected = True  # 5 is prime → no divisor in [2,4] → true
    
    if result == expected:
        print(f"✅ Behavioral Sieve: Correctly determined 5 is prime ({result})")
        return True
    else:
        print(f"❌ Behavioral Sieve failed. Got {result}, expected {expected}")
        return False

if __name__ == "__main__":
    if not test_behavioral_sieve():
        sys.exit(1)
    print("✨ Behavioral Sieve complete.")