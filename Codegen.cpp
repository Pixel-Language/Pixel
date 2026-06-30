#include "Codegen.h"
#include <cctype>
#include <iostream>

// helpers

std::string Codegen::c_type_from_value(const std::string& value) {
    if (value == "true" || value == "false") return "bool";
    if (!value.empty() && value.front() == '"') return "string";
    if (value.find('.') != std::string::npos) return "float";
    return "int";
}

std::string Codegen::printf_format_for(const std::string& expr) {
    if (!expr.empty() && expr.front() == '"') return "%s";
    if (expr.find('.') != std::string::npos) return "%f";

    auto it = variable_types.find(expr);
    if (it != variable_types.end()) {
        if (it->second == "string") return "%s";
        if (it->second == "bool") return "%d";
        if (it->second == "float") return "%f";
    }
    return "%d";
}

// type inference

std::string Codegen::type_info_to_c_string(const TypeInfo& info) {
    if (!info.struct_name.empty()) {
        return "struct px_" + info.struct_name + (info.is_pointer ? "*" : "");
    }
    std::string base;
    switch (info.base_type) {
        case TokenType::IntKeyword:    base = "int"; break;
        case TokenType::FloatKeyword:  base = "double"; break;
        case TokenType::BoolKeyword:   base = "bool"; break;
        case TokenType::StringKeyword: base = "std::string"; break;
        case TokenType::VoidKeyword:   base = "void"; break;
        default:                       base = "int"; break;
    }
    if (info.is_pointer) base += "*";
    if (info.is_array)   base += "[]";
    return base;
}

std::string Codegen::infer_type(ASTNode* node) {
    if (!node) return "int";

    // literal or identifier
    if (auto lit = dynamic_cast<LiteralNode*>(node)) {
        std::string val = lit->value;
        if (!val.empty() && val.front() == '"') return "std::string";
        if (val == "true" || val == "false") return "bool";
        if (val.find('.') != std::string::npos) return "double";
        if (!val.empty() && (std::isdigit(val[0]) || val[0] == '-')) return "int";
        
        // variable lookup
        auto it = variable_types.find(val);
        if (it != variable_types.end()) return it->second;
        return "int";
    }

    // function call
    if (auto call = dynamic_cast<FunctionCallNode*>(node)) {
        auto it = function_return_types.find(call->name);
        if (it != function_return_types.end()) return it->second;
        return "int";
    }

    // binary op
    if (auto bin = dynamic_cast<BinOpNode*>(node)) {
        std::string left_type = infer_type(bin->left.get());
        std::string right_type = infer_type(bin->right.get());
        if (left_type == "double" || right_type == "double") return "double";
        return "int";
    }

    // dereference
    if (auto deref = dynamic_cast<DereferenceNode*>(node)) {
        auto it = variable_types.find(generate_node(deref->target.get()));
        if (it != variable_types.end()) {
            std::string type = it->second;
            if (!type.empty() && type.back() == '*') type.pop_back();
            return type;
        }
        return "int";
    }

    // array literal
    if (auto arr = dynamic_cast<ArrayLiteralNode*>(node)) {
        if (!arr->elements.empty()) {
            std::string elem_type = infer_type(arr->elements[0].get());
            return "std::vector<" + elem_type + ">";
        }
        return "std::vector<int>";
    }

    // array index access
    if (auto idx = dynamic_cast<IndexAccessNode*>(node)) {
        auto it = variable_types.find(idx->target);
        if (it != variable_types.end()) {
            std::string type = it->second;
            if (type.find("std::vector<") == 0 && type.back() == '>') {
                size_t start = 12;
                size_t end = type.size() - 1;
                return type.substr(start, end - start);
            }
            if (type.find("[]") != std::string::npos) {
                return type.substr(0, type.find("[]"));
            }
        }
        return "int";
    }

    return "int";
}

// 100% pure Pixel generation. No external flags!

