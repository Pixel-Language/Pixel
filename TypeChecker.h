#pragma once
#include "AST.h"
#include "Token.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

class TypeChecker {
public:
    bool check(const std::vector<std::unique_ptr<ASTNode>>& ast);

private:
    // Symbol tables
    struct FunctionSignature {
        TypeInfo return_type;
        std::vector<Parameter> parameters;
    };

    // Namespace info
    struct NamespaceInfo {
        std::unordered_map<std::string, FunctionSignature> functions;
        std::unordered_map<std::string, TypeInfo> variables;
        std::unordered_map<std::string, std::unordered_map<std::string, TypeInfo>> structs;
    };

    // Global tables
    std::unordered_map<std::string, FunctionSignature> function_signatures;
    std::unordered_map<std::string, std::unordered_map<std::string, TypeInfo>> struct_defs;
    std::unordered_map<std::string, NamespaceInfo> namespaces;

    // Scoped variable types
    std::vector<std::unordered_map<std::string, TypeInfo>> scopes;

    // Current function return type
    TypeInfo current_return_type;

    // Error collection
    std::vector<std::string> errors;
    bool had_error = false;

    // Helpers
    void error(const std::string& msg, const SourceLocation& loc);
    void push_scope();
    void pop_scope();
    bool declare_variable(const std::string& name, const TypeInfo& type, const SourceLocation& loc);
    bool is_variable_declared(const std::string& name) const;
    TypeInfo lookup_variable(const std::string& name, const SourceLocation& loc);
    bool type_compatible(const TypeInfo& a, const TypeInfo& b) const;
    bool is_numeric(const TypeInfo& t) const;
    bool is_integer(const TypeInfo& t) const;
    bool is_boolean(const TypeInfo& t) const;
    bool is_string(const TypeInfo& t) const;
    bool is_pointer(const TypeInfo& t) const;
    bool is_array(const TypeInfo& t) const;
    bool is_struct(const TypeInfo& t) const;
    TypeInfo get_element_type(const TypeInfo& t) const;
    TypeInfo get_pointer_target(const TypeInfo& t) const;
    TypeInfo get_struct_field_type(const std::string& struct_name, const std::string& field_name, const SourceLocation& loc);
    bool struct_exists(const TypeInfo& type_info, const SourceLocation& loc);

    // Checking methods
    void check_program(const std::vector<std::unique_ptr<ASTNode>>& ast);
    void collect_declarations(const std::vector<std::unique_ptr<ASTNode>>& ast);
    void collect_namespace_declarations(NamespaceNode* node);
    void check_node(ASTNode* node);
    void check_statement(ASTNode* node);
    TypeInfo check_expression(ASTNode* node);
    void check_function_decl(FunctionDeclNode* node);
    void check_struct_decl(StructDeclNode* node);
    void check_assign(AssignNode* node);
    void check_return(ReturnNode* node);
    void check_if(IfNode* node);
    void check_while(WhileNode* node);
    TypeInfo check_binop(BinOpNode* node);
    void check_function_call(FunctionCallNode* node);
    void check_array_literal(ArrayLiteralNode* node);
    TypeInfo check_index_access(IndexAccessNode* node);
    TypeInfo check_dereference(DereferenceNode* node);
    void check_arrow(ArrowNode* node);
    void check_arrow_assign(ArrowAssignNode* node);
    TypeInfo check_namespace_acc(NamespaceAccNode* node);
};