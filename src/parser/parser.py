"""
Parser for PAL (Programmatic Abstraction Language).

Converts a token stream into an Abstract Syntax Tree (AST) using recursive descent parsing
with operator precedence climbing for binary operators.

Grammar (simplified):
  program         := definition*
  definition      := <identifier> '≜' expression
  expression      := conditional | lambda | quantifier | iteration | term
  lambda          := 'λ' params '->' term
  conditional     := term '?' expression ':' expression
  quantifier      := ('∀'|'∃') identifier '∈' term '=>' expression
  iteration       := term '{' expression '}'
  term            := comparison
  comparison      := arithmetic (('<'|'>'|'='|'∈'|'⊆') arithmetic)*
  arithmetic      := multiply (('+' | '-' | '∪' | '∩') multiply)*
  multiply        := unary (('*' | '÷' | '×' | '∘') unary)*
  unary           := (¬ | - | ∃ | ∀) unary | application
  application     := atom (atom)*
  atom            := identifier | literal | '(' expression ')'
"""

from typing import Optional

from src.tokenizer.tokenizer import Token, Tokenizer
from src.tokenizer.vocabulary import SymbolVocabulary, ReservedToken
from src.parser.ast_nodes import *


class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.pos = 0
        self.current_token = tokens[0] if tokens else None

    def _peek(self, offset: int = 0) -> Optional[Token]:
        """Look ahead without consuming."""
        idx = self.pos + offset
        return self.tokens[idx] if idx < len(self.tokens) else None

    def _advance(self) -> Token:
        """Consume and return current token."""
        tok = self.current_token
        self.pos += 1
        self.current_token = self._peek()
        return tok

    def _expect(self, expected_value: str) -> Token:
        """Consume token with expected value or raise error."""
        if self.current_token and self.current_token.value == expected_value:
            return self._advance()
        raise SyntaxError(
            f"Expected '{expected_value}' at line {self.current_token.line if self.current_token else 'EOF'}"
        )

    def _match(self, *values: str) -> bool:
        """Check if current token matches any of the values."""
        return self.current_token and self.current_token.value in values

    def _source_location(self) -> SourceLocation:
        """Get current source location."""
        if self.current_token:
            return SourceLocation(self.current_token.line, self.current_token.column, "PAL")
        return SourceLocation(0, 0, "PAL")

    def parse(self) -> Program:
        """Parse a complete program."""
        definitions = []
        while self.current_token and self.current_token.id not in (ReservedToken.END.value, ReservedToken.UNKNOWN.value):
            if self._match("<END>"):
                break
            definitions.append(self._parse_definition())
        return Program(definitions)

    def _parse_definition(self) -> Definition:
        """Parse a single definition: name ≜ body."""
        loc = self._source_location()
        name_tok = self._advance()
        name = name_tok.value

        self._expect("≜")
        body = self._parse_expression()

        return Definition(name, body, loc)

    def _parse_expression(self) -> Expression:
        """Parse an expression (top-level)."""
        # Try conditional (ternary)
        expr = self._parse_lambda()

        if self._match("?"):
            self._advance()
            then_expr = self._parse_expression()
            self._expect(":")
            else_expr = self._parse_expression()
            loc = SourceLocation(0, 0, "PAL")
            return Conditional(expr, then_expr, else_expr, loc)

        return expr

    def _parse_lambda(self) -> Expression:
        """Parse lambda abstraction: λ x y z -> body."""
        if self._match("λ"):
            loc = self._source_location()
            self._advance()
            params = []
            while not self._match("->"):
                if self.current_token and self.current_token.id not in (ReservedToken.END.value, ReservedToken.UNKNOWN.value):
                    param_tok = self._advance()
                    params.append(param_tok.value)
            self._expect("->")
            body = self._parse_expression()
            # Build nested lambdas right-to-left
            result = body
            for param in reversed(params):
                result = Lambda(param, result, loc)
            return result

        return self._parse_quantifier()

    def _parse_quantifier(self) -> Expression:
        """Parse quantifiers: ∀ x ∈ set => expr or ∃ x ∈ set => expr."""
        if self._match("∀", "∃"):
            loc = self._source_location()
            op = self._advance().value
            var = self._advance().value
            self._expect("∈")
            domain = self._parse_comparison()
            self._expect("=>")
            body = self._parse_expression()
            return Quantifier(op, var, domain, body, loc)

        return self._parse_iteration()

    def _parse_iteration(self) -> Expression:
        """Parse iteration: expr { accumulator }."""
        expr = self._parse_comparison()

        if self._match("{"):
            loc = self._source_location()
            self._advance()
            accum = self._parse_expression()
            self._expect("}")
            return Iteration(expr, accum, loc)

        return expr

    def _parse_comparison(self) -> Expression:
        """Parse comparison operators: < > = ∈ ⊆."""
        left = self._parse_arithmetic()

        while self._match("<", ">", "=", "∈", "⊆"):
            loc = self._source_location()
            op = self._advance().value
            right = self._parse_arithmetic()
            left = BinaryOp(op, left, right, loc)

        return left

    def _parse_arithmetic(self) -> Expression:
        """Parse addition/subtraction: + - ∪ ∩."""
        left = self._parse_multiplication()

        while self._match("+", "-", "∪", "∩"):
            loc = self._source_location()
            op = self._advance().value
            right = self._parse_multiplication()
            left = BinaryOp(op, left, right, loc)

        return left

    def _parse_multiplication(self) -> Expression:
        """Parse multiplication: * × ÷ ∘."""
        left = self._parse_unary()

        while self._match("*", "×", "÷", "∘"):
            loc = self._source_location()
            op = self._advance().value
            right = self._parse_unary()
            left = BinaryOp(op, left, right, loc)

        return left

    def _parse_unary(self) -> Expression:
        """Parse unary operators: ¬ - ∃ ∀."""
        if self._match("¬", "-"):
            loc = self._source_location()
            op = self._advance().value
            operand = self._parse_unary()
            return UnaryOp(op, operand, loc)

        return self._parse_application()

    def _parse_application(self) -> Expression:
        """Parse function application (juxtaposition)."""
        func = self._parse_atom()

        while self.current_token and self._is_atom_start():
            arg = self._parse_atom()
            loc = SourceLocation(0, 0, "PAL")
            func = Application(func, arg, loc)

        return func

    def _is_atom_start(self) -> bool:
        """Check if current token starts an atom."""
        if not self.current_token:
            return False
        return self._match("(") or (self.current_token.id > ReservedToken.UNKNOWN.value and
                                     self.current_token.id < 256)  # approximation

    def _parse_atom(self) -> Expression:
        """Parse atomic expressions: literals, variables, parenthesized expressions."""
        loc = self._source_location()

        # Parenthesized expression
        if self._match("("):
            self._advance()
            expr = self._parse_expression()
            self._expect(")")
            return expr

        # Literal or variable
        if self.current_token:
            tok = self._advance()
            value = tok.value

            # Try parsing as literal
            try:
                if "." in value:
                    return Literal(float(value), PrimitiveT("ℝ"), loc)
                else:
                    return Literal(int(value), PrimitiveT("ℤ"), loc)
            except ValueError:
                # It's a variable
                return Variable(value, loc)

        raise SyntaxError("Unexpected end of input while parsing atom")


def parse(input_string: str) -> Program:
    """Convenience function: tokenize and parse input string."""
    tokenizer = Tokenizer()
    tokens = tokenizer.tokenize(input_string)
    parser = Parser(tokens)
    return parser.parse()
