"""
PAL Type System

This module defines the complete type system for PAL (Programmatic Abstraction Language).
Types are immutable and can be composed recursively.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import List, Dict, Optional, Tuple, Set
from enum import Enum


class PrimitiveType(Enum):
    """Primitive PAL types that map to C types."""
    TOP = "⊤"          # Universal type (void*)
    BOTTOM = "⊥"       # Bottom type (unreachable)
    BOOL = "𝔹"         # Boolean (bool / unsigned char)
    INT = "ℤ"          # Integer (int / int64_t)
    REAL = "ℝ"         # Real number (double)
    STRING = "𝕾"       # String (const char*)


@dataclass(frozen=True)
class Type(ABC):
    """Abstract base class for all PAL types."""
    
    @abstractmethod
    def __str__(self) -> str:
        """String representation of the type."""
        pass
    
    @abstractmethod
    def to_c_type(self) -> str:
        """Convert to C type string for GIMPLE generation."""
        pass


@dataclass(frozen=True)
class PrimitiveT(Type):
    """Primitive type."""
    primitive: PrimitiveType
    
    def __str__(self) -> str:
        return self.primitive.value
    
    def to_c_type(self) -> str:
        """Map PAL primitives to C types."""
        mapping = {
            PrimitiveType.TOP: "void*",
            PrimitiveType.BOTTOM: "void",  # Unreachable type
            PrimitiveType.BOOL: "unsigned char",
            PrimitiveType.INT: "int64_t",
            PrimitiveType.REAL: "double",
            PrimitiveType.STRING: "const char*",
        }
        return mapping[self.primitive]


@dataclass(frozen=True)
class FunctionT(Type):
    """Function type: Domain -> Codomain."""
    domain: Type
    codomain: Type
    
    def __str__(self) -> str:
        return f"({self.domain} → {self.codomain})"
    
    def to_c_type(self) -> str:
        return f"{self.codomain.to_c_type()}(*)(PARAMS)"


@dataclass(frozen=True)
class ProductT(Type):
    """Product type: T1 × T2 (tuple/struct)."""
    types: Tuple[Type, ...]
    
    def __str__(self) -> str:
        inner = " × ".join(str(t) for t in self.types)
        return f"({inner})"
    
    def to_c_type(self) -> str:
        """Generate struct type for GIMPLE."""
        return "struct pal_product"


@dataclass(frozen=True)
class UnionT(Type):
    """Union type: T1 ⊔ T2 (disjoint union / tagged union)."""
    types: Tuple[Type, ...]
    
    def __str__(self) -> str:
        inner = " ⊔ ".join(str(t) for t in self.types)
        return f"({inner})"
    
    def to_c_type(self) -> str:
        """Generate tagged union struct for GIMPLE."""
        return "struct pal_union"


@dataclass(frozen=True)
class ArrayT(Type):
    """Array type: T[N]."""
    element_type: Type
    size: Optional[int] = None  # None for unbounded/VLA
    
    def __str__(self) -> str:
        if self.size is None:
            return f"[]{self.element_type}"
        else:
            return f"[{self.size}]{self.element_type}"
    
    def to_c_type(self) -> str:
        elem_c = self.element_type.to_c_type()
        if self.size is None:
            return f"{elem_c}*"  # Decay to pointer for VLA
        return f"{elem_c}[{self.size}]"


@dataclass(frozen=True)
class BoxedT(Type):
    """Boxed type: □T (lifted/wrapped type)."""
    inner: Type
    
    def __str__(self) -> str:
        return f"□{self.inner}"
    
    def to_c_type(self) -> str:
        """Boxed type becomes a struct wrapper."""
        return f"struct pal_box_{self.inner.to_c_type()}"


@dataclass(frozen=True)
class PointerT(Type):
    """Pointer type: T*."""
    pointee: Type
    
    def __str__(self) -> str:
        return f"{self.pointee}*"
    
    def to_c_type(self) -> str:
        return f"{self.pointee.to_c_type()}*"


# Singleton instances for primitives
TOP_TYPE = PrimitiveT(PrimitiveType.TOP)
BOTTOM_TYPE = PrimitiveT(PrimitiveType.BOTTOM)
BOOL_TYPE = PrimitiveT(PrimitiveType.BOOL)
INT_TYPE = PrimitiveT(PrimitiveType.INT)
REAL_TYPE = PrimitiveT(PrimitiveType.REAL)
STRING_TYPE = PrimitiveT(PrimitiveType.STRING)


class TypeEnvironment:
    """Environment for tracking variable types in a scope."""
    
    def __init__(self, parent: Optional['TypeEnvironment'] = None):
        self.parent = parent
        self.bindings: Dict[str, Type] = {}
    
    def bind(self, name: str, typ: Type) -> None:
        """Bind a variable to a type in the current scope."""
        self.bindings[name] = typ
    
    def lookup(self, name: str) -> Optional[Type]:
        """Look up a variable's type in this scope or parent scopes."""
        if name in self.bindings:
            return self.bindings[name]
        if self.parent:
            return self.parent.lookup(name)
        return None
    
    def extend(self) -> 'TypeEnvironment':
        """Create a child environment."""
        return TypeEnvironment(parent=self)


class TypeSubstitution:
    """Represents a substitution of type variables."""
    
    def __init__(self, subst: Dict[str, Type]):
        self.subst = subst
    
    def apply(self, typ: Type) -> Type:
        """Apply substitution to a type."""
        # For now, just return the type as-is
        # Full implementation would handle type variables
        return typ
    
    def compose(self, other: 'TypeSubstitution') -> 'TypeSubstitution':
        """Compose two substitutions."""
        # Combined substitution
        combined = {**self.subst, **other.subst}
        return TypeSubstitution(combined)


def unify(t1: Type, t2: Type) -> Optional[TypeSubstitution]:
    """
    Attempt to unify two types.
    Returns a substitution if unification succeeds, None otherwise.
    """
    # Base case: same type
    if isinstance(t1, PrimitiveT) and isinstance(t2, PrimitiveT):
        if t1.primitive == t2.primitive:
            return TypeSubstitution({})
        return None
    
    # Structural recursion for composite types
    if isinstance(t1, FunctionT) and isinstance(t2, FunctionT):
        domain_subst = unify(t1.domain, t2.domain)
        if domain_subst is None:
            return None
        codomain_subst = unify(t1.codomain, t2.codomain)
        if codomain_subst is None:
            return None
        return domain_subst.compose(codomain_subst)
    
    # Different kinds don't unify
    return None


def is_subtype(sub: Type, sup: Type) -> bool:
    """
    Check if sub is a subtype of sup.
    
    Subtyping rules:
    - ⊥ is subtype of any type (bottom)
    - Any type is subtype of ⊤ (top)
    - Function types are contravariant in domain
    """
    if isinstance(sub, PrimitiveT) and sub.primitive == PrimitiveType.BOTTOM:
        return True
    if isinstance(sup, PrimitiveT) and sup.primitive == PrimitiveType.TOP:
        return True
    if unify(sub, sup) is not None:
        return True
    return False
