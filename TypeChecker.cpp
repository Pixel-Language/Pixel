#include "TypeChecker.h"
#include <iostream>
#include <cctype>

void TypeChecker::error(const std::string& msg, const SourceLocation& loc) {
    had_error = true;
    std::cerr << loc.filename << ":" << loc.line << ":" << loc.column << " type error: " << msg << "\n";
}

void TypeChecker::push_scope() {
    scopes.push_back({});
}

void TypeChecker::pop_scope() {
    if (!scopes.empty()) scopes.pop_back();
}

bool TypeChecker::declare_variable(const std::string& name, const TypeInfo& type, const SourceLocation& loc) {
    if (scopes.empty()) push_scope();
    auto& current_scope = scopes.back();
    if (current_scope.find(name) != current_scope.end()) {
        error("variable '" + name + "' already declared in this scope", loc);
        return false;
    }
    current_scope[name] = type;
    return true;
}

bool TypeChecker::is_variable_declared(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->find(name) != it->end()) return true;
    }
    return false;
}

TypeInfo TypeChecker::lookup_variable(const std::string& name, const SourceLocation& loc) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    error("variable '" + name + "' not declared", loc);
    return TypeInfo{};
}

bool TypeChecker::type_compatible(const TypeInfo& a, const TypeInfo& b) const {
    if (a.base_type != b.base_type) return false;
    if (a.is_array != b.is_array) return false;
    if (a.is_pointer != b.is_pointer) return false;
    if (a.struct_name != b.struct_name) return false;
    return true;
}

bool TypeChecker::is_numeric(const TypeInfo& t) const {
    return t.base_type == TokenType::IntKeyword || t.base_type == TokenType::FloatKeyword;
}

bool TypeChecker::is_integer(const TypeInfo& t) const {
    return t.base_type == TokenType::IntKeyword;
}

bool TypeChecker::is_boolean(const TypeInfo& t) const {
    return t.base_type == TokenType::BoolKeyword;
}

bool TypeChecker::is_string(const TypeInfo& t) const {
    return t.base_type == TokenType::StringKeyword;
}

bool TypeChecker::is_pointer(const TypeInfo& t) const {
    return t.is_pointer;
}

bool TypeChecker::is_array(const TypeInfo& t) const {
    return t.is_array;
}

bool TypeChecker::is_struct(const TypeInfo& t) const {
    return !t.struct_name.empty();
}

TypeInfo TypeChecker::get_element_type(const TypeInfo& t) const {
    if (!t.is_array) return t;
    TypeInfo elem = t;
    elem.is_array = false;
    return elem;
}

TypeInfo TypeChecker::get_pointer_target(const TypeInfo& t) const {
    if (!t.is_pointer) return t;
    TypeInfo target = t;
    target.is_pointer = false;
    return target;
}

TypeInfo TypeChecker::get_struct_field_type(const std::string& struct_name, const std::string& field_name, const SourceLocation& loc) {
    // Check if it's a namespace-qualified struct (namespace::struct)
    size_t pos = struct_name.find("::");
    if (pos != std::string::npos) {
        std::string ns_name = struct_name.substr(0, pos);
        std::string struct_name_only = struct_name.substr(pos + 2);
        
        auto ns_it = namespaces.find(ns_name);
        if (ns_it == namespaces.end()) {
            error("namespace '" + ns_name + "' not found", loc);
            return TypeInfo{};
        }
        
        auto struct_it = ns_it->second.structs.find(struct_name_only);
        if (struct_it == ns_it->second.structs.end()) {
            error("struct '" + struct_name_only + "' not found in namespace '" + ns_name + "'", loc);
            return TypeInfo{};
        }
        
        auto field_it = struct_it->second.find(field_name);
        if (field_it == struct_it->second.end()) {
            error("struct '" + struct_name_only + "' has no field '" + field_name + "'", loc);
            return TypeInfo{};
        }
        return field_it->second;
    }
    
    // Regular struct
    auto it = struct_defs.find(struct_name);
    if (it == struct_defs.end()) {
        error("struct '" + struct_name + "' not defined", loc);
        return TypeInfo{};
    }
    
    auto field_it = it->second.find(field_name);
    if (field_it == it->second.end()) {
        error("struct '" + struct_name + "' has no field '" + field_name + "'", loc);
        return TypeInfo{};
    }
    return field_it->second;
}

bool TypeChecker::check(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    errors.clear();
    had_error = false;
    scopes.clear();
    function_signatures.clear();
    struct_defs.clear();
    namespaces.clear();
    push_scope();

    collect_declarations(ast);
    check_program(ast);

    return !had_error;
}

