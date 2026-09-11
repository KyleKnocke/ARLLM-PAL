"""
Example: Demonstrating the PAL/Ø Pipeline

This example shows the flow:
  PAL Source → Tokenization → Parsing → Type Checking → GIMPLE Generation → C Code
"""

from src.common.types import *
from src.tokenizer.vocabulary import get_vocabulary, char_to_token
from src.parser.ast_nodes import *
from src.gimple.builder import GimpleGenerator


def example_1_simple_arithmetic():
    """
    Example 1: Simple arithmetic function
    
    PAL: add(x, y) ≜ x + y
    """
    print("=" * 70)
    print("Example 1: Simple Arithmetic")
    print("=" * 70)
    
    # Manually construct AST (normally parser would do this)
    # add(x, y) ≜ x + y
    add_def = Definition(
        name="add",
        parameters=[
            Parameter("x", INT_TYPE),
            Parameter("y", INT_TYPE),
        ],
        body=BinaryOp("+", Variable("x"), Variable("y")),
        return_type=INT_TYPE,
    )
    
    program = Program(definitions=[add_def])
    
    # Generate GIMPLE
    generator = GimpleGenerator()
    generator.generate(program)
    gimple_code = generator.emit_to_string()
    
    print("\nGenerated GIMPLE IR:")
    print(gimple_code)
    
    # Show AST
    print("\nAST:")
    print(f"  {program}")


def example_2_conditional():
    """
    Example 2: Conditional expression
    
    PAL: abs(x) ≜ ⟨ x > 0 → x ∣ -x ⟩
    """
    print("\n" + "=" * 70)
    print("Example 2: Conditional (Absolute Value)")
    print("=" * 70)
    
    # abs(x) ≜ if (x > 0) then x else -x
    abs_def = Definition(
        name="abs",
        parameters=[Parameter("x", INT_TYPE)],
        body=Conditional(
            condition=BinaryOp(">", Variable("x"), Literal(0)),
            then_branch=Variable("x"),
            else_branch=UnaryOp("-", Variable("x")),
        ),
        return_type=INT_TYPE,
    )
    
    program = Program(definitions=[abs_def])
    
    # Generate GIMPLE
    generator = GimpleGenerator()
    generator.generate(program)
    gimple_code = generator.emit_to_string()
    
    print("\nGenerated GIMPLE IR:")
    print(gimple_code)


def example_3_type_system():
    """
    Example 3: Demonstrating the type system
    """
    print("\n" + "=" * 70)
    print("Example 3: Type System")
    print("=" * 70)
    
    # Show various types
    print("\nType System Examples:")
    print(f"  Integer:           {INT_TYPE} → C: {INT_TYPE.to_c_type()}")
    print(f"  Boolean:           {BOOL_TYPE} → C: {BOOL_TYPE.to_c_type()}")
    print(f"  String:            {STRING_TYPE} → C: {STRING_TYPE.to_c_type()}")
    print(f"  Real:              {REAL_TYPE} → C: {REAL_TYPE.to_c_type()}")
    
    # Function type
    func_type = FunctionT(INT_TYPE, INT_TYPE)
    print(f"  Function (ℤ→ℤ):    {func_type} → C: {func_type.to_c_type()}")
    
    # Product type
    product = ProductT((INT_TYPE, REAL_TYPE))
    print(f"  Product (ℤ×ℝ):     {product} → C: {product.to_c_type()}")
    
    # Array type
    array = ArrayT(INT_TYPE, 10)
    print(f"  Array [10]ℤ:       {array} → C: {array.to_c_type()}")


def example_4_tokenizer():
    """
    Example 4: Tokenizer vocabulary
    """
    print("\n" + "=" * 70)
    print("Example 4: Tokenizer Vocabulary")
    print("=" * 70)
    
    vocab = get_vocabulary()
    
    print(f"\nVocabulary size: {vocab.get_vocabulary_size()} tokens")
    
    print("\nLogical Operators:")
    for symbol in vocab.get_by_category("logical"):
        print(f"  {symbol.char:3s} → {symbol.token_id:4d} ({symbol.name})")
    
    print("\nArithmetic Operators:")
    for symbol in vocab.get_by_category("arithmetic"):
        print(f"  {symbol.char:3s} → {symbol.token_id:4d} ({symbol.name})")
    
    print("\nComparison Operators:")
    for symbol in vocab.get_by_category("comparison"):
        print(f"  {symbol.char:3s} → {symbol.token_id:4d} ({symbol.name})")
    
    # Show token conversion
    print("\nToken Conversion Examples:")
    test_symbols = ["∀", "+", "→", "x", "λ"]
    for sym in test_symbols:
        token_id = char_to_token(sym)
        print(f"  '{sym}' → token {token_id}")


def example_5_bellman_ford_ast():
    """
    Example 5: Simplified Bellman-Ford in AST form
    
    This shows the structure of the algorithm we saw earlier:
    d{v:∞ ∀v∈g, s:0}; loop|V|-1{ ∀u∈g, ∀v,w∈g[u]{ ... } }
    """
    print("\n" + "=" * 70)
    print("Example 5: Algorithm Structure (Simplified)")
    print("=" * 70)
    
    print("\nBellman-Ford Algorithm Represented as AST Nodes:")
    print("""
    The algorithm can be represented as:
    
    Definition(
        name="bellman_ford",
        parameters=[Parameter("graph", ArrayT(...))],
        body=Let(
            bindings=[("d", dict_literal)],
            body=Iteration(
                kind="loop",
                iterations="|V| - 1",
                body=Quantifier(
                    quantifier="∀",
                    parameter=Parameter("u", ...),
                    domain=Variable("graph"),
                    body=Quantifier(
                        quantifier="∀",
                        parameter=Parameter("edge", ...),
                        domain=...,
                        body=Conditional(...)
                    )
                )
            )
        ),
        return_type=...
    )
    """)


def main():
    """Run all examples."""
    example_1_simple_arithmetic()
    example_2_conditional()
    example_3_type_system()
    example_4_tokenizer()
    example_5_bellman_ford_ast()
    
    print("\n" + "=" * 70)
    print("Examples Complete!")
    print("=" * 70)
    print("""
Next Steps:
  1. Implement the PAL Tokenizer (string → token stream)
  2. Implement the PAL Parser (token stream → AST)
  3. Implement the Type Checker (AST → Typed AST)
  4. Expand GIMPLE generator for more complex features
  5. Integrate with GCC backend for compilation
  6. Train the 7B model to output PAL
    """)


if __name__ == "__main__":
    main()
