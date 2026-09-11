#include "type_checker.h"
#include <stdexcept>
#include <memory>

namespace arllm {

std::string TypeChecker::fresh_var_name() {
    return "α" + std::to_string(++var_counter);
}

TypePtr TypeChecker::fresh_type_var() {
    return std::make_shared<TypeVariable>(fresh_var_name());
}

std::vector<TypePtr> TypeChecker::check_program(const Program& program) {
    std::vector<TypePtr> results;
    for (const auto& def : program.definitions) {
        std::vector<TypePtr> locals; // empty at top level
        TypePtr t = infer_type(*def, locals);
        global_types.push_back(t);
        results.push_back(t);
    }
    return results;
}

TypePtr TypeChecker::infer_type(const ExprNode& n, std::vector<TypePtr>& locals) {
    auto int_t = std::make_shared<PrimitiveType>(PrimitiveType::INT);
    auto real_t = std::make_shared<PrimitiveType>(PrimitiveType::REAL);
    auto bool_t = std::make_shared<PrimitiveType>(PrimitiveType::BOOL);

    switch (n.op) {
        case Opcode::True:
        case Opcode::False:
            return bool_t;

        case Opcode::Num:
            return real_t;

        case Opcode::Nil:
            return std::make_shared<ArrayType>(fresh_type_var());

        case Opcode::Var: {
            if (n.index < 0 || static_cast<size_t>(n.index) >= locals.size()) {
                throw std::runtime_error("Unbound de Bruijn variable index " + std::to_string(n.index));
            }
            return locals[static_cast<size_t>(n.index)];
        }

        case Opcode::GVar: {
            if (n.index < 0 || static_cast<size_t>(n.index) >= global_types.size()) {
                throw std::runtime_error("Undefined (or forward) global reference index " + std::to_string(n.index));
            }
            return global_types[static_cast<size_t>(n.index)];
        }

        // --- Unary ---
        case Opcode::Neg: {
            TypePtr t = infer_type(*n.children[0], locals);
            constraints.push_back({t, real_t});
            return t;
        }
        case Opcode::Not: {
            TypePtr t = infer_type(*n.children[0], locals);
            constraints.push_back({t, bool_t});
            return bool_t;
        }
        case Opcode::Fix: {
            // fix f :: (A -> A) -> A
            TypePtr f_t = infer_type(*n.children[0], locals);
            TypePtr a = fresh_type_var();
            constraints.push_back({f_t, std::make_shared<FunctionType>(a, a)});
            return a;
        }
        case Opcode::Ret:
            return infer_type(*n.children[0], locals);
        case Opcode::Box:
            return std::make_shared<BoxedType>(infer_type(*n.children[0], locals));
        case Opcode::Deref: {
            TypePtr t = infer_type(*n.children[0], locals);
            if (auto ptr = dynamic_cast<PointerType*>(t.get())) {
                return ptr->pointed_type;
            }
            if (auto box = dynamic_cast<BoxedType*>(t.get())) {
                return box->inner_type;
            }
            throw std::runtime_error("DEREF applied to a non-pointer/non-boxed type");
        }
        case Opcode::Lambda: {
            TypePtr param_t = fresh_type_var();
            locals.insert(locals.begin(), param_t);
            TypePtr body_t = infer_type(*n.children[0], locals);
            locals.erase(locals.begin());
            return std::make_shared<FunctionType>(param_t, body_t);
        }

        // --- Binary arithmetic ---
        case Opcode::Add: case Opcode::Sub: case Opcode::Mul: case Opcode::Div: {
            TypePtr l = infer_type(*n.children[0], locals);
            TypePtr r = infer_type(*n.children[1], locals);
            constraints.push_back({l, real_t});
            constraints.push_back({r, real_t});
            return real_t;
        }

        // --- Binary logical ---
        case Opcode::And: case Opcode::Or: {
            TypePtr l = infer_type(*n.children[0], locals);
            TypePtr r = infer_type(*n.children[1], locals);
            constraints.push_back({l, bool_t});
            constraints.push_back({r, bool_t});
            return bool_t;
        }

        // --- Binary comparison ---
        case Opcode::Eq: case Opcode::Neq: case Opcode::Lt:
        case Opcode::Le: case Opcode::Gt: case Opcode::Ge: {
            TypePtr l = infer_type(*n.children[0], locals);
            TypePtr r = infer_type(*n.children[1], locals);
            constraints.push_back({l, r});
            return bool_t;
        }

        // --- Binary set ops ---
        case Opcode::Mem: case Opcode::NMem: {
            TypePtr elem = infer_type(*n.children[0], locals);
            TypePtr set_t = infer_type(*n.children[1], locals);
            constraints.push_back({set_t, std::make_shared<ArrayType>(elem)});
            return bool_t;
        }
        case Opcode::SubsetEq: case Opcode::Subset: {
            TypePtr l = infer_type(*n.children[0], locals);
            TypePtr r = infer_type(*n.children[1], locals);
            constraints.push_back({l, r});
            return bool_t;
        }
        case Opcode::Union: case Opcode::Intersect: case Opcode::SetMinus: {
            TypePtr l = infer_type(*n.children[0], locals);
            TypePtr r = infer_type(*n.children[1], locals);
            constraints.push_back({l, r});
            return l;
        }

        // --- Application / composition / binding ---
        case Opcode::Apply: {
            TypePtr func_t = infer_type(*n.children[0], locals);
            TypePtr arg_t = infer_type(*n.children[1], locals);
            TypePtr result_t = fresh_type_var();
            constraints.push_back({func_t, std::make_shared<FunctionType>(arg_t, result_t)});
            return result_t;
        }
        case Opcode::Compose: {
            // (f ∘ g) :: A -> C  where f :: B -> C, g :: A -> B
            TypePtr f_t = infer_type(*n.children[0], locals);
            TypePtr g_t = infer_type(*n.children[1], locals);
            TypePtr a = fresh_type_var();
            TypePtr b = fresh_type_var();
            TypePtr c = fresh_type_var();
            constraints.push_back({g_t, std::make_shared<FunctionType>(a, b)});
            constraints.push_back({f_t, std::make_shared<FunctionType>(b, c)});
            return std::make_shared<FunctionType>(a, c);
        }
        case Opcode::Let: {
            TypePtr value_t = infer_type(*n.children[0], locals);
            locals.insert(locals.begin(), value_t);
            TypePtr body_t = infer_type(*n.children[1], locals);
            locals.erase(locals.begin());
            return body_t;
        }
        case Opcode::Forall: case Opcode::Exists: {
            TypePtr domain_t = infer_type(*n.children[0], locals);
            TypePtr elem_t = fresh_type_var();
            constraints.push_back({domain_t, std::make_shared<ArrayType>(elem_t)});
            locals.insert(locals.begin(), elem_t);
            TypePtr body_t = infer_type(*n.children[1], locals);
            locals.erase(locals.begin());
            constraints.push_back({body_t, bool_t});
            return bool_t;
        }
        case Opcode::Idx: {
            TypePtr arr_t = infer_type(*n.children[0], locals);
            TypePtr idx_t = infer_type(*n.children[1], locals);
            constraints.push_back({idx_t, real_t});
            TypePtr elem_t = fresh_type_var();
            constraints.push_back({arr_t, std::make_shared<ArrayType>(elem_t)});
            return elem_t;
        }
        case Opcode::Cons: {
            TypePtr head_t = infer_type(*n.children[0], locals);
            TypePtr tail_t = infer_type(*n.children[1], locals);
            constraints.push_back({tail_t, std::make_shared<ArrayType>(head_t)});
            return tail_t;
        }
        case Opcode::TAnnot: {
            TypePtr expr_t = infer_type(*n.children[0], locals);
            TypePtr annot_t = node_to_type(*n.children[1]);
            constraints.push_back({expr_t, annot_t});
            return annot_t;
        }

        // --- Ternary ---
        case Opcode::Cond: {
            TypePtr cond_t = infer_type(*n.children[0], locals);
            TypePtr then_t = infer_type(*n.children[1], locals);
            TypePtr else_t = infer_type(*n.children[2], locals);
            constraints.push_back({cond_t, bool_t});
            constraints.push_back({then_t, else_t});
            return then_t;
        }
        case Opcode::Iter: {
            // ITER(iterable, init, body): body sees [0]=accumulator, [1]=element
            TypePtr iterable_t = infer_type(*n.children[0], locals);
            TypePtr init_t = infer_type(*n.children[1], locals);
            TypePtr elem_t = fresh_type_var();
            constraints.push_back({iterable_t, std::make_shared<ArrayType>(elem_t)});
            locals.insert(locals.begin(), elem_t);
            locals.insert(locals.begin(), init_t);
            TypePtr body_t = infer_type(*n.children[2], locals);
            locals.erase(locals.begin(), locals.begin() + 2);
            constraints.push_back({body_t, init_t});
            return init_t;
        }

        default:
            throw std::runtime_error("infer_type: unsupported or type-only opcode in expression position");
    }
}

std::optional<std::map<std::string, TypePtr>> TypeChecker::solve_constraints() {
    std::map<std::string, TypePtr> substitution;

    for (const auto& constraint : constraints) {
        TypePtr lhs = apply_substitution(*constraint.lhs, substitution);
        TypePtr rhs = apply_substitution(*constraint.rhs, substitution);

        auto result = unify(*lhs, *rhs);
        if (!result) {
            return std::nullopt;
        }
        substitution.insert(result->begin(), result->end());
    }

    return substitution;
}

TypePtr TypeChecker::apply_substitution(const Type& typ, const std::map<std::string, TypePtr>& subst) {
    if (auto tv = dynamic_cast<const TypeVariable*>(&typ)) {
        auto it = subst.find(tv->name);
        if (it != subst.end()) {
            return apply_substitution(*it->second, subst);
        }
        return std::make_shared<TypeVariable>(*tv);
    } else if (auto ft = dynamic_cast<const FunctionType*>(&typ)) {
        return std::make_shared<FunctionType>(
            apply_substitution(*ft->param_type, subst),
            apply_substitution(*ft->return_type, subst));
    } else if (auto at = dynamic_cast<const ArrayType*>(&typ)) {
        return std::make_shared<ArrayType>(apply_substitution(*at->element_type, subst));
    } else if (auto pt = dynamic_cast<const PointerType*>(&typ)) {
        return std::make_shared<PointerType>(apply_substitution(*pt->pointed_type, subst));
    } else if (auto bt = dynamic_cast<const BoxedType*>(&typ)) {
        return std::make_shared<BoxedType>(apply_substitution(*bt->inner_type, subst));
    }
    return std::make_shared<PrimitiveType>(dynamic_cast<const PrimitiveType&>(typ));
}

} // namespace arllm