std::string Codegen::generate_node(ASTNode* node) {
    if (!node) return "";

    if (auto grouping = dynamic_cast<GroupingNode*>(node)) {
        return "(" + generate_node(grouping->expression.get()) + ")";
    }

    if (auto ext_block = dynamic_cast<ExtBlockNode*>(node)) {
        return "    " + ext_block->raw_c_code + "\n";
    }

    if (auto func_call = dynamic_cast<FunctionCallNode*>(node)) {
        std::string call;

        if (func_call->name == "ref") {
            call = "&(" + generate_node(func_call->arguments[0].get()) + ")";
            return call;
        }

        call = "px_" + func_call->name + "(";
        
        for (size_t i = 0; i < func_call->arguments.size(); i++) {
            call += generate_node(func_call->arguments[i].get());
            if (i < func_call->arguments.size() - 1) call += ", ";
        }
        call += ")";

        if (func_call->is_statement) return "    " + call + ";\n";
        return call;
    }

    if (auto deref_node = dynamic_cast<DereferenceNode*>(node)) {
        std::string thing = generate_node(deref_node->target.get());
        return "*(" + thing + ")";
    }

    // i can frickin' read

    if (auto bind = dynamic_cast<BindNode*>(node)) {
        std::string full_path = stdlib_path + bind->filepath;
        includes.push_back(full_path);
        return "";
    }

    if (auto nmsp = dynamic_cast<NamespaceNode*>(node)) {
        bool was_return_code_true = return_code;

        return_code = true;

        std::string code;
        code += "namespace px_" + nmsp->name + " {\n";

        for (const auto& item : nmsp->contents) {
            code += generate_node(item.get());
        }
        
        code += "}\n";
        
        namespace_defenitions += code;

        return_code = was_return_code_true;
        return "";
    }

    // variales 'n stuff
    if (auto assign = dynamic_cast<AssignNode*>(node)) {
        std::string value;

        if (assign->initialized) {
            value = generate_node(assign->expression.get());
        }

        std::string final_id;

        if (assign->is_const) {
            final_id += "const ";
        }

        final_id += "px_" + assign->identifier;

        // auto keyword inference
        if (assign->is_declaration && assign->type_info.base_type == TokenType::AutoKeyword) {
            std::string inferred_c_type = infer_type(assign->expression.get());
            variable_types[assign->identifier] = inferred_c_type;
            std::string rhs = generate_node(assign->expression.get());
            return "    " + inferred_c_type + " " + final_id + " = " + rhs + ";\n";
        }

        if (assign->is_declaration) {
            std::string type =
                (assign->type_info.base_type == TokenType::StringKeyword) ? "string" :
                (assign->type_info.base_type == TokenType::BoolKeyword) ? "bool" :
                (assign->type_info.base_type == TokenType::VoidKeyword) ? "void" :
                (assign->type_info.base_type == TokenType::FloatKeyword) ? "float" : "int";

            // If its an array tag the tracker type string with []
            if (assign->type_info.is_array) {
                variable_types[assign->identifier] = type + "[]";
            } else {
                variable_types[assign->identifier] = type;
            }

            std::string c_type;

            if (!assign->type_info.struct_name.empty()) {
                c_type = "struct px_" + assign->type_info.struct_name;
            } else {
                c_type =
                    (type == "string") ? "std::string" :
                    (type == "bool") ? "bool" :
                    (type == "void") ? "void" :
                    (type == "float") ? "double" : "int";
            }

            if (assign->type_info.is_pointer) {
                c_type += "*";
            }

            // array bracket
            if (assign->initialized) {
                if (assign->type_info.is_array) {
                    return "    " + c_type + " " + final_id + "[] = " + value + ";\n";
                } else {
                    return "    " + c_type + " " + final_id + " = " + value + ";\n";
                }
            } else {
                if (assign->type_info.is_array) {
                    return "    " + c_type + " " + final_id + "[];\n";
                } else {
                    return "    " + c_type + " " + final_id + ";\n";
                }
            }
        }
        else {
            return "    " + final_id + " = " + value + ";\n";
        }
    }

    if (auto bin_op = dynamic_cast<BinOpNode*>(node)) {
        return generate_node(bin_op->left.get()) + " " + bin_op->op + " " + generate_node(bin_op->right.get());
    }

    // structs
    if (auto struct_decl = dynamic_cast<StructDeclNode*>(node)) {
        std::string c_code = "struct px_" + struct_decl->name + " {\n";

        for (const auto& stmt : struct_decl->contents) {
            c_code += generate_node(stmt.get());
        }

        c_code += "};\n\n";

        if (!return_code) {
            struct_definitions += c_code;  // goes before main(), not inside it (i mean, i think.)
            return "";
        } else {
            return c_code;
        }
    }

    // val_thing->x  used as an expression
    if (auto arrow = dynamic_cast<ArrowNode*>(node)) {
        return "px_" + arrow->left + ".px_" + arrow->right;
    }

    if (auto nmsp = dynamic_cast<NamespaceAccNode*>(node)) {
        return "px_" + nmsp->left + "::px_" + nmsp->right;
    }

    // arrow field WRITE val_thing->x = expr
    if (auto arrow_assign = dynamic_cast<ArrowAssignNode*>(node)) {
        std::string rhs = generate_node(arrow_assign->expression.get());
        return "    px_" + arrow_assign->left + ".px_" + arrow_assign->right + " = " + rhs + ";\n";
    }

    // literals and identifiers
    if (auto lit = dynamic_cast<LiteralNode*>(node)) {
        if (!lit->value.empty() && (std::isalpha(lit->value[0]) || lit->value[0] == '_')) {
            if (lit->value != "true" && lit->value != "false" && lit->value != "nullptr") {
                if (current_function_params.find(lit->value) != current_function_params.end()) {
                    return lit->value;  // Raw parameter name
                }
                return "px_" + lit->value;
            }
            
            if (lit->value == "nullptr") {
                return "(void*)0";
            }
        }
        return lit->value;
    }

    if (auto lit = dynamic_cast<ArrayLiteralNode*>(node)) {
        std::string result = "{ ";
        for (size_t i = 0; i < lit->elements.size(); i++) {
            result += generate_node(lit->elements[i].get());
            if (i < lit->elements.size() - 1) result += ", ";
        }
        result += " }";
        return result;
    }

    if (auto acc = dynamic_cast<IndexAccessNode*>(node)) {
        std::string result = "";
        result += "px_";
        result += acc->target; //name
        result += "[";
        result += generate_node(acc->index.get());
        result += "]";
        return result;
    }

    if (auto func_decl = dynamic_cast<FunctionDeclNode*>(node)) {
        current_function_params.clear();

        for (const auto& param : func_decl->parameters) {
            current_function_params.insert(param.name);
        }
        
        std::string func_c_code = "";

        std::string c_return_type = type_info_to_c_string(func_decl->return_type);
        function_return_types[func_decl->name] = c_return_type;

        func_c_code += c_return_type + " px_" + func_decl->name + "(";

        for (size_t i = 0; i < func_decl->parameters.size(); i++) {
            const auto& param = func_decl->parameters[i];
            const TypeInfo& ti = param.type_info;

            std::string param_c_type;
            if (ti.is_array) {
                // Arrays decay to pointers in function parameters
                TypeInfo base_info = ti;
                base_info.is_array = false;  // remove array flag
                param_c_type = type_info_to_c_string(base_info) + "*";
            } else {
                param_c_type = type_info_to_c_string(ti);
            }

            func_c_code += param_c_type + " px_" + param.name;
            if (i < func_decl->parameters.size() - 1) func_c_code += ", ";
        }

        func_c_code += ") {\n";

        for (const auto& stmt : func_decl->body_block) {
            func_c_code += generate_node(stmt.get());
        }

        current_function_params.clear();

        func_c_code += "}\n\n";

        if (!return_code) {
            function_definitions += func_c_code;
            return "";
        } else {
            return func_c_code;
        }
    }

    if (auto if_node = dynamic_cast<IfNode*>(node)) {
        std::string c_if = "    if (" ;
        
        if (if_node->is_unless == true) {
            c_if += "!(";
        }

        c_if += generate_node(if_node->condition.get());

        if (if_node->is_unless == true) {
            c_if += ")";
        }

        c_if += + ") {\n";

        for (const auto& stmt : if_node->then_block) c_if += generate_node(stmt.get());

        c_if += "    }\n";

        // Generate elif blocks
        for (const auto& elif_pair : if_node->elif_blocks) {
            const auto& elif_condition = elif_pair.first;
            const auto& elif_body = elif_pair.second;
            
            c_if += "    else if (";
            c_if += generate_node(elif_condition.get());
            c_if += ") {\n";
            
            for (const auto& stmt : elif_body) {
                c_if += generate_node(stmt.get());
            }
            
            c_if += "    }\n";
        }
        
        // Generate else block
        if (!if_node->else_block.empty()) {
            c_if += "    else {\n";
            for (const auto& stmt : if_node->else_block) {
                c_if += generate_node(stmt.get());
            }
            c_if += "    }\n";
        }

        return c_if;
    }

    if (auto while_node = dynamic_cast<WhileNode*>(node)) {
        std::string c_while = "    while (" + generate_node(while_node->condition.get()) + ") {\n";
        for (const auto& stmt : while_node->body) c_while += generate_node(stmt.get());
        c_while += "    }\n";
        return c_while;
    }

    if (auto ret = dynamic_cast<ReturnNode*>(node)) {
        std::string expr = generate_node(ret->expression.get());
        return "    return " + expr + ";\n";
    }

    return "";
}

std::string Codegen::generate_c_code(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    struct_definitions   = "";
    function_definitions = "";
    namespace_defenitions = "";
    std::string main_body = "";
    
    for (const auto& node : ast) {
        main_body += generate_node(node.get());
    }

    std::string complete_code =
        "#include <stdio.h>\n"
        "#include <stdbool.h>\n"
        "#include <string>\n";

    for (const auto& inc : includes) {
        complete_code += "#include \"" + inc + "\"\n";
    }

    complete_code += "\n" + struct_definitions + function_definitions + namespace_defenitions + "int main() {\n" + main_body + "    return 0;\n}\n";

    includes.clear();
    return complete_code;
}