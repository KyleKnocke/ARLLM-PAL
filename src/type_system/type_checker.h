/**
 * Type Checker for PAL/Ø.
 *
 * Performs Hindley-Milner style type inference over the unified ExprNode
 * tree, dispatching purely on Opcode (no RTTI / dynamic_cast needed).
 *
 * Because bound variables carry de Bruijn indices rather than names, the
 * local type environment is just a stack (vector<TypePtr>) where index 0 is
 * the innermost binder — pushing/popping on entry/exit of Lambda, Let,
 * Forall/Exists, and Iter is all that's needed; there is no name lookup.
 *
 * Global references (Opcode::GVar) index into `global_types`, populated in
 * declaration order as each top-level definition is checked — a GVar may
 * only refer to a definition that was already type-checked (use Opcode::Fix
 * for recursion instead of forward references).
 *
 * Operator types (+, -, ∧, =, ...) are opcodes, not names, so their
 * signatures are simply hard-coded in the infer_type switch — no "prelude
 * environment" indirection is needed at all.
 */

#pragma once

#include "ast_nodes.h"
#include "../common/types.h"
#include <vector>
#include <map>
#include <optional>

namespace arllm {

struct TypeConstraint {
    TypePtr lhs;
    TypePtr rhs;
};

class TypeChecker {
public:
    TypeChecker() = default;

    // Type-check an entire program; returns per-definition inferred types.
    std::vector<TypePtr> check_program(const Program& program);

    // Infer the type of a single expression node given its local (de
    // Bruijn-indexed) scope stack; index 0 = innermost bound variable.
    TypePtr infer_type(const ExprNode& node, std::vector<TypePtr>& locals);

    // Solve all generated constraints via Robinson unification.
    std::optional<std::map<std::string, TypePtr>> solve_constraints();

    const std::vector<TypeConstraint>& get_constraints() const { return constraints; }

private:
    std::vector<TypeConstraint> constraints;
    std::vector<TypePtr> global_types; // GVar(i) -> type of the i-th definition
    int var_counter = 0;

    std::string fresh_var_name();
    TypePtr fresh_type_var();
    TypePtr apply_substitution(const Type& typ, const std::map<std::string, TypePtr>& subst);
};

} // namespace arllm

