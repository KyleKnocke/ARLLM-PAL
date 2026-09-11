/**
 * GIMPLE IR Builder for PAL
 * 
 * Converts typed AST to GCC GIMPLE three-address code
 */

#pragma once

#include "../parser/ast_nodes.h"
#include "../common/types.h"
#include <string>
#include <vector>
#include <map>

namespace arllm {

// GIMPLE statements
enum class GimpleStmtKind {
    Assignment,
    Call,
    Label,
    Branch,
    ConditionalBranch,
    Return,
    Declaration
};

class GimpleStatement {
public:
    virtual ~GimpleStatement() = default;
    virtual std::string to_string() const = 0;
    virtual GimpleStmtKind kind() const = 0;
};

// Basic block: sequence of statements
class BasicBlock {
public:
    std::string label;
    std::vector<std::shared_ptr<GimpleStatement>> statements;
    
    void add_statement(std::shared_ptr<GimpleStatement> stmt);
    std::string to_string() const;
};

// GIMPLE function
class GimpleFunction {
public:
    std::string name;
    std::vector<std::pair<std::string, TypePtr>> params;
    TypePtr return_type;
    std::vector<std::pair<std::string, TypePtr>> local_vars;
    std::vector<BasicBlock> blocks;
    
    std::string to_string() const;
};

// GIMPLE module
class GimpleModule {
public:
    std::map<std::string, GimpleFunction> functions;
    std::map<std::string, TypePtr> globals;
    
    std::string to_string() const;
};

// Code generator
class GimpleGenerator {
public:
    GimpleModule generate(const Program& program);
    
private:
    std::map<std::string, std::string> temp_vars;
    int temp_counter = 0;
    int label_counter = 0;
    
    std::string gen_temp();
    std::string gen_label();
};

} // namespace arllm
