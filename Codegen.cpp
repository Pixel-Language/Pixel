#include "Codegen.h"
#include <cctype>
#include <iostream>

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

// 100% pure Pixel generation. No external flags!
std::string Codegen::generate_node(ASTNode* node) {
    if (!node) return "";
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
        std::string target = deref_node->target;

        return "*px_" + target;
    }

    // i can frickin' read

    if (auto bind = dynamic_cast<BindNode*>(node)) {
        std::string full_path = stdlib_path + bind->filepath;
        includes.push_back(full_path);
        return "";
    }

    // variales 'n stuff
    if (auto assign = dynamic_cast<AssignNode*>(node)) {
        std::string value;

        if (assign->initialized) {
            value = generate_node(assign->expression.get());
        }

        std::string final_id = "px_" + assign->identifier;

        if (assign->is_declaration && assign->type_info.base_type == TokenType::SmartKeyword) {
            // Infer type from RHS
            std::string inferred_c_type = "int";  // default
            
            if (auto lit = dynamic_cast<LiteralNode*>(assign->expression.get())) {
                if (!lit->value.empty() && lit->value.front() == '"') {
                    inferred_c_type = "const char*";
                } else if (lit->value.find('.') != std::string::npos) {
                    inferred_c_type = "double";
                } else if (lit->value == "true" || lit->value == "false") {
                    inferred_c_type = "bool";
                } else {
                    std::cerr << "error: can't infer type";
                }
            }
            
            std::string value = generate_node(assign->expression.get());
            return "    " + inferred_c_type + " px_" + assign->identifier + " = " + value + ";\n";
        }

        if (assign->is_declaration) {
            std::string type =
                (assign->type_info.base_type == TokenType::StringKeyword) ? "string" :
                (assign->type_info.base_type == TokenType::BoolKeyword) ? "bool" :
                (assign->type_info.base_type == TokenType::VoidKeyword) ? "void" :
                (assign->type_info.base_type == TokenType::FloatKeyword) ? "float" : "int";

            // If its an array tag the tracker type string with []
            if (assign->type_info.is_array) {
                variable_types[final_id] = type + "[]";
            } else {
                variable_types[final_id] = type;
            }

            std::string c_type;

            if (!assign->type_info.struct_name.empty()) {
                c_type = "struct px_" + assign->type_info.struct_name;
            } else {
                c_type =
                    (type == "string") ? "const char*" :
                    (type == "bool") ? "bool" :
                    (type == "void") ? "void" :
                    (type == "float") ? "double" : "int";
            }

            if (assign->type_info.is_pointer) {
                c_type += "*";
            }

            // array brakcet
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
        struct_definitions += c_code;  // goes before main(), not inside it (i mean, i think.)
        return "";
    }

    // val_thing->x  used as an expression
    if (auto arrow = dynamic_cast<ArrowNode*>(node)) {
        return "px_" + arrow->left + ".px_" + arrow->right;
    }

    // arrow field WRITE val_thing->x = expr
    if (auto arrow_assign = dynamic_cast<ArrowAssignNode*>(node)) {
        std::string rhs = generate_node(arrow_assign->expression.get());
        return "    px_" + arrow_assign->left + ".px_" + arrow_assign->right + " = " + rhs + ";\n";
    }

    // literals and identifiers
    if (auto lit = dynamic_cast<LiteralNode*>(node)) {
        if (!lit->value.empty() && (std::isalpha(lit->value[0]) || lit->value[0] == '_')) {
            if (lit->value != "true" && lit->value != "false") {
                if (current_function_params.find(lit->value) != current_function_params.end()) {
                    return lit->value;  // Raw parameter name
                }
                return "px_" + lit->value;
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

        std::string c_return_type;
        const TypeInfo& rti = func_decl->return_type;

        if (!rti.struct_name.empty()) {
            c_return_type = "struct px_" + rti.struct_name;
        } else {
            switch (rti.base_type) {
                case TokenType::StringKeyword: c_return_type = "const char*"; break;
                case TokenType::BoolKeyword:   c_return_type = "bool";        break;
                case TokenType::FloatKeyword:  c_return_type = "double";      break;
                case TokenType::VoidKeyword:   c_return_type = "void";        break;
                default:                        c_return_type = "int";         break;
            }
        }

        if (rti.is_array)   c_return_type += "*";
        if (rti.is_pointer) c_return_type += "*";

        func_c_code += c_return_type + " px_" + func_decl->name + "(";

        for (size_t i = 0; i < func_decl->parameters.size(); i++) {
            const auto& param = func_decl->parameters[i];
            const TypeInfo& ti = param.type_info;

            std::string param_c_type;

            if (!ti.struct_name.empty()) {
                param_c_type = "struct px_" + ti.struct_name;
            } else {
                switch (ti.base_type) {
                    case TokenType::StringKeyword: param_c_type = "const char*"; break;
                    case TokenType::BoolKeyword:   param_c_type = "bool";        break;
                    case TokenType::FloatKeyword:  param_c_type = "double";      break;
                    case TokenType::VoidKeyword:  param_c_type = "void";      break;
                    default:                        param_c_type = "int";         break;
                }
            }

            if (ti.is_array)   param_c_type += "*";   // arrays decay to pointer in C params
            if (ti.is_pointer) param_c_type += "*";

            func_c_code += param_c_type + " px_" + param.name;
            if (i < func_decl->parameters.size() - 1) func_c_code += ", ";
        }

        func_c_code += ") {\n";

        for (const auto& stmt : func_decl->body_block) {
            func_c_code += generate_node(stmt.get());
        }

        current_function_params.clear();

        func_c_code += "}\n\n";
        function_definitions += func_c_code;
        return "";
    }

    if (auto if_node = dynamic_cast<IfNode*>(node)) {
        std::string c_if = "    if (" + generate_node(if_node->condition.get()) + ") {\n";
        for (const auto& stmt : if_node->then_block) c_if += generate_node(stmt.get());
        c_if += "    }\n";
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
    std::string main_body = "";
    
    for (const auto& node : ast) {
        main_body += generate_node(node.get());
    }

    std::string complete_code =
        "#include <stdio.h>\n"
        "#include <stdbool.h>\n";

    for (const auto& inc : includes) {
        complete_code += "#include \"" + inc + "\"\n";
    }

    complete_code += "\n" + struct_definitions + function_definitions + "int main() {\n" + main_body + "    return 0;\n}\n";

    includes.clear();
    return complete_code;
}