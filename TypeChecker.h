// TypeChecker.h
#pragma once
#include "AST.h"
#include <vector>
#include <unordered_map>
#include <string>

class TypeChecker {
public:
    bool check_program(const std::vector<std::unique_ptr<ASTNode>>& ast);

private:
    // scope stack, same shape as Interpreter::scopes
    std::vector<std::unordered_map<std::string, TypeInfo>> scopes;

    // declared funcs/structs, gathered in a pre-pass so forward refs work
    std::unordered_map<std::string, FunctionDeclNode*> functions;
    std::unordered_map<std::string, StructDeclNode*>   structs;

    // tracks what the current function is allowed to return, for ReturnNode checks
    std::vector<TypeInfo> return_type_stack;

    std::vector<std::string> errors;

    void push_scope() { scopes.emplace_back(); }
    void pop_scope()  { scopes.pop_back(); }

    void declare(const std::string& name, const TypeInfo& t) {
        scopes.back()[name] = t;
    }

    // walk innermost -> outermost, like Interpreter::find_variable
    bool find_variable(const std::string& name, TypeInfo& out) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) { out = found->second; return true; }
        }
        return false;
    }

    void collect_declarations(const std::vector<std::unique_ptr<ASTNode>>& ast);

    void check_statement(ASTNode* node);
    void check_block(const std::vector<std::unique_ptr<ASTNode>>& block);
    TypeInfo check_expression(ASTNode* node);

    bool is_numeric(const TypeInfo& t);
    bool types_compatible(const TypeInfo& lhs, const TypeInfo& rhs);
    std::string type_to_string(const TypeInfo& t);
    TypeInfo default_type_for_literal(const std::string& value);

    TypeInfo unknown_type() { return TypeInfo(TokenType::VoidKeyword); } // non-pointer, non-array Void = wildcard
    bool is_unknown(const TypeInfo& t) {
        return !t.is_array && !t.is_pointer && t.base_type == TokenType::VoidKeyword;
    }
};