void TypeChecker::collect_declarations(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    for (const auto& node : ast) {
        if (auto func = dynamic_cast<FunctionDeclNode*>(node.get())) {
            FunctionSignature sig;
            sig.return_type = func->return_type;
            sig.parameters = func->parameters;
            function_signatures[func->name] = sig;
        } else if (auto strct = dynamic_cast<StructDeclNode*>(node.get())) {
            std::unordered_map<std::string, TypeInfo> fields;
            for (const auto& field_node : strct->contents) {
                if (auto field = dynamic_cast<AssignNode*>(field_node.get())) {
                    fields[field->identifier] = field->type_info;
                }
            }
            struct_defs[strct->name] = fields;
        } else if (auto nmsp = dynamic_cast<NamespaceNode*>(node.get())) {
            collect_namespace_declarations(nmsp);
        }
    }
}

void TypeChecker::collect_namespace_declarations(NamespaceNode* node) {
    NamespaceInfo ns_info;
    
    for (const auto& item : node->contents) {
        if (auto func = dynamic_cast<FunctionDeclNode*>(item.get())) {
            FunctionSignature sig;
            sig.return_type = func->return_type;
            sig.parameters = func->parameters;
            ns_info.functions[func->name] = sig;
        } else if (auto strct = dynamic_cast<StructDeclNode*>(item.get())) {
            std::unordered_map<std::string, TypeInfo> fields;
            for (const auto& field_node : strct->contents) {
                if (auto field = dynamic_cast<AssignNode*>(field_node.get())) {
                    fields[field->identifier] = field->type_info;
                }
            }
            ns_info.structs[strct->name] = fields;
        } else if (auto assign = dynamic_cast<AssignNode*>(item.get())) {
            if (assign->is_declaration) {
                ns_info.variables[assign->identifier] = assign->type_info;
            }
        }
    }
    
    namespaces[node->name] = ns_info;
}

void TypeChecker::check_program(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    for (const auto& node : ast) {
        check_node(node.get());
    }
}

void TypeChecker::check_node(ASTNode* node) {
    if (!node) return;
    
    if (auto stmt = dynamic_cast<FunctionDeclNode*>(node)) {
        check_function_decl(stmt);
    } else if (auto stmt = dynamic_cast<StructDeclNode*>(node)) {
        // Already collected
    } else if (auto stmt = dynamic_cast<NamespaceNode*>(node)) {
        push_scope();
        for (const auto& child : stmt->contents) {
            check_node(child.get());
        }
        pop_scope();
    } else {
        check_statement(node);
    }
}

void TypeChecker::check_statement(ASTNode* node) {
    if (!node) return;
    
    if (auto assign = dynamic_cast<AssignNode*>(node)) {
        check_assign(assign);
    } else if (auto ret = dynamic_cast<ReturnNode*>(node)) {
        check_return(ret);
    } else if (auto ifnode = dynamic_cast<IfNode*>(node)) {
        check_if(ifnode);
    } else if (auto whilenode = dynamic_cast<WhileNode*>(node)) {
        check_while(whilenode);
    } else if (auto call = dynamic_cast<FunctionCallNode*>(node)) {
        check_function_call(call);
    } else if (auto arrow_assign = dynamic_cast<ArrowAssignNode*>(node)) {
        check_arrow_assign(arrow_assign);
    } else if (auto ext = dynamic_cast<ExtBlockNode*>(node)) {
        // ignore
    } else if (auto bind = dynamic_cast<BindNode*>(node)) {
        // ignore
    } else if (auto group = dynamic_cast<GroupingNode*>(node)) {
        check_expression(group);
    } else {
        check_expression(node);
    }
}

void TypeChecker::check_function_decl(FunctionDeclNode* node) {
    push_scope();
    for (const auto& param : node->parameters) {
        declare_variable(param.name, param.type_info, SourceLocation{});
    }
    
    TypeInfo old_return = current_return_type;
    current_return_type = node->return_type;

    for (const auto& stmt : node->body_block) {
        check_statement(stmt.get());
    }

    current_return_type = old_return;
    pop_scope();
}

void TypeChecker::check_struct_decl(StructDeclNode* node) {
    // Already collected
}

