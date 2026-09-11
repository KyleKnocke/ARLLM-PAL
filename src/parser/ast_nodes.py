"""
AST Node Definitions

Defines all Abstract Syntax Tree node types for PAL.
These nodes are produced by the parser and consumed by the type checker and code generator.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import List, Optional, Dict, Tuple, Any
from src.common.types import Type, INT_TYPE, BOOL_TYPE


@dataclass
class SourceLocation:
    """Source code location for error reporting."""
    line: int
    column: int
    file: str = "unknown"
    
    def __str__(self) -> str:
        return f"{self.file}:{self.line}:{self.column}"


class ASTNode(ABC):
    """Base class for all AST nodes."""
    
    @abstractmethod
    def __repr__(self) -> str:
        pass


# ============================================================================
# Expressions
# ============================================================================

@dataclass
class Literal(ASTNode):
    """Literal value (number, boolean, string)."""
    value: Any
    type_hint: Optional[Type] = None
    
    def __repr__(self) -> str:
        return f"Literal({self.value})"


@dataclass
class Variable(ASTNode):
    """Variable reference."""
    name: str
    
    def __repr__(self) -> str:
        return f"Variable({self.name})"


@dataclass
class BinaryOp(ASTNode):
    """Binary operation."""
    operator: str           # Symbol like "+" or "∧"
    left: ASTNode
    right: ASTNode
    
    def __repr__(self) -> str:
        return f"BinaryOp({self.operator}, {self.left}, {self.right})"


@dataclass
class UnaryOp(ASTNode):
    """Unary operation."""
    operator: str           # Symbol like "¬" or "-"
    operand: ASTNode
    
    def __repr__(self) -> str:
        return f"UnaryOp({self.operator}, {self.operand})"


@dataclass
class Application(ASTNode):
    """Function application: f(x) or f(x,y,...)."""
    func: ASTNode
    args: List[ASTNode]
    
    def __repr__(self) -> str:
        args_str = ", ".join(repr(a) for a in self.args)
        return f"Application({self.func}, [{args_str}])"


@dataclass
class Lambda(ASTNode):
    """Lambda abstraction: λx:T. e or λT. e."""
    parameters: List['Parameter']
    body: ASTNode
    
    def __repr__(self) -> str:
        params_str = ", ".join(repr(p) for p in self.parameters)
        return f"Lambda([{params_str}], {self.body})"


@dataclass
class Parameter:
    """Function parameter with optional type annotation."""
    name: str
    type_annotation: Optional[Type] = None
    
    def __repr__(self) -> str:
        if self.type_annotation:
            return f"Parameter({self.name}:{self.type_annotation})"
        return f"Parameter({self.name})"


@dataclass
class Let(ASTNode):
    """Let binding: ⦅ x ≜ e1; e2 ⦆."""
    bindings: List[Tuple[str, ASTNode]]  # [(name, value), ...]
    body: ASTNode
    
    def __repr__(self) -> str:
        bindings_str = "; ".join(f"{n}={repr(v)}" for n, v in self.bindings)
        return f"Let([{bindings_str}], {self.body})"


@dataclass
class Conditional(ASTNode):
    """Conditional expression: ⟨ φ → e1 ∣ e2 ⟩."""
    condition: ASTNode
    then_branch: ASTNode
    else_branch: ASTNode
    
    def __repr__(self) -> str:
        return f"Conditional({self.condition}, {self.then_branch}, {self.else_branch})"


@dataclass
class Quantifier(ASTNode):
    """Quantified expression: ∀x∈S. φ or ∃x∈S. φ."""
    quantifier: str         # "∀" or "∃"
    parameter: Parameter
    domain: Optional[ASTNode]  # The set S (None = implicit universal domain)
    body: ASTNode
    
    def __repr__(self) -> str:
        domain_str = repr(self.domain) if self.domain else "all"
        return f"Quantifier({self.quantifier}, {self.parameter}, {domain_str}, {self.body})"


@dataclass
class Iteration(ASTNode):
    """Iteration/map: ⟦ x∈S ∶ e(x) ⟧ or loop N { body }."""
    kind: str               # "map", "loop", "for_each"
    parameter: Optional[Parameter] = None
    domain: Optional[ASTNode] = None
    iterations: Optional[int] = None  # For "loop N"
    body: Optional[ASTNode] = None
    
    def __repr__(self) -> str:
        return f"Iteration({self.kind}, param={self.parameter}, domain={self.domain}, body={self.body})"


@dataclass
class Fixpoint(ASTNode):
    """Fixpoint/recursion: μf. e."""
    variable: str
    body: ASTNode
    
    def __repr__(self) -> str:
        return f"Fixpoint({self.variable}, {self.body})"


@dataclass
class SetLiteral(ASTNode):
    """Set literal: {a, b, c} or {x | φ(x)}."""
    elements: Optional[List[ASTNode]] = None  # For {a,b,c}
    variable: Optional[str] = None            # For {x | φ}
    domain: Optional[ASTNode] = None          # Domain of x
    predicate: Optional[ASTNode] = None       # Predicate φ
    
    def __repr__(self) -> str:
        if self.elements:
            elems_str = ", ".join(repr(e) for e in self.elements)
            return f"SetLiteral({{{elems_str}}})"
        else:
            return f"SetLiteral({{x ∈ {self.domain} | {self.predicate}}})"


@dataclass
class Composition(ASTNode):
    """Function composition: f · g."""
    functions: List[ASTNode]
    
    def __repr__(self) -> str:
        funcs_str = " · ".join(repr(f) for f in self.functions)
        return f"Composition({funcs_str})"


@dataclass
class TypeAnnotation(ASTNode):
    """Type annotation: e : T."""
    expression: ASTNode
    type_: Type
    
    def __repr__(self) -> str:
        return f"TypeAnnotation({self.expression}, {self.type_})"


@dataclass
class Return(ASTNode):
    """Return statement: ↩ e."""
    value: Optional[ASTNode] = None
    
    def __repr__(self) -> str:
        return f"Return({self.value})"


# ============================================================================
# Statements and Definitions
# ============================================================================

@dataclass
class Definition(ASTNode):
    """Top-level definition: f ≜ e."""
    name: str
    parameters: List[Parameter]
    body: ASTNode
    return_type: Optional[Type] = None
    
    def __repr__(self) -> str:
        params_str = ", ".join(repr(p) for p in self.parameters)
        return f"Definition({self.name}([{params_str}]), {self.body})"


@dataclass
class Program(ASTNode):
    """Complete program: collection of definitions."""
    definitions: List[Definition]
    
    def __repr__(self) -> str:
        defs_str = ", ".join(repr(d) for d in self.definitions)
        return f"Program([{defs_str}])"


# ============================================================================
# Type Annotations in AST
# ============================================================================

@dataclass
class TypedNode:
    """Wrapper that attaches type information to an AST node."""
    node: ASTNode
    type_: Optional[Type] = None
    
    def __repr__(self) -> str:
        if self.type_:
            return f"TypedNode({self.node}, type={self.type_})"
        return f"TypedNode({self.node})"


# ============================================================================
# Helper functions
# ============================================================================

def find_free_variables(node: ASTNode) -> set:
    """Find all free (unbound) variables in an expression."""
    free_vars = set()
    
    if isinstance(node, Variable):
        free_vars.add(node.name)
    elif isinstance(node, BinaryOp):
        free_vars.update(find_free_variables(node.left))
        free_vars.update(find_free_variables(node.right))
    elif isinstance(node, UnaryOp):
        free_vars.update(find_free_variables(node.operand))
    elif isinstance(node, Application):
        free_vars.update(find_free_variables(node.func))
        for arg in node.args:
            free_vars.update(find_free_variables(arg))
    elif isinstance(node, Lambda):
        bound_vars = {p.name for p in node.parameters}
        body_vars = find_free_variables(node.body)
        free_vars.update(body_vars - bound_vars)
    elif isinstance(node, Let):
        for name, value in node.bindings:
            free_vars.update(find_free_variables(value))
        body_vars = find_free_variables(node.body)
        bound_vars = {name for name, _ in node.bindings}
        free_vars.update(body_vars - bound_vars)
    elif isinstance(node, Conditional):
        free_vars.update(find_free_variables(node.condition))
        free_vars.update(find_free_variables(node.then_branch))
        free_vars.update(find_free_variables(node.else_branch))
    elif isinstance(node, Quantifier):
        body_vars = find_free_variables(node.body)
        free_vars.update(body_vars - {node.parameter.name})
        if node.domain:
            free_vars.update(find_free_variables(node.domain))
    
    return free_vars


def ast_to_string(node: ASTNode, indent: int = 0) -> str:
    """Convert AST to indented string representation for debugging."""
    prefix = "  " * indent
    
    if isinstance(node, Literal):
        return f"{prefix}Literal({node.value})"
    elif isinstance(node, Variable):
        return f"{prefix}Variable({node.name})"
    elif isinstance(node, BinaryOp):
        left_str = ast_to_string(node.left, indent + 1)
        right_str = ast_to_string(node.right, indent + 1)
        return f"{prefix}BinaryOp({node.operator})\n{left_str}\n{right_str}"
    elif isinstance(node, Lambda):
        params_str = ", ".join(repr(p) for p in node.parameters)
        body_str = ast_to_string(node.body, indent + 1)
        return f"{prefix}Lambda([{params_str}])\n{body_str}"
    elif isinstance(node, Application):
        func_str = ast_to_string(node.func, indent + 1)
        args_str = "\n".join(ast_to_string(a, indent + 1) for a in node.args)
        return f"{prefix}Application\n{func_str}\n{args_str}"
    else:
        return f"{prefix}{repr(node)}"
