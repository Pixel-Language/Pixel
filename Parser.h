#pragma once
#include <vector>
#include <memory>
#include <unordered_set>
#include <string>
#include "Token.h"
#include "AST.h"

class Parser {
public:
    Parser(std::vector<Token> token_list);

    std::vector<std::unique_ptr<ASTNode>> parse_program();

    void set_source_dir(const std::string& dir) { source_dir = dir; }

    bool has_errors() const { return error_count > 0; }
    int get_error_count() const { return error_count; }

private:
    int error_count = 0;
    
    Token peek(size_t offset = 1);
    std::vector<Token> tokens;
    size_t             pos;
    std::string        source_dir;

    void error(const std::string& msg);
    void fatal_error(const std::string& msg);

    // Shared across all Parser instances (including sub-parsers for #use'd files)
    // so that variables and imports stay consistent across the whole compilation.
    inline static std::unordered_set<std::string> declared_vars;
    inline static std::unordered_set<std::string> processed_files;
    inline static std::unordered_set<std::string> known_structs;   // tracks declared struct names

    

    Token current_token();
    void  advance();
    void  expect(TokenType type);     // advance if match, print error otherwise
    bool  is_type_keyword(TokenType t) const;

    std::string token_type_to_string(TokenType type);

    // Searches for a file in: source_dir/ then source_dir/lib/
    // Returns the full path, or "" if not found
    std::string find_library(const std::string& name);

    // Loads, lexes, and parses a .px file; appends its nodes to `out`
    void load_and_parse_file(const std::string& path,
                             std::vector<std::unique_ptr<ASTNode>>& out);

    std::unique_ptr<ASTNode> parse_statement();
    std::unique_ptr<ASTNode> parse_expression();

    std::unique_ptr<ASTNode> parse_funccall(std::string name);

    // Parses any type annotation and returns a filled TypeInfo.
    // Handles: Int, Float, String, Bool, Void, Array(Int), Int*, MyStruct (for now)
    TypeInfo parse_type();

    std::unique_ptr<ASTNode> parse_declaration(TypeInfo type_info);

    // Called by parse_statement() when 'fn' is seen
    std::unique_ptr<ASTNode> parse_function_definition();
   // Called by parse_statement() when 'fn' is seen
    std::unique_ptr<ASTNode> parse_struct_definition();

    std::vector<Parameter> parse_parameters();
};