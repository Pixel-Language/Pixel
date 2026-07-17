#include "TypeChecker.h"
#include <iostream>
#include <cstdlib>

// helper func

bool TypeChecker::is_numeric(const TypeInfo& t) {
	if (is_unknown(t)) return true; // wildcard passes any check

    return !t.is_array && !t.is_pointer &&
           (t.base_type == TokenType::IntKeyword || t.base_type == TokenType::FloatKeyword);
}

std::string TypeChecker::type_to_string(const TypeInfo& t) {
    std::string base;
    switch (t.base_type) {
        case TokenType::IntKeyword:    base = "Int"; break;
        case TokenType::FloatKeyword:  base = "Float"; break;
        case TokenType::StringKeyword: base = "String"; break;
        case TokenType::BoolKeyword:   base = "Bool"; break;
        case TokenType::VoidKeyword:   base = "Void"; break;
        case TokenType::Identifier:    base = t.struct_name.empty() ? "<unknown>" : t.struct_name; break;
        default:                       base = "<unknown>"; break;
    }
    if (t.is_array)   base = "Array(" + base + ")";
    if (t.is_pointer) base += "*";
    return base;
}

// exact match for now cuz numeric int/float mixing is handled explicitly at call sites
bool TypeChecker::types_compatible(const TypeInfo& lhs, const TypeInfo& rhs) {
    if (is_unknown(lhs) || is_unknown(rhs)) return true; // wildcard matches anything

    // Void* is a generic pointer any pointer converts to/from it
    if (lhs.is_pointer && rhs.is_pointer) {
        if (lhs.base_type == TokenType::VoidKeyword || rhs.base_type == TokenType::VoidKeyword)
            return true;
    }

    if (lhs.is_array != rhs.is_array)     return false;
    if (lhs.is_pointer != rhs.is_pointer) return false;
    if (lhs.base_type == TokenType::FloatKeyword && rhs.base_type == TokenType::IntKeyword) return true;
    if (lhs.base_type != rhs.base_type) return false;
    if (lhs.base_type == TokenType::Identifier) return lhs.struct_name == rhs.struct_name;
    return true;
}

// figure out a literals type from its raw string, or fall back to a
// symbol ttable lookup if its actually an identifier
TypeInfo TypeChecker::default_type_for_literal(const std::string& value) {
    if (value == "true" || value == "false") return TypeInfo(TokenType::BoolKeyword);
    if (value == "nullptr")                  return TypeInfo(TokenType::VoidKeyword); // untyped nil sort of thing

    if (!value.empty() && (isdigit((unsigned char)value[0]) ||
        (value[0] == '-' && value.size() > 1 && isdigit((unsigned char)value[1])))) {
        if (value.find('.') != std::string::npos) return TypeInfo(TokenType::FloatKeyword);
        return TypeInfo(TokenType::IntKeyword);
    }

    // Not numeric/bool/nullptr then well treat as a string literal by default.
    return TypeInfo(TokenType::StringKeyword);
}

// decleration pre pass

void TypeChecker::collect_declarations(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    for (const auto& node : ast) {
        if (auto fn = dynamic_cast<FunctionDeclNode*>(node.get())) {
            functions[fn->name] = fn;
        }
        if (auto sd = dynamic_cast<StructDeclNode*>(node.get())) {
            structs[sd->name] = sd;
        }
    }
}

// entry point

bool TypeChecker::check_program(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    scopes.clear();
    push_scope();

    collect_declarations(ast); // so forward references between functions and sstructs work

    for (const auto& node : ast) {
        check_statement(node.get());
    }

    for (const auto& err : errors) {
        std::cerr << "type error: " << err << "\n";
    }

    return errors.empty();
}

// stmts

void TypeChecker::check_block(const std::vector<std::unique_ptr<ASTNode>>& block) {
    push_scope();
    for (const auto& stmt : block) check_statement(stmt.get());
    pop_scope();
}

