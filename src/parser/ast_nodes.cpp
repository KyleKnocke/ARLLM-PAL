#include "ast_nodes.h"
#include <sstream>
#include <stdexcept>

namespace arllm {

// Debug-only pretty printer. Renders the dense glyph tree back into a
// readable (but no longer dense) form purely for humans debugging the
// compiler; this string is never fed back into the tokenizer/parser.
std::string ExprNode::to_string() const {
    auto sym_name = [](Opcode op) {
        switch (op) {
            case Opcode::True: return "true"; case Opcode::False: return "false";
            case Opcode::Nil: return "nil";
            case Opcode::Neg: return "neg"; case Opcode::Not: return "not";
            case Opcode::Fix: return "fix"; case Opcode::Ret: return "ret";
            case Opcode::Box: return "box"; case Opcode::Deref: return "deref";
            case Opcode::Lambda: return "λ";
            case Opcode::Add: return "+"; case Opcode::Sub: return "-";
            case Opcode::Mul: return "*"; case Opcode::Div: return "/";
            case Opcode::And: return "and"; case Opcode::Or: return "or";
            case Opcode::Eq: return "=="; case Opcode::Neq: return "!=";
            case Opcode::Lt: return "<"; case Opcode::Le: return "<=";
            case Opcode::Gt: return ">"; case Opcode::Ge: return ">=";
            case Opcode::Mem: return "in"; case Opcode::NMem: return "notin";
            case Opcode::SubsetEq: return "subseteq"; case Opcode::Subset: return "subset";
            case Opcode::Union: return "union"; case Opcode::Intersect: return "intersect";
            case Opcode::SetMinus: return "setminus";
            case Opcode::Apply: return "apply"; case Opcode::Compose: return "compose";
            case Opcode::Let: return "let"; case Opcode::Forall: return "forall";
            case Opcode::Exists: return "exists"; case Opcode::Idx: return "idx";
            case Opcode::Cons: return "cons"; case Opcode::TAnnot: return "annot";
            case Opcode::Cond: return "cond"; case Opcode::Iter: return "iter";
            case Opcode::TTop: return "⊤"; case Opcode::TBot: return "⊥";
            case Opcode::TBool: return "𝔹"; case Opcode::TInt: return "ℤ";
            case Opcode::TReal: return "ℝ"; case Opcode::TStr: return "𝕾";
            case Opcode::TArr: return "array"; case Opcode::TPtr: return "ptr";
            case Opcode::TBox: return "boxed"; case Opcode::TFun: return "→";
            case Opcode::TProd: return "×"; case Opcode::TUnion: return "∪";
            default: return "?";
        }
    };

    switch (op) {
        case Opcode::Var: return "v" + std::to_string(index);
        case Opcode::GVar: return "g" + std::to_string(index);
        case Opcode::Num: {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
        default: break;
    }

    std::ostringstream oss;
    oss << "(" << sym_name(op);
    for (const auto& c : children) {
        oss << " " << c->to_string();
    }
    oss << ")";
    return oss.str();
}

std::string Program::to_string() const {
    std::ostringstream oss;
    for (size_t i = 0; i < definitions.size(); ++i) {
        oss << "def" << i << " = " << definitions[i]->to_string() << "\n";
    }
    return oss.str();
}

TypePtr node_to_type(const ExprNode& node) {
    switch (node.op) {
        case Opcode::TTop: return std::make_shared<PrimitiveType>(PrimitiveType::TOP);
        case Opcode::TBot: return std::make_shared<PrimitiveType>(PrimitiveType::BOTTOM);
        case Opcode::TBool: return std::make_shared<PrimitiveType>(PrimitiveType::BOOL);
        case Opcode::TInt: return std::make_shared<PrimitiveType>(PrimitiveType::INT);
        case Opcode::TReal: return std::make_shared<PrimitiveType>(PrimitiveType::REAL);
        case Opcode::TStr: return std::make_shared<PrimitiveType>(PrimitiveType::STRING);
        case Opcode::TFun:
            return std::make_shared<FunctionType>(node_to_type(*node.children[0]), node_to_type(*node.children[1]));
        case Opcode::TProd:
            return std::make_shared<ProductType>(std::vector<TypePtr>{
                node_to_type(*node.children[0]), node_to_type(*node.children[1])});
        case Opcode::TUnion:
            return std::make_shared<UnionType>(std::vector<TypePtr>{
                node_to_type(*node.children[0]), node_to_type(*node.children[1])});
        case Opcode::TArr:
            return std::make_shared<ArrayType>(node_to_type(*node.children[0]));
        case Opcode::TPtr:
            return std::make_shared<PointerType>(node_to_type(*node.children[0]));
        case Opcode::TBox:
            return std::make_shared<BoxedType>(node_to_type(*node.children[0]));
        default:
            throw std::runtime_error("node_to_type: node is not a type opcode");
    }
}

} // namespace arllm

