"""
Tokenizer for PAL (Programmatic Abstraction Language).

Converts a UTF-8 input string into a sequence of token IDs using the symbol vocabulary.
Handles:
- Unicode symbol recognition and mapping to token IDs
- Identifier/keyword tokenization
- Numeric literals
- Whitespace and comment skipping
- Error reporting with line/column tracking
"""

import re
from dataclasses import dataclass
from typing import Optional

from src.common.types import Type
from src.tokenizer.vocabulary import SymbolVocabulary, ReservedToken


@dataclass
class Token:
    """A single token with metadata."""
    id: int
    value: str
    line: int
    column: int


class Tokenizer:
    def __init__(self, vocab: Optional[SymbolVocabulary] = None):
        self.vocab = vocab or SymbolVocabulary()
        self.input = ""
        self.pos = 0
        self.line = 1
        self.column = 1
        self.tokens: list[Token] = []

    def _peek(self, offset: int = 0) -> Optional[str]:
        """Look ahead without consuming."""
        idx = self.pos + offset
        return self.input[idx] if idx < len(self.input) else None

    def _advance(self) -> Optional[str]:
        """Consume and return next character."""
        if self.pos >= len(self.input):
            return None
        ch = self.input[self.pos]
        self.pos += 1
        if ch == "\n":
            self.line += 1
            self.column = 1
        else:
            self.column += 1
        return ch

    def _skip_whitespace(self) -> None:
        """Skip spaces, tabs, newlines."""
        while self._peek() and self._peek() in " \t\n\r":
            self._advance()

    def _skip_comment(self) -> None:
        """Skip line comment starting with #."""
        if self._peek() == "#":
            while self._peek() and self._peek() != "\n":
                self._advance()

    def _read_identifier(self) -> str:
        """Read a keyword or identifier (alphanumeric + underscore)."""
        start = self.pos
        while self._peek() and (self._peek().isalnum() or self._peek() == "_"):
            self._advance()
        return self.input[start : self.pos]

    def _read_number(self) -> str:
        """Read a numeric literal (int or float)."""
        start = self.pos
        while self._peek() and (self._peek().isdigit() or self._peek() == "."):
            self._advance()
        return self.input[start : self.pos]

    def _read_string(self, quote: str) -> str:
        """Read a string literal enclosed in quotes."""
        result = ""
        self._advance()  # consume opening quote
        while self._peek() and self._peek() != quote:
            if self._peek() == "\\":
                self._advance()
                escaped = self._peek()
                if escaped == "n":
                    result += "\n"
                elif escaped == "t":
                    result += "\t"
                elif escaped == "\\":
                    result += "\\"
                elif escaped == quote:
                    result += quote
                else:
                    result += escaped or ""
                self._advance()
            else:
                result += self._advance() or ""
        if self._peek() == quote:
            self._advance()  # consume closing quote
        return result

    def tokenize(self, input_string: str) -> list[Token]:
        """Convert input string to token stream."""
        self.input = input_string
        self.pos = 0
        self.line = 1
        self.column = 1
        self.tokens = []

        # Add START token
        self.tokens.append(Token(ReservedToken.START.value, "<START>", self.line, self.column))

        while self.pos < len(self.input):
            self._skip_whitespace()
            if self.pos >= len(self.input):
                break

            # Skip comments
            if self._peek() == "#":
                self._skip_comment()
                continue

            start_line = self.line
            start_col = self.column

            # Try to match Unicode symbol
            symbol = self.vocab.get_symbol(self._peek() or "")
            if symbol:
                self.tokens.append(Token(symbol.token_id, symbol.char, start_line, start_col))
                self._advance()
                continue

            # Try multi-char symbols (for operators like ->, |->)
            lookahead = (self._peek() or "") + (self._peek(1) or "")
            multi_symbol = None
            for s in self.vocab.symbols:
                if lookahead.startswith(s.char) and len(s.char) > 1:
                    multi_symbol = s
                    break

            if multi_symbol:
                for _ in range(len(multi_symbol.char)):
                    self._advance()
                self.tokens.append(Token(multi_symbol.token_id, multi_symbol.char, start_line, start_col))
                continue

            # Try identifier/keyword
            if self._peek() and (self._peek().isalpha() or self._peek() == "_"):
                ident = self._read_identifier()
                # Map known keywords to token IDs, otherwise treat as variable name
                tok_id = self.vocab.get_token_id(ident) or ReservedToken.UNKNOWN.value
                self.tokens.append(Token(tok_id, ident, start_line, start_col))
                continue

            # Try number
            if self._peek() and self._peek().isdigit():
                num = self._read_number()
                self.tokens.append(Token(ReservedToken.UNKNOWN.value, num, start_line, start_col))
                continue

            # Try string literal
            if self._peek() in ('"', "'"):
                quote = self._peek() or '"'
                string_val = self._read_string(quote)
                self.tokens.append(Token(ReservedToken.UNKNOWN.value, f'"{string_val}"', start_line, start_col))
                continue

            # Unknown character - skip
            self._advance()

        # Add END token
        self.tokens.append(Token(ReservedToken.END.value, "<END>", self.line, self.column))

        return self.tokens

    def get_token_ids(self, input_string: str) -> list[int]:
        """Convenience method: return just the token IDs."""
        return [tok.id for tok in self.tokenize(input_string)]
