#include "builder.h"
#include <sstream>

namespace arllm {

void BasicBlock::add_statement(std::shared_ptr<GimpleStatement> stmt) {
    statements.push_back(stmt);
}

std::string BasicBlock::to_string() const {
    std::ostringstream oss;
    oss << label << ":\n";
    for (const auto& stmt : statements) {
        oss << "  " << stmt->to_string() << "\n";
    }
    return oss.str();
}

std::string GimpleFunction::to_string() const {
    std::ostringstream oss;
    oss << "function " << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << params[i].first << " : " << params[i].second->to_string();
    }
    oss << ") -> " << return_type->to_string() << " {\n";
    
    for (const auto& block : blocks) {
        oss << block.to_string();
    }
    
    oss << "}\n";
    return oss.str();
}

std::string GimpleModule::to_string() const {
    std::ostringstream oss;
    for (const auto& [name, func] : functions) {
        oss << func.to_string();
    }
    return oss.str();
}

GimpleModule GimpleGenerator::generate(const Program& program) {
    GimpleModule module;
    // TODO: Implement code generation
    return module;
}

std::string GimpleGenerator::gen_temp() {
    return "_t" + std::to_string(temp_counter++);
}

std::string GimpleGenerator::gen_label() {
    return ".L" + std::to_string(label_counter++);
}

} // namespace arllm