void TypeChecker::check_statement(ASTNode* node) {
    if (!node) return;

    // bariables
    if (auto assign = dynamic_cast<AssignNode*>(node)) {
        if (assign->is_declaration) {
            declare(assign->identifier, assign->type_info);
            if (assign->initialized) {
                TypeInfo rhs = check_expression(assign->expression.get());
                if (!types_compatible(assign->type_info, rhs)) {
                    errors.push_back("cannot initialize '" + assign->identifier + "' (" +
                                      type_to_string(assign->type_info) + ") with " + type_to_string(rhs));
                }
            }
            // uninitialized declarations default to the zero value of type_info so yes always fine
        } else {
            TypeInfo lhs;
            if (!find_variable(assign->identifier, lhs)) {
                errors.push_back("assignment to undeclared variable: " + assign->identifier);
                return;
            }
            TypeInfo rhs = check_expression(assign->expression.get());
            if (!types_compatible(lhs, rhs)) {
                errors.push_back("cannot assign " + type_to_string(rhs) + " to '" +
                                  assign->identifier + "' (" + type_to_string(lhs) + ")");
            }
        }
        return;
    }

    if (auto ret = dynamic_cast<ReturnNode*>(node)) {
        TypeInfo actual = check_expression(ret->expression.get());
        if (return_type_stack.empty()) {
            errors.push_back("'return' used outside of a function");
            return;
        }
        TypeInfo expected = return_type_stack.back();
        if (expected.base_type != TokenType::VoidKeyword && !types_compatible(expected, actual)) {
            errors.push_back("return type mismatch: expected " + type_to_string(expected) +
                              ", got " + type_to_string(actual));
        }
        return;
    }

    if (auto if_node = dynamic_cast<IfNode*>(node)) {
        TypeInfo cond = check_expression(if_node->condition.get());
        if (cond.base_type != TokenType::BoolKeyword) {
            errors.push_back("if condition must be Bool, got " + type_to_string(cond));
        }
        check_block(if_node->then_block);

        for (auto& elif_pair : if_node->elif_blocks) {
            TypeInfo elif_cond = check_expression(elif_pair.first.get());
            if (elif_cond.base_type != TokenType::BoolKeyword) {
                errors.push_back("elif condition must be Bool, got " + type_to_string(elif_cond));
            }
            check_block(elif_pair.second);
        }

        check_block(if_node->else_block);
        return;
    }

    if (auto while_node = dynamic_cast<WhileNode*>(node)) {
        TypeInfo cond = check_expression(while_node->condition.get());
        if (cond.base_type != TokenType::BoolKeyword) {
            errors.push_back("while condition must be Bool, got " + type_to_string(cond));
        }
        check_block(while_node->body);
        return;
    }

    if (auto lc = dynamic_cast<LoopControlNode*>(node)) {
        return;
    }

    // @target@ = expr
    if (auto deref_assign = dynamic_cast<DerefAssignNode*>(node)) {
        TypeInfo target_type = check_expression(deref_assign->target.get());
        if (!target_type.is_pointer) {
            errors.push_back("cannot dereference-assign a non-pointer");
            return;
        }
        TypeInfo pointee = target_type;
        pointee.is_pointer = false;

        TypeInfo rhs = check_expression(deref_assign->expression.get());
        if (!types_compatible(pointee, rhs)) {
            errors.push_back("cannot assign " + type_to_string(rhs) + " through pointer to " +
                              type_to_string(pointee));
        }
        return;
    }

    // x->y = z
    if (auto arrow_assign = dynamic_cast<ArrowAssignNode*>(node)) {
        TypeInfo obj_type;
        if (!find_variable(arrow_assign->left, obj_type)) {
            errors.push_back("undefined variable: " + arrow_assign->left);
            return;
        }
        if (obj_type.base_type != TokenType::Identifier || obj_type.struct_name.empty()) {
            errors.push_back(arrow_assign->left + " is not a struct");
            return;
        }
        auto sd = structs.find(obj_type.struct_name);
        if (sd == structs.end()) {
            errors.push_back("unknown struct type: " + obj_type.struct_name);
            return;
        }
        TypeInfo field_type;
        bool found_field = false;
        for (auto& field_node : sd->second->contents) {
            if (auto field = dynamic_cast<AssignNode*>(field_node.get())) {
                if (field->identifier == arrow_assign->right) {
                    field_type = field->type_info;
                    found_field = true;
                    break;
                }
            }
        }
        if (!found_field) {
            errors.push_back("struct '" + obj_type.struct_name + "' has no field '" + arrow_assign->right + "'");
            return;
        }
        TypeInfo rhs = check_expression(arrow_assign->expression.get());
        if (!types_compatible(field_type, rhs)) {
            errors.push_back("cannot assign " + type_to_string(rhs) + " to field '" +
                              arrow_assign->right + "' (" + type_to_string(field_type) + ")");
        }
        return;
    }

    // func
    if (auto fn = dynamic_cast<FunctionDeclNode*>(node)) {
        push_scope();
        for (auto& param : fn->parameters) {
            declare(param.name, param.type_info);
        }
        return_type_stack.push_back(fn->return_type);
        check_block(fn->body_block);
        return_type_stack.pop_back();
        pop_scope();
        return;
    }

    if (auto sd = dynamic_cast<StructDeclNode*>(node)) {
        // just sanity-check field types reference known things; struct-typed
        // fields referencing an undeclared struct name is the main failure mode
        for (auto& field_node : sd->contents) {
            auto field = dynamic_cast<AssignNode*>(field_node.get());
            if (!field) continue;
            if (field->type_info.base_type == TokenType::Identifier &&
                !field->type_info.struct_name.empty() &&
                structs.find(field->type_info.struct_name) == structs.end() &&
                field->type_info.struct_name != sd->name) { // allow self-reference via pointer
                errors.push_back("struct '" + sd->name + "' field '" + field->identifier +
                                  "' has unknown type " + field->type_info.struct_name);
            }
        }
        return;
    }

    /// everything else is expression shaped looking
    check_expression(node);
}