bool TypeChecker::struct_exists(const TypeInfo& type_info, const SourceLocation& loc) {
    if (type_info.struct_name.empty()) return true; // Not a struct type
    
    if (!type_info.namespace_name.empty()) {
        // Namespace-qualified struct: look in namespace
        auto ns_it = namespaces.find(type_info.namespace_name);
        if (ns_it == namespaces.end()) {
            error("namespace '" + type_info.namespace_name + "' not found", loc);
            return false;
        }
        auto struct_it = ns_it->second.structs.find(type_info.struct_name);
        if (struct_it == ns_it->second.structs.end()) {
            error("struct '" + type_info.struct_name + "' not found in namespace '" + 
                  type_info.namespace_name + "'", loc);
            return false;
        }
        return true;
    } else {
        // Global struct
        if (struct_defs.find(type_info.struct_name) == struct_defs.end()) {
            error("struct '" + type_info.struct_name + "' not defined", loc);
            return false;
        }
        return true;
    }
}

void TypeChecker::check_assign(AssignNode* node) {
    if (node->is_declaration) {
        // Verify the struct exists (if it's a struct type)
        if (!node->type_info.struct_name.empty()) {
            if (!struct_exists(node->type_info, SourceLocation{})) {
                return;
            }
        }
        
        // Handle Auto type inference
        if (node->type_info.base_type == TokenType::AutoKeyword) {
            if (node->initialized) {
                TypeInfo init_type = check_expression(node->expression.get());
                // Use inferred type
                declare_variable(node->identifier, init_type, SourceLocation{});
                return;
            } else {
                error("cannot use 'Auto' without initializer", SourceLocation{});
                return;
            }
        }
        
        // Check initializer type
        if (node->initialized) {
            TypeInfo init_type = check_expression(node->expression.get());
            if (!type_compatible(node->type_info, init_type)) {
                error("type mismatch in declaration of '" + node->identifier + "'", SourceLocation{});
            }
        }
        
        // Declare the variable with its type
        declare_variable(node->identifier, node->type_info, SourceLocation{});
    } else {
        // Assignment (not declaration)
        if (!is_variable_declared(node->identifier)) {
            error("variable '" + node->identifier + "' not declared", SourceLocation{});
            return;
        }
        TypeInfo var_type = lookup_variable(node->identifier, SourceLocation{});
        if (!node->initialized) {
            error("assignment requires an expression", SourceLocation{});
            return;
        }
        TypeInfo expr_type = check_expression(node->expression.get());
        if (!type_compatible(var_type, expr_type)) {
            error("type mismatch in assignment to '" + node->identifier + "'", SourceLocation{});
        }
    }
}

void TypeChecker::check_return(ReturnNode* node) {
    TypeInfo expr_type = check_expression(node->expression.get());
    if (!type_compatible(current_return_type, expr_type)) {
        error("return type mismatch", SourceLocation{});
    }
}

void TypeChecker::check_if(IfNode* node) {
    TypeInfo cond_type = check_expression(node->condition.get());
    if (!is_boolean(cond_type)) {
        error("if/unless condition must be boolean", SourceLocation{});
    }
    
    push_scope();
    for (const auto& stmt : node->then_block) {
        check_statement(stmt.get());
    }
    pop_scope();

    for (const auto& elif : node->elif_blocks) {
        TypeInfo elif_cond_type = check_expression(elif.first.get());
        if (!is_boolean(elif_cond_type)) {
            error("elif condition must be boolean", SourceLocation{});
        }
        push_scope();
        for (const auto& stmt : elif.second) {
            check_statement(stmt.get());
        }
        pop_scope();
    }

    if (!node->else_block.empty()) {
        push_scope();
        for (const auto& stmt : node->else_block) {
            check_statement(stmt.get());
        }
        pop_scope();
    }
}

void TypeChecker::check_while(WhileNode* node) {
    TypeInfo cond_type = check_expression(node->condition.get());
    if (!is_boolean(cond_type)) {
        error("while condition must be boolean", SourceLocation{});
    }
    push_scope();
    for (const auto& stmt : node->body) {
        check_statement(stmt.get());
    }
    pop_scope();
}

TypeInfo TypeChecker::check_expression(ASTNode* node) {
    if (!node) return TypeInfo{};

    if (auto lit = dynamic_cast<LiteralNode*>(node)) {
        TypeInfo t;
        if (lit->value == "true" || lit->value == "false") {
            t.base_type = TokenType::BoolKeyword;
        } else if (!lit->value.empty() && lit->value.front() == '"') {
            t.base_type = TokenType::StringKeyword;
        } else if (lit->value.find('.') != std::string::npos) {
            t.base_type = TokenType::FloatKeyword;
        } else if (lit->value == "nullptr") {
            t.is_pointer = true;
            t.base_type = TokenType::VoidKeyword;
        } else if (!lit->value.empty() && (std::isdigit(lit->value[0]) || lit->value[0] == '-')) {
            t.base_type = TokenType::IntKeyword;
        } else {
            return lookup_variable(lit->value, SourceLocation{});
        }
        return t;
    } else if (auto bin = dynamic_cast<BinOpNode*>(node)) {
        return check_binop(bin);
    } else if (auto call = dynamic_cast<FunctionCallNode*>(node)) {
        check_function_call(call);
        auto it = function_signatures.find(call->name);
        if (it != function_signatures.end()) {
            return it->second.return_type;
        }
        error("function '" + call->name + "' not declared", SourceLocation{});
        return TypeInfo{};
    } else if (auto arr = dynamic_cast<ArrayLiteralNode*>(node)) {
        check_array_literal(arr);
        TypeInfo elem_type;
        if (!arr->elements.empty()) {
            elem_type = check_expression(arr->elements[0].get());
            for (size_t i = 1; i < arr->elements.size(); ++i) {
                TypeInfo other = check_expression(arr->elements[i].get());
                if (!type_compatible(elem_type, other)) {
                    error("array literal elements must have same type", SourceLocation{});
                }
            }
        } else {
            elem_type.base_type = TokenType::IntKeyword;
        }
        TypeInfo array_type = elem_type;
        array_type.is_array = true;
        return array_type;
    } else if (auto idx = dynamic_cast<IndexAccessNode*>(node)) {
        return check_index_access(idx);
    } else if (auto deref = dynamic_cast<DereferenceNode*>(node)) {
        return check_dereference(deref);
    } else if (auto group = dynamic_cast<GroupingNode*>(node)) {
        return check_expression(group->expression.get());
    } else if (auto arrow = dynamic_cast<ArrowNode*>(node)) {
        check_arrow(arrow);
        TypeInfo struct_type = lookup_variable(arrow->left, SourceLocation{});
        if (struct_type.struct_name.empty()) {
            error("left side of '->' must be a struct variable", SourceLocation{});
            return TypeInfo{};
        }
        return get_struct_field_type(struct_type.struct_name, arrow->right, SourceLocation{});
    } else if (auto nmsp = dynamic_cast<NamespaceAccNode*>(node)) {
        return check_namespace_acc(nmsp);
    } else {
        error("unknown expression node", SourceLocation{});
        return TypeInfo{};
    }
}

TypeInfo TypeChecker::check_binop(BinOpNode* node) {
    TypeInfo left_type = check_expression(node->left.get());
    TypeInfo right_type = check_expression(node->right.get());

    std::string op = node->op;
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (!is_numeric(left_type) || !is_numeric(right_type)) {
            error("arithmetic operator requires numeric operands", SourceLocation{});
        }
        if (left_type.base_type == TokenType::FloatKeyword || right_type.base_type == TokenType::FloatKeyword)
            return TypeInfo{TokenType::FloatKeyword};
        else
            return TypeInfo{TokenType::IntKeyword};
    } else if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!type_compatible(left_type, right_type)) {
            error("comparison operator requires compatible operand types", SourceLocation{});
        }
        return TypeInfo{TokenType::BoolKeyword};
    } else if (op == "&&" || op == "||") {
        if (!is_boolean(left_type) || !is_boolean(right_type)) {
            error("logical operator requires boolean operands", SourceLocation{});
        }
        return TypeInfo{TokenType::BoolKeyword};
    } else {
        error("unknown binary operator", SourceLocation{});
        return TypeInfo{};
    }
}

void TypeChecker::check_function_call(FunctionCallNode* node) {
    // Check if it's a plain function
    auto it = function_signatures.find(node->name);
    if (it == function_signatures.end()) {
        error("function '" + node->name + "' not declared", SourceLocation{});
        return;
    }
    const auto& sig = it->second;
    if (node->arguments.size() != sig.parameters.size()) {
        error("function '" + node->name + "' expects " + std::to_string(sig.parameters.size()) +
              " arguments, got " + std::to_string(node->arguments.size()), SourceLocation{});
        return;
    }
    for (size_t i = 0; i < node->arguments.size(); ++i) {
        TypeInfo arg_type = check_expression(node->arguments[i].get());
        const TypeInfo& param_type = sig.parameters[i].type_info;
        if (!type_compatible(param_type, arg_type)) {
            error("argument " + std::to_string(i+1) + " of function '" + node->name +
                  "' type mismatch", SourceLocation{});
        }
    }
}

void TypeChecker::check_array_literal(ArrayLiteralNode* node) {
    for (const auto& elem : node->elements) {
        check_expression(elem.get());
    }
}

