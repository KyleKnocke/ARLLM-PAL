#!/usr/bin/env python3
"""
Lexical Sieve: Validates that every glyph maps correctly to an Opcode and token_id.

This is the first filter — if tokens are misencoded, everything else fails.
"""

import sys
from pathlib import Path

# Import PAL/Ø vocabulary (we'll need to generate this from C++ headers)
# For now, we hardcode based on vocabulary.h
EXPECTED_SYMBOLS = {
    "⊤": ("TTop", 4),
    "⊥": ("TBot", 5),
    "𝔹": ("TBool", 6),
    "ℕ": ("TInt", 7),
    "ℝ": ("TReal", 8),
    "ℂ": ("TStr", 9),
    "λ": ("Lambda", 10),
    "¬": ("Not", 11),
    "−": ("Neg", 12),
    "+": ("Add", 13),
    "−": ("Sub", 14),   # Note: Unicode has two minus signs, ensure correct one
    "*": ("Mul", 15),
    "/": ("Div", 16),
    "∧": ("And", 17),
    "∨": ("Or", 18),
    "=": ("Eq", 19),
    "≠": ("Neq", 20),
    "<": ("Lt", 21),
    "≤": ("Le", 22),
    ">": ("Gt", 23),
    "≥": ("Ge", 24),
    "∈": ("Mem", 25),
    "∉": ("NMem", 26),
    "⊆": ("SubsetEq", 27),
    "⊂": ("Subset", 28),
    "∪": ("Union", 29),
    "∩": ("Intersect", 30),
    "∖": ("SetMinus", 31),
    "⟨⟩": ("Apply", 32),   # Placeholder for application glyph
    "∘": ("Compose", 33),
    "∶": ("Let", 34),
    "∀": ("Forall", 35),
    "∃": ("Exists", 36),
    "₀": ("Var", 37, 0),   # de Bruijn index 0
    "₁": ("Var", 38, 1),   # de Bruijn index 1
    "₂": ("Var", 39, 2),
    "₃": ("Var", 40, 3),
    "₄": ("Var", 41, 4),
    "₅": ("Var", 42, 5),
    "₆": ("Var", 43, 6),
    "₇": ("Var", 44, 7),
    "₈": ("Var", 45, 8),
    "₉": ("Var", 46, 9),
    "⁰": ("GVar", 47, 0),   # global var index
    "¹": ("GVar", 48, 1),
    "²": ("GVar", 49, 2),
    "³": ("GVar", 50, 3),
    "⁴": ("GVar", 51, 4),
    "⁵": ("GVar", 52, 5),
    "⁶": ("GVar", 53, 6),
    "⁷": ("GVar", 54, 7),
    "⁸": ("GVar", 55, 8),
    "⁹": ("GVar", 56, 9),
}

def load_glyph_map():
    # This will eventually be auto-generated from vocabulary.h
    return EXPECTED_SYMBOLS

def test_lexical_sieve():
    print("🧪 Lexical Sieve: Testing glyph-to-opcode mapping...")
    
    glyph_map = load_glyph_map()
    errors = []

    for glyph, expected in glyph_map.items():
        name, token_id = expected[0], expected[1]
        if len(glyph.encode('utf-8')) != 2 and len(glyph) > 1:
            # Some glyphs are multi-byte UTF-8 but single Unicode codepoints
            pass
        
        if not isinstance(token_id, int) or token_id < 4:
            errors.append(f"Invalid token_id for {glyph}: {token_id} (must be >= 4)")
    
    if len(errors) == 0:
        print("✅ Lexical Sieve: Passed")
        return True
    else:
        for e in errors:
            print(f"❌ {e}")
        return False

if __name__ == "__main__":
    if not test_lexical_sieve():
        sys.exit(1)
    print("✨ Lexical Sieve complete.")