TypeInfo TypeChecker::check_expression(ASTNode* node) {
    if (!node) return unknown_type();

    if (auto lit = dynamic_cast<LiteralNode*>(node)) {
        TypeInfo found;
        if (find_variable(lit->value, found)) return found; // it's a variable
        return default_type_for_literal(lit->value);         // it's an actual literal
    }

    if (auto bin = dynamic_cast<BinOpNode*>(node)) {
        TypeInfo left_type  = check_expression(bin->left.get());
        TypeInfo right_type = check_expression(bin->right.get());

        if (bin->op == "+" || bin->op == "-" || bin->op == "*" || bin->op == "/") {
            if (bin->op == "+" && left_type.base_type == TokenType::StringKeyword &&
                right_type.base_type == TokenType::StringKeyword) {
                return TypeInfo(TokenType::StringKeyword);
            }
            if (!is_numeric(left_type)) {
                errors.push_back("left operand of '" + bin->op + "' must be numeric, got " + type_to_string(left_type));
                return unknown_type();
            }
            if (!is_numeric(right_type)) {
                errors.push_back("right operand of '" + bin->op + "' must be numeric, got " + type_to_string(right_type));
                return unknown_type();
            }
            if (left_type.base_type == TokenType::FloatKeyword || right_type.base_type == TokenType::FloatKeyword) {
                return TypeInfo(TokenType::FloatKeyword);
            }
            return TypeInfo(TokenType::IntKeyword);
        }

        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" ||
            bin->op == "<=" || bin->op == ">=") {
            if (is_numeric(left_type) && is_numeric(right_type)) return TypeInfo(TokenType::BoolKeyword);
            if (left_type.base_type == right_type.base_type)     return TypeInfo(TokenType::BoolKeyword);
            errors.push_back("operands of '" + bin->op + "' must be comparable (" +
                              type_to_string(left_type) + " vs " + type_to_string(right_type) + ")");
            return unknown_type();
        }

        if (bin->op == "and" || bin->op == "or") {
            if (left_type.base_type != TokenType::BoolKeyword) {
                errors.push_back("left operand of '" + bin->op + "' must be Bool");
                return unknown_type();
            }
            if (right_type.base_type != TokenType::BoolKeyword) {
                errors.push_back("right operand of '" + bin->op + "' must be Bool");
                return unknown_type();
            }
            return TypeInfo(TokenType::BoolKeyword);
        }

        errors.push_back("unknown binary operator: " + bin->op);
        return unknown_type();
    }

    if (auto grp = dynamic_cast<GroupingNode*>(node)) {
        return check_expression(grp->expression.get());
    }

    if (auto arr = dynamic_cast<ArrayLiteralNode*>(node)) {
        TypeInfo elem_type;
        bool first = true;
        for (auto& el : arr->elements) {
            TypeInfo t = check_expression(el.get());
            if (first) { elem_type = t; first = false; }
            else if (!types_compatible(elem_type, t)) {
                errors.push_back("array elements have mismatched types (" +
                                  type_to_string(elem_type) + " vs " + type_to_string(t) + ")");
            }
        }
        elem_type.is_array = true;
        return elem_type;
    }

    if (auto idx = dynamic_cast<IndexAccessNode*>(node)) {
        TypeInfo target_type;
        if (!find_variable(idx->target, target_type)) {
            errors.push_back("undefined variable: " + idx->target);
            return unknown_type();
        }
        if (!target_type.is_array) {
            errors.push_back(idx->target + " is not an array");
            return unknown_type();
        }
        TypeInfo index_type = check_expression(idx->index.get());
        if (index_type.base_type != TokenType::IntKeyword) {
            errors.push_back("array index must be Int, got " + type_to_string(index_type));
        }
        TypeInfo elem_type = target_type;
        elem_type.is_array = false;
        return elem_type;
    }

    if (auto drf = dynamic_cast<DereferenceNode*>(node)) {
        TypeInfo target_type = check_expression(drf->target.get());
        if (!target_type.is_pointer) {
            errors.push_back("cannot dereference a non-pointer");
            return unknown_type();
        }
        TypeInfo pointee = target_type;
        pointee.is_pointer = false;
        return pointee;
    }

    if (auto arw = dynamic_cast<ArrowNode*>(node)) {
        TypeInfo obj_type;
        if (!find_variable(arw->left, obj_type)) {
            errors.push_back("undefined variable: " + arw->left);
            return unknown_type();
        }
        if (obj_type.base_type != TokenType::Identifier || obj_type.struct_name.empty()) {
            errors.push_back(arw->left + " is not a struct");
            return unknown_type();
        }
        auto sd = structs.find(obj_type.struct_name);
        if (sd == structs.end()) {
            errors.push_back("unknown struct type: " + obj_type.struct_name);
            return unknown_type();
        }
        for (auto& field_node : sd->second->contents) {
            if (auto field = dynamic_cast<AssignNode*>(field_node.get())) {
                if (field->identifier == arw->right) return field->type_info;
            }
        }
        errors.push_back("struct '" + obj_type.struct_name + "' has no field '" + arw->right + "'");
        return unknown_type();
    }

    if (auto call = dynamic_cast<FunctionCallNode*>(node)) {
        auto it = functions.find(call->name);
        if (it == functions.end()) {
            // if its unknown to the type checker then its probably a native/builtin
            // Skip strict checking rather than falsepositiverroring on every builtin
            for (auto& arg : call->arguments) check_expression(arg.get());
            return unknown_type();
        }
        FunctionDeclNode* fn = it->second;
        if (call->arguments.size() != fn->parameters.size()) {
            errors.push_back("function '" + call->name + "' expects " +
                              std::to_string(fn->parameters.size()) + " argument(s), got " +
                              std::to_string(call->arguments.size()));
        } else {
            for (size_t i = 0; i < call->arguments.size(); ++i) {
                TypeInfo arg_type = check_expression(call->arguments[i].get());
                if (!types_compatible(fn->parameters[i].type_info, arg_type)) {
                    errors.push_back("argument " + std::to_string(i + 1) + " to '" + call->name +
                                      "' expects " + type_to_string(fn->parameters[i].type_info) +
                                      ", got " + type_to_string(arg_type));
                }
            }
        }
        return fn->return_type;
    }

    errors.push_back("cannot type-check this expression");
    return unknown_type();
}