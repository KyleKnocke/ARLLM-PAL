/**
 * Concrete GIMPLE Statement implementations for PAL compiler
 */

#pragma once

#include "builder.h"
#include <string>

namespace arllm {

// Concrete GIMPLE statement types
class GimpleAssignmentStatement : public GimpleStatement {
public:
    std::string left;
    std::string right;
    
    GimpleAssignmentStatement() = default;
    
    std::string to_string() const override {
        return left + " = " + right + ";";
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::Assignment;
    }
};

class GimpleCallStatement : public GimpleStatement {
public:
    std::string function_name;
    std::vector<std::string> args;
    
    GimpleCallStatement() = default;
    
    std::string to_string() const override {
        std::ostringstream oss;
        oss << function_name << "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << args[i];
        }
        oss << ");";
        return oss.str();
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::Call;
    }
};

class GimpleLabelStatement : public GimpleStatement {
public:
    std::string label;
    
    GimpleLabelStatement(const std::string& lbl) : label(lbl) {}
    
    std::string to_string() const override {
        return label + ":";
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::Label;
    }
};

class GimpleBranchStatement : public GimpleStatement {
public:
    std::string target;
    
    GimpleBranchStatement(const std::string& tgt) : target(tgt) {}
    
    std::string to_string() const override {
        return "goto " + target + ";";
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::Branch;
    }
};

class GimpleConditionalBranchStatement : public GimpleStatement {
public:
    std::string condition;
    std::string true_target;
    std::string false_target;
    
    GimpleConditionalBranchStatement(const std::string& cond, const std::string& t, const std::string& f) 
        : condition(cond), true_target(t), false_target(f) {}
    
    std::string to_string() const override {
        return "if (" + condition + ") goto " + true_target + "; else goto " + false_target + ";";
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::ConditionalBranch;
    }
};

class GimpleReturnStatement : public GimpleStatement {
public:
    std::string value;
    
    GimpleReturnStatement() = default;
    GimpleReturnStatement(const std::string& val) : value(val) {}
    
    std::string to_string() const override {
        if (value.empty()) {
            return "return;";
        }
        return "return " + value + ";";
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::Return;
    }
};

class GimpleDeclarationStatement : public GimpleStatement {
public:
    std::string var_name;
    TypePtr type;
    
    GimpleDeclarationStatement(const std::string& name, const TypePtr& t) 
        : var_name(name), type(t) {}
    
    std::string to_string() const override {
        return var_name + " : " + type->to_string() + ";";
    }
    
    GimpleStmtKind kind() const override {
        return GimpleStmtKind::Declaration;
    }
};

} // namespace arllm