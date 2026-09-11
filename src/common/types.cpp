#include "types.h"
#include <sstream>
#include <algorithm>

namespace arllm {

// PrimitiveType
PrimitiveType::PrimitiveType(const std::string& symbol) {
    if (symbol == "⊤") kind = TOP;
    else if (symbol == "⊥") kind = BOTTOM;
    else if (symbol == "𝔹") kind = BOOL;
    else if (symbol == "ℤ") kind = INT;
    else if (symbol == "ℝ") kind = REAL;
    else if (symbol == "𝕾") kind = STRING;
    else kind = BOTTOM;
}

std::string PrimitiveType::to_string() const {
    switch (kind) {
        case TOP: return "⊤";
        case BOTTOM: return "⊥";
        case BOOL: return "𝔹";
        case INT: return "ℤ";
        case REAL: return "ℝ";
        case STRING: return "𝕾";
    }
    return "?";
}

bool PrimitiveType::equals(const Type& other) const {
    auto* p = dynamic_cast<const PrimitiveType*>(&other);
    return p && p->kind == kind;
}

// FunctionType
std::string FunctionType::to_string() const {
    return param_type->to_string() + " → " + return_type->to_string();
}

bool FunctionType::equals(const Type& other) const {
    auto* f = dynamic_cast<const FunctionType*>(&other);
    return f && param_type->equals(*f->param_type) && 
           return_type->equals(*f->return_type);
}

// ProductType
std::string ProductType::to_string() const {
    std::string result = "(";
    for (size_t i = 0; i < types.size(); ++i) {
        if (i > 0) result += " × ";
        result += types[i]->to_string();
    }
    result += ")";
    return result;
}

bool ProductType::equals(const Type& other) const {
    auto* p = dynamic_cast<const ProductType*>(&other);
    if (!p || p->types.size() != types.size()) return false;
    for (size_t i = 0; i < types.size(); ++i) {
        if (!types[i]->equals(*p->types[i])) return false;
    }
    return true;
}

// UnionType
std::string UnionType::to_string() const {
    std::string result;
    for (size_t i = 0; i < types.size(); ++i) {
        if (i > 0) result += " ∪ ";
        result += types[i]->to_string();
    }
    return result;
}

bool UnionType::equals(const Type& other) const {
    auto* u = dynamic_cast<const UnionType*>(&other);
    if (!u || u->types.size() != types.size()) return false;
    for (size_t i = 0; i < types.size(); ++i) {
        if (!types[i]->equals(*u->types[i])) return false;
    }
    return true;
}

// ArrayType
std::string ArrayType::to_string() const {
    return "[" + element_type->to_string() + "]";
}

bool ArrayType::equals(const Type& other) const {
    auto* a = dynamic_cast<const ArrayType*>(&other);
    return a && element_type->equals(*a->element_type);
}

// BoxedType
std::string BoxedType::to_string() const {
    return "Box " + inner_type->to_string();
}

bool BoxedType::equals(const Type& other) const {
    auto* b = dynamic_cast<const BoxedType*>(&other);
    return b && inner_type->equals(*b->inner_type);
}

// PointerType
std::string PointerType::to_string() const {
    return "*" + pointed_type->to_string();
}

bool PointerType::equals(const Type& other) const {
    auto* p = dynamic_cast<const PointerType*>(&other);
    return p && pointed_type->equals(*p->pointed_type);
}

// TypeVariable
std::string TypeVariable::to_string() const {
    return name;
}

bool TypeVariable::equals(const Type& other) const {
    auto* tv = dynamic_cast<const TypeVariable*>(&other);
    return tv && tv->name == name;
}

// Unification (Robinson algorithm)
std::optional<std::map<std::string, TypePtr>>
unify(const Type& t1, const Type& t2) {
    std::map<std::string, TypePtr> subst;
    
    // Same type
    if (t1.equals(t2)) {
        return subst;
    }
    
    // TypeVar = Type
    if (auto tv = dynamic_cast<const TypeVariable*>(&t1)) {
        if (!dynamic_cast<const TypeVariable*>(&t2) || tv->name != dynamic_cast<const TypeVariable&>(t2).name) {
            // Occurs check: tv should not appear in t2
            subst[tv->name] = std::make_shared<decltype(t2)>(t2);
            return subst;
        }
    }
    
    // Type = TypeVar
    if (auto tv = dynamic_cast<const TypeVariable*>(&t2)) {
        subst[tv->name] = std::make_shared<decltype(t1)>(t1);
        return subst;
    }
    
    // Function → Function
    if (auto f1 = dynamic_cast<const FunctionType*>(&t1)) {
        if (auto f2 = dynamic_cast<const FunctionType*>(&t2)) {
            auto s1 = unify(*f1->param_type, *f2->param_type);
            if (!s1) return std::nullopt;
            auto s2 = unify(*f1->return_type, *f2->return_type);
            if (!s2) return std::nullopt;
            subst.insert(s1->begin(), s1->end());
            subst.insert(s2->begin(), s2->end());
            return subst;
        }
    }
    
    return std::nullopt;
}

bool is_subtype(const Type& sub, const Type& super_type) {
    return sub.equals(super_type);
}

// TypeEnvironment
void TypeEnvironment::push_scope() {
    scopes.push_back(std::map<std::string, TypePtr>());
}

void TypeEnvironment::pop_scope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

void TypeEnvironment::bind(const std::string& name, TypePtr type) {
    if (scopes.empty()) {
        push_scope();
    }
    scopes.back()[name] = type;
}

TypePtr TypeEnvironment::lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    throw std::runtime_error("Undefined variable: " + name);
}

} // namespace arllm
