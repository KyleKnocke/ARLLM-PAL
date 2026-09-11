/**
 * Type System for PAL (Programmatic Abstraction Language)
 * 
 * Supports:
 * - 7 primitive types: ⊤⊥𝔹ℤℝ𝕾
 * - 6 composite types: Function, Product, Union, Array, Boxed, Pointer
 * - Robinson unification algorithm for type inference
 * - Scope-aware type environments
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <iostream>

namespace arllm {

// Forward declarations
class Type;
class PrimitiveType;
class FunctionType;
class ProductType;
class UnionType;
class ArrayType;
class BoxedType;
class PointerType;
class TypeVariable;

using TypePtr = std::shared_ptr<Type>;
using TypeVariant = std::variant<
    PrimitiveType,
    FunctionType,
    ProductType,
    UnionType,
    ArrayType,
    BoxedType,
    PointerType,
    TypeVariable
>;

// Base type class
class Type {
public:
    virtual ~Type() = default;
    virtual std::string to_string() const = 0;
    virtual bool equals(const Type& other) const = 0;
};

// Primitive types: ⊤⊥𝔹ℤℝ𝕾
class PrimitiveType : public Type {
public:
    enum Kind { TOP, BOTTOM, BOOL, INT, REAL, STRING };
    
    explicit PrimitiveType(Kind k) : kind(k) {}
    explicit PrimitiveType(const std::string& symbol);
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    Kind kind;
};

// Function type: param -> return
class FunctionType : public Type {
public:
    FunctionType(TypePtr p, TypePtr r) : param_type(p), return_type(r) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    TypePtr param_type;
    TypePtr return_type;
};

// Product type: A × B
class ProductType : public Type {
public:
    ProductType(std::vector<TypePtr> ts) : types(std::move(ts)) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    std::vector<TypePtr> types;
};

// Union type: A ∪ B
class UnionType : public Type {
public:
    UnionType(std::vector<TypePtr> ts) : types(std::move(ts)) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    std::vector<TypePtr> types;
};

// Array type: [A]
class ArrayType : public Type {
public:
    explicit ArrayType(TypePtr elem) : element_type(elem) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    TypePtr element_type;
};

// Boxed type: Box A
class BoxedType : public Type {
public:
    explicit BoxedType(TypePtr inner) : inner_type(inner) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    TypePtr inner_type;
};

// Pointer type: *A
class PointerType : public Type {
public:
    explicit PointerType(TypePtr pointee) : pointed_type(pointee) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    TypePtr pointed_type;
};

// Type variable for inference: α₁, α₂, ...
class TypeVariable : public Type {
public:
    explicit TypeVariable(const std::string& n) : name(n) {}
    
    std::string to_string() const override;
    bool equals(const Type& other) const override;
    
    std::string name;
};

// Type utilities
std::optional<std::map<std::string, TypePtr>> 
unify(const Type& t1, const Type& t2);

bool is_subtype(const Type& sub, const Type& super_type);

// Type environment for scope-aware binding
class TypeEnvironment {
public:
    TypeEnvironment() = default;
    
    void push_scope();
    void pop_scope();
    
    void bind(const std::string& name, TypePtr type);
    TypePtr lookup(const std::string& name) const;
    
private:
    std::vector<std::map<std::string, TypePtr>> scopes;
};

} // namespace arllm
