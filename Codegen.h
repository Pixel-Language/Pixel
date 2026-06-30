#ifndef CODEGEN_H
#define CODEGEN_H

#include "AST.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>

class Codegen {
public:
    Codegen() = default;
    
    void set_source_dir(const std::string& dir) { source_dir = dir; }
    void set_stdlib_path(const std::string& path) { stdlib_path = path; }
    
    std::string generate_c_code(const std::vector<std::unique_ptr<ASTNode>>& ast);
    std::string generate_node(ASTNode* node);
    
    // Type inference helpers
    std::string infer_type(ASTNode* node);
    std::string type_info_to_c_string(const TypeInfo& info);
    
private:
    std::string source_dir = ".";
    std::string stdlib_path = "lib/";
    bool return_code = false;
    
    std::string struct_definitions;
    std::string namespace_defenitions;
    std::string function_definitions;
    std::vector<std::string> includes;
    
    std::unordered_map<std::string, std::string> variable_types;
    std::unordered_map<std::string, std::string> function_return_types;
    std::set<std::string> current_function_params;
    
    // Helper functions
    std::string c_type_from_value(const std::string& value);
    std::string printf_format_for(const std::string& expr);
};

#endif