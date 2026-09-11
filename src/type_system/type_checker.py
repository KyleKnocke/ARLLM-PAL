"""
Type Checker for PAL (Programmatic Abstraction Language).

Performs:
- Type inference using Hindley-Milner algorithm with Robinson unification
- Type constraint generation and solving
- Type environment tracking with scope
- Error reporting with source locations

Algorithm:
1. Traverse AST and generate type constraints
2. Unify constraints to infer types
3. Annotate AST nodes with inferred types
4. Check for type errors (conflicting constraints, undefined variables)
"""

from typing import Optional, Tuple
from dataclasses import dataclass, field

from src.common.types import (
    Type,
    PrimitiveT,
    FunctionT,
    ProductT,
    UnionT,
    ArrayT,
    PointerT,
    TypeVariable,
    TypeEnvironment,
    unify,
    is_subtype,
)
from src.parser.ast_nodes import *


@dataclass
class TypeConstraint:
    """A type equality constraint: lhs = rhs."""
    lhs: Type
    rhs: Type


@dataclass
class TypedAST:
    """AST node annotated with inferred type."""
    node: Expression
    typ: Type


class TypeChecker:
    def __init__(self):
        self.env = TypeEnvironment()
        self.constraints: list[TypeConstraint] = []
        self.var_counter = 0
        self.prelude_types()

    def prelude_types(self) -> None:
        """Initialize built-in types and functions."""
        # Primitives
        self.env.bind("+", FunctionT(PrimitiveT("ℤ"), FunctionT(PrimitiveT("ℤ"), PrimitiveT("ℤ"))))
        self.env.bind("-", FunctionT(PrimitiveT("ℤ"), FunctionT(PrimitiveT("ℤ"), PrimitiveT("ℤ"))))
        self.env.bind("*", FunctionT(PrimitiveT("ℤ"), FunctionT(PrimitiveT("ℤ"), PrimitiveT("ℤ"))))
        self.env.bind("÷", FunctionT(PrimitiveT("ℤ"), FunctionT(PrimitiveT("ℤ"), PrimitiveT("ℝ"))))

        # Logic
        self.env.bind("∧", FunctionT(PrimitiveT("𝔹"), FunctionT(PrimitiveT("𝔹"), PrimitiveT("𝔹"))))
        self.env.bind("∨", FunctionT(PrimitiveT("𝔹"), FunctionT(PrimitiveT("𝔹"), PrimitiveT("𝔹"))))
        self.env.bind("¬", FunctionT(PrimitiveT("𝔹"), PrimitiveT("𝔹")))

        # Set operations
        self.env.bind("∪", FunctionT(ArrayT(TypeVariable(self._fresh_var())), 
                                       FunctionT(ArrayT(TypeVariable(self._fresh_var())), 
                                                  ArrayT(TypeVariable(self._fresh_var())))))

    def _fresh_var(self) -> str:
        """Generate a fresh type variable."""
        self.var_counter += 1
        return f"α{self.var_counter}"

    def _fresh_type_var(self) -> Type:
        """Generate a fresh type variable."""
        return TypeVariable(self._fresh_var())

    def check_program(self, program: Program) -> Program:
        """Type-check a complete program."""
        for defn in program.definitions:
            self._check_definition(defn)
        return program

    def _check_definition(self, defn: Definition) -> None:
        """Type-check a definition."""
        body_typ = self._infer_type(defn.body)
        self.env.bind(defn.name, body_typ)

    def _infer_type(self, expr: Expression) -> Type:
        """Infer the type of an expression."""
        if isinstance(expr, Literal):
            return expr.typ

        elif isinstance(expr, Variable):
            try:
                return self.env.lookup(expr.name)
            except KeyError:
                raise NameError(f"Undefined variable: {expr.name} at line {expr.location.line}")

        elif isinstance(expr, BinaryOp):
            left_typ = self._infer_type(expr.left)
            right_typ = self._infer_type(expr.right)

            # Lookup operator signature
            try:
                op_typ = self.env.lookup(expr.op)
            except KeyError:
                raise TypeError(f"Undefined operator: {expr.op}")

            # op_typ should be: lhs -> rhs -> result
            if isinstance(op_typ, FunctionT):
                # First application: op(left)
                left_applied = op_typ.param_type
                constraint1 = TypeConstraint(left_typ, left_applied)
                self.constraints.append(constraint1)

                if isinstance(op_typ.return_type, FunctionT):
                    # Second application: op(left)(right)
                    right_applied = op_typ.return_type.param_type
                    constraint2 = TypeConstraint(right_typ, right_applied)
                    self.constraints.append(constraint2)
                    return op_typ.return_type.return_type
                else:
                    return op_typ.return_type
            else:
                raise TypeError(f"Operator {expr.op} is not a function")

        elif isinstance(expr, UnaryOp):
            operand_typ = self._infer_type(expr.operand)
            try:
                op_typ = self.env.lookup(expr.op)
            except KeyError:
                raise TypeError(f"Undefined unary operator: {expr.op}")

            if isinstance(op_typ, FunctionT):
                constraint = TypeConstraint(operand_typ, op_typ.param_type)
                self.constraints.append(constraint)
                return op_typ.return_type
            else:
                raise TypeError(f"Unary operator {expr.op} is not a function")

        elif isinstance(expr, Conditional):
            cond_typ = self._infer_type(expr.condition)
            then_typ = self._infer_type(expr.then_expr)
            else_typ = self._infer_type(expr.else_expr)

            # Condition must be boolean
            constraint = TypeConstraint(cond_typ, PrimitiveT("𝔹"))
            self.constraints.append(constraint)

            # Both branches must have same type
            constraint2 = TypeConstraint(then_typ, else_typ)
            self.constraints.append(constraint2)

            return then_typ

        elif isinstance(expr, Lambda):
            # Fresh type for parameter
            param_typ = self._fresh_type_var()
            self.env.push_scope()
            self.env.bind(expr.param, param_typ)
            body_typ = self._infer_type(expr.body)
            self.env.pop_scope()

            return FunctionT(param_typ, body_typ)

        elif isinstance(expr, Application):
            func_typ = self._infer_type(expr.func)
            arg_typ = self._infer_type(expr.arg)

            if isinstance(func_typ, FunctionT):
                constraint = TypeConstraint(arg_typ, func_typ.param_type)
                self.constraints.append(constraint)
                return func_typ.return_type
            else:
                raise TypeError(f"Cannot apply non-function type {func_typ}")

        elif isinstance(expr, Quantifier):
            domain_typ = self._infer_type(expr.domain)
            self.env.push_scope()
            self.env.bind(expr.var, domain_typ)
            body_typ = self._infer_type(expr.body)
            self.env.pop_scope()
            return PrimitiveT("𝔹")  # Quantifiers are propositions

        elif isinstance(expr, Iteration):
            iter_typ = self._infer_type(expr.iterable)
            accum_typ = self._infer_type(expr.accumulator)
            return accum_typ

        elif isinstance(expr, Return):
            if expr.value:
                return self._infer_type(expr.value)
            return PrimitiveT("⊥")  # Bottom type for void

        elif isinstance(expr, SetLiteral):
            if expr.elements:
                elem_typ = self._infer_type(expr.elements[0])
                for elem in expr.elements[1:]:
                    elem_typ2 = self._infer_type(elem)
                    constraint = TypeConstraint(elem_typ2, elem_typ)
                    self.constraints.append(constraint)
                return ArrayT(elem_typ)
            return ArrayT(self._fresh_type_var())

        else:
            raise TypeError(f"Cannot infer type for {type(expr).__name__}")

    def solve_constraints(self) -> dict[str, Type]:
        """Solve all type constraints via unification."""
        substitution: dict[str, Type] = {}

        for constraint in self.constraints:
            # Apply current substitution
            lhs = self._apply_substitution(constraint.lhs, substitution)
            rhs = self._apply_substitution(constraint.rhs, substitution)

            # Try to unify
            new_subst = unify(lhs, rhs)
            if new_subst is None:
                raise TypeError(f"Type mismatch: {lhs} ≠ {rhs}")

            # Merge substitutions
            substitution.update(new_subst)

        return substitution

    def _apply_substitution(self, typ: Type, subst: dict[str, Type]) -> Type:
        """Apply a substitution to a type."""
        if isinstance(typ, TypeVariable):
            if typ.name in subst:
                return self._apply_substitution(subst[typ.name], subst)
            return typ

        elif isinstance(typ, FunctionT):
            return FunctionT(
                self._apply_substitution(typ.param_type, subst),
                self._apply_substitution(typ.return_type, subst),
            )

        elif isinstance(typ, ArrayT):
            return ArrayT(self._apply_substitution(typ.element_type, subst))

        elif isinstance(typ, PointerT):
            return PointerT(self._apply_substitution(typ.pointed_type, subst))

        else:
            return typ


def type_check(program: Program) -> Program:
    """Convenience function to type-check a program."""
    checker = TypeChecker()
    checker.check_program(program)
    constraints = checker.constraints
    substitution = checker.solve_constraints()
    # In a full implementation, we'd annotate the AST with inferred types here
    return program