TypeInfo TypeChecker::check_index_access(IndexAccessNode* node) {
    TypeInfo target_type = lookup_variable(node->target, SourceLocation{});
    if (!target_type.is_array) {
        error("index access on non-array variable '" + node->target + "'", SourceLocation{});
        return TypeInfo{};
    }
    TypeInfo index_type = check_expression(node->index.get());
    if (!is_integer(index_type)) {
        error("array index must be integer", SourceLocation{});
    }
    TypeInfo elem_type = target_type;
    elem_type.is_array = false;
    return elem_type;
}

TypeInfo TypeChecker::check_dereference(DereferenceNode* node) {
    TypeInfo target_type = check_expression(node->target.get());
    if (!target_type.is_pointer) {
        error("dereference of non-pointer type", SourceLocation{});
        return TypeInfo{};
    }
    TypeInfo pointed = target_type;
    pointed.is_pointer = false;
    return pointed;
}

void TypeChecker::check_arrow(ArrowNode* node) {
    TypeInfo struct_type = lookup_variable(node->left, SourceLocation{});
    if (struct_type.struct_name.empty()) {
        error("left side of '->' must be a struct variable", SourceLocation{});
    }
}

void TypeChecker::check_arrow_assign(ArrowAssignNode* node) {
    TypeInfo struct_type = lookup_variable(node->left, SourceLocation{});
    if (struct_type.struct_name.empty()) {
        error("left side of '->' must be a struct variable", SourceLocation{});
        return;
    }
    
    // Build qualified name if namespace-qualified
    std::string qualified_name = struct_type.namespace_name.empty()
        ? struct_type.struct_name
        : struct_type.namespace_name + "::" + struct_type.struct_name;
    
    TypeInfo field_type = get_struct_field_type(qualified_name, node->right, SourceLocation{});
    if (field_type.base_type == TokenType::IntKeyword && field_type.struct_name.empty()) {
        return;
    }
    
    TypeInfo expr_type = check_expression(node->expression.get());
    if (!type_compatible(field_type, expr_type)) {
        error("type mismatch in assignment to field '" + node->right + "'", SourceLocation{});
    }
}

TypeInfo TypeChecker::check_namespace_acc(NamespaceAccNode* node) {
    auto ns_it = namespaces.find(node->left);
    if (ns_it == namespaces.end()) {
        error("namespace '" + node->left + "' not found", SourceLocation{});
        return TypeInfo{};
    }
    
    const auto& ns = ns_it->second;
    
    // If right side is a function call, look it up in the namespace
    if (auto call = dynamic_cast<FunctionCallNode*>(node->right.get())) {
        auto func_it = ns.functions.find(call->name);
        if (func_it == ns.functions.end()) {
            error("function '" + call->name + "' not found in namespace '" + node->left + "'", SourceLocation{});
            return TypeInfo{};
        }
        
        // Validate arguments against the namespaced function signature
        const auto& sig = func_it->second;
        if (call->arguments.size() != sig.parameters.size()) {
            error("function '" + call->name + "' expects " + std::to_string(sig.parameters.size()) +
                  " arguments, got " + std::to_string(call->arguments.size()), SourceLocation{});
            return TypeInfo{};
        }
        for (size_t i = 0; i < call->arguments.size(); ++i) {
            TypeInfo arg_type = check_expression(call->arguments[i].get());
            const TypeInfo& param_type = sig.parameters[i].type_info;
            if (!type_compatible(param_type, arg_type)) {
                error("argument " + std::to_string(i+1) + " type mismatch", SourceLocation{});
            }
        }
        
        return sig.return_type;
    }
    
    // Right side is a literal (variable, struct name, etc.)
    if (auto lit = dynamic_cast<LiteralNode*>(node->right.get())) {
        std::string name = lit->value;
        
        // Check if it's a variable
        auto var_it = ns.variables.find(name);
        if (var_it != ns.variables.end()) {
            return var_it->second;
        }
        
        // Check if it's a struct
        auto struct_it = ns.structs.find(name);
        if (struct_it != ns.structs.end()) {
            TypeInfo struct_type;
            struct_type.struct_name = node->left + "::" + name;  // Store qualified name
            return struct_type;
        }
        
        // Check if it's a function (will be called later)
        auto func_it = ns.functions.find(name);
        if (func_it != ns.functions.end()) {
            // For function references, return its return type
            return func_it->second.return_type;
        }
        
        error("symbol '" + name + "' not found in namespace '" + node->left + "'", SourceLocation{});
        return TypeInfo{};
    }
    
    // Right side is something else (function call, etc.)
    return check_expression(node->right.get());
}

