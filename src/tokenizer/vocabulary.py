"""
PAL Tokenizer Vocabulary

Defines all valid Unicode symbols used in PAL and their token IDs.
Token ID ranges:
  - 0-255: Reserved tokens (padding, start, end, markers, etc.)
  - 256+: Symbol tokens (actual PAL symbols)
"""

from dataclasses import dataclass
from typing import Dict, Optional, List, Set
from enum import IntEnum


class ReservedToken(IntEnum):
    """Reserved token IDs for special purposes."""
    PAD = 0
    START = 1
    END = 2
    UNKNOWN = 3
    SCOPE_OPEN = 4
    SCOPE_CLOSE = 5
    TYPE_CHANGE = 6
    ERROR = 7
    # 8-255 available for future reserved tokens


@dataclass(frozen=True)
class Symbol:
    """Represents a PAL symbol and its token ID."""
    char: str                  # Unicode character(s)
    token_id: int             # Unique token ID
    name: str                 # Human-readable name
    category: str             # Category (logical, math, structural, etc.)
    precedence: int = 0       # Operator precedence (0 = lowest)
    associativity: str = "none"  # "left", "right", or "none"


class SymbolVocabulary:
    """Manages the complete PAL symbol vocabulary."""
    
    def __init__(self):
        self.symbols: Dict[str, Symbol] = {}
        self.token_to_symbol: Dict[int, Symbol] = {}
        self._initialize_vocabulary()
    
    def _initialize_vocabulary(self) -> None:
        """Initialize the standard PAL symbol vocabulary."""
        
        # Mathematical/Logical Operators
        self._register_symbol("∀", "FORALL", "logical", 1)           # Universal quantifier
        self._register_symbol("∃", "EXISTS", "logical", 1)           # Existential quantifier
        self._register_symbol("¬", "NOT", "logical", 8)              # Negation
        self._register_symbol("∧", "AND", "logical", 4, "left")      # Conjunction
        self._register_symbol("∨", "OR", "logical", 3, "left")       # Disjunction
        self._register_symbol("→", "IMPLIES", "logical", 2, "right") # Implication
        self._register_symbol("↔", "IFF", "logical", 2)              # Biconditional
        
        # Set Operations
        self._register_symbol("∈", "ELEM", "set", 6)                 # Membership
        self._register_symbol("∉", "NOT_ELEM", "set", 6)             # Non-membership
        self._register_symbol("⊆", "SUBSET", "set", 5)               # Subset
        self._register_symbol("⊃", "SUPERSET", "set", 5)             # Superset
        self._register_symbol("∪", "UNION", "set", 4, "left")        # Union
        self._register_symbol("∩", "INTERSECTION", "set", 4, "left") # Intersection
        self._register_symbol("\\", "SET_DIFF", "set", 4)            # Set difference
        
        # Type Operations
        self._register_symbol("⊤", "TOP_TYPE", "type", 0)            # Top type
        self._register_symbol("⊥", "BOTTOM_TYPE", "type", 0)         # Bottom type
        self._register_symbol("𝔹", "BOOL_TYPE", "type", 0)           # Boolean type
        self._register_symbol("ℤ", "INT_TYPE", "type", 0)            # Integer type
        self._register_symbol("ℝ", "REAL_TYPE", "type", 0)           # Real type
        self._register_symbol("𝕾", "STRING_TYPE", "type", 0)         # String type
        self._register_symbol("□", "BOX", "type", 9)                 # Boxed/lifted type
        
        # Comparison Operators
        self._register_symbol("=", "EQ", "comparison", 5)            # Equality
        self._register_symbol("≠", "NEQ", "comparison", 5)            # Inequality
        self._register_symbol("<", "LT", "comparison", 5)            # Less than
        self._register_symbol("≤", "LE", "comparison", 5)            # Less than or equal
        self._register_symbol(">", "GT", "comparison", 5)            # Greater than
        self._register_symbol("≥", "GE", "comparison", 5)            # Greater than or equal
        self._register_symbol("≡", "EQUIV", "comparison", 5)         # Structural equivalence
        
        # Arithmetic Operators
        self._register_symbol("+", "ADD", "arithmetic", 6, "left")   # Addition
        self._register_symbol("-", "SUB", "arithmetic", 6, "left")   # Subtraction
        self._register_symbol("×", "MUL", "arithmetic", 7, "left")   # Multiplication
        self._register_symbol("÷", "DIV", "arithmetic", 7, "left")   # Division
        self._register_symbol("mod", "MOD", "arithmetic", 7)         # Modulo
        self._register_symbol("⊕", "XOR", "arithmetic", 4, "left")   # XOR / Generic op
        self._register_symbol("⊗", "TENSOR", "arithmetic", 7, "left")  # Tensor product
        self._register_symbol("⊙", "HADAMARD", "arithmetic", 7, "left") # Hadamard product
        
        # Structural Symbols
        self._register_symbol("⟨", "ANGLE_OPEN", "structural", 0)    # Angle bracket open
        self._register_symbol("⟩", "ANGLE_CLOSE", "structural", 0)   # Angle bracket close
        self._register_symbol("⟦", "BRACKET_OPEN", "structural", 0)  # Double bracket open
        self._register_symbol("⟧", "BRACKET_CLOSE", "structural", 0) # Double bracket close
        self._register_symbol("⦅", "SCOPE_OPEN", "structural", 0)    # Scope open
        self._register_symbol("⦆", "SCOPE_CLOSE", "structural", 0)   # Scope close
        self._register_symbol("⟪", "DBL_PAREN_OPEN", "structural", 0)  # Double paren open
        self._register_symbol("⟫", "DBL_PAREN_CLOSE", "structural", 0) # Double paren close
        self._register_symbol("{", "BRACE_OPEN", "structural", 0)    # Set/dict open
        self._register_symbol("}", "BRACE_CLOSE", "structural", 0)   # Set/dict close
        self._register_symbol("[", "BRACKET_SQ_OPEN", "structural", 0) # Square bracket open
        self._register_symbol("]", "BRACKET_SQ_CLOSE", "structural", 0) # Square bracket close
        
        # Binding & Definition
        self._register_symbol("λ", "LAMBDA", "binding", 1)           # Lambda abstraction
        self._register_symbol("μ", "FIXPOINT", "binding", 1)         # Fixpoint (recursion)
        self._register_symbol("≜", "DEFINE", "binding", 0)           # Definition
        self._register_symbol(":", "COLON", "binding", 0)            # Type annotation
        self._register_symbol("∣", "PIPE", "structural", 0)          # Alternative/pipe
        self._register_symbol(",", "COMMA", "structural", 0)         # Comma separator
        self._register_symbol(";", "SEMICOLON", "structural", 0)     # Semicolon
        self._register_symbol(".", "DOT", "structural", 0)           # Dot/composition
        self._register_symbol("·", "CDOT", "structural", 0)          # Composition operator
        
        # Control Flow
        self._register_symbol("?", "COND", "control", 2)             # Conditional
        self._register_symbol("↩", "RETURN", "control", 0)           # Return
        self._register_symbol("loop", "LOOP", "control", 0)          # Loop
        self._register_symbol("break", "BREAK", "control", 0)        # Break
        self._register_symbol("continue", "CONTINUE", "control", 0)  # Continue
        
        # Special
                self._register_symbol("_", "WILDCARD", "special", 0)         # Wildcard
                self._register_symbol("@", "AT", "special", 0)               # Symbol reference
                self._register_symbol("|", "CARDINALITY", "special", 9)      # Cardinality/absolute value

                # === DE BRUIJN AND GVAR INDICES (MISSING IN INITIAL IMPLEMENTATION) ===
                # De Bruijn indices: subscript digits ₀–₉ → VAR0–VAR9
                for i in range(10):
                    char = chr(0x2080 + i)   # U+2080 to U+2089: subscript 0-9
                    name = f"VAR{i}"
                    self._register_symbol(char, name, "special", 0)

                # Global variable references: superscript digits ⁰–⁹ → GVAR0–GVAR9
                for i in range(10):
                    char = chr(0x2070 + i)   # U+2070 to U+2079: superscript 0-9
                    name = f"GVAR{i}"
                    self._register_symbol(char, name, "special", 0)

                # Escape glyphs for indices >=10 (inline ASCII digit runs)
                self._register_symbol("ᵥ", "VAR_ESC", "special", 0)
                self._register_symbol("ᴳ", "GVAR_ESC", "special", 0)
    
            def _register_symbol(self, char: str, name: str, category: str, 
                        precedence: int = 0, associativity: str = "none") -> None:
        """Register a symbol in the vocabulary."""
        token_id = 256 + len(self.symbols)
        symbol = Symbol(char, token_id, name, category, precedence, associativity)
        self.symbols[char] = symbol
        self.token_to_symbol[token_id] = symbol
    
    def get_token_id(self, symbol: str) -> int:
        """Get token ID for a symbol, or UNKNOWN if not found."""
        if symbol in self.symbols:
            return self.symbols[symbol].token_id
        return ReservedToken.UNKNOWN
    
    def get_symbol(self, token_id: int) -> Optional[Symbol]:
        """Get symbol for a token ID."""
        return self.token_to_symbol.get(token_id)
    
    def is_valid_token(self, token_id: int) -> bool:
        """Check if a token ID is valid (not UNKNOWN)."""
        return token_id != ReservedToken.UNKNOWN
    
    def get_by_name(self, name: str) -> Optional[Symbol]:
        """Look up symbol by name."""
        for symbol in self.symbols.values():
            if symbol.name == name:
                return symbol
        return None
    
    def get_by_category(self, category: str) -> List[Symbol]:
        """Get all symbols in a category."""
        return [s for s in self.symbols.values() if s.category == category]
    
    def get_operators(self) -> Dict[str, Symbol]:
        """Get all operator symbols sorted by precedence."""
        ops = {s.char: s for s in self.symbols.values() 
               if s.category in ["logical", "arithmetic", "comparison", "set"]}
        return dict(sorted(ops.items(), key=lambda x: x[1].precedence, reverse=True))
    
    def get_vocabulary_size(self) -> int:
        """Get total vocabulary size (reserved + symbols)."""
        return 256 + len(self.symbols)


# Global vocabulary instance
_vocabulary: Optional[SymbolVocabulary] = None


def get_vocabulary() -> SymbolVocabulary:
    """Get or create the global symbol vocabulary."""
    global _vocabulary
    if _vocabulary is None:
        _vocabulary = SymbolVocabulary()
    return _vocabulary


def token_to_char(token_id: int) -> str:
    """Convert token ID to character."""
    vocab = get_vocabulary()
    if token_id < 256:
        return f"[RESERVED_{token_id}]"
    symbol = vocab.get_symbol(token_id)
    return symbol.char if symbol else "[UNKNOWN]"


def char_to_token(char: str) -> int:
    """Convert character to token ID."""
    vocab = get_vocabulary()
    return vocab.get_token_id(char)
