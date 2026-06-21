#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "AST.h"
#include "Token.h"

class Codegen {
public:
    // Main generation entry point
    std::string generate_c_code(const std::vector<std::unique_ptr<ASTNode>>& ast);

    void set_source_dir(const std::string& dir) { 
        source_dir = dir; 
    }
    
    void set_stdlib_path(const std::string& path) { 
        stdlib_path = path; 
    }

private:
	std::unordered_set<std::string> current_function_params;

    std::string source_dir = "";
    std::string stdlib_path = ""; 
    
    std::vector<std::string> includes;
    std::string struct_definitions;
    std::string function_definitions;
    std::unordered_map<std::string, std::string> variable_types;

    // Formatting helper functions
    std::string c_type_from_value(const std::string& value);
    std::string printf_format_for(const std::string& expr);
    
    // Node generation worker
    std::string generate_node(ASTNode* node);
};