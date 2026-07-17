#include "Interpreter.h"
#include <iostream>
#include <string>
#include <cctype>
#include <cmath>

void Interpreter::error(const std::string& msg) {
    std::cerr << "interpreter error: " << msg << "\n";
}

bool Interpreter::is_numeric(const Value& v) {
    return v.type == Value::TYPE_INT || v.type == Value::TYPE_FLOAT;
}

std::string* Interpreter::intern_string(const std::string& s) {
    string_pool.push_back(s);
    return &string_pool.back();
}

std::vector<Value>* Interpreter::intern_array(std::vector<Value> v) {
    array_pool.push_back(std::move(v));
    return &array_pool.back();
}

std::unordered_map<std::string, Value>* Interpreter::intern_struct(std::unordered_map<std::string, Value> m) {
    struct_pool.push_back(std::move(m));
    return &struct_pool.back();
}

std::string Interpreter::value_to_string(const Value& v) {
    switch (v.type) {
        case Value::TYPE_INT:    return std::to_string(v.as_int);
        case Value::TYPE_FLOAT:  return std::to_string(v.as_float);
        case Value::TYPE_BOOL:   return v.as_bool ? "true" : "false";
        case Value::TYPE_STRING: return *v.as_string;
        case Value::TYPE_POINTER:
            return "ptr";
        case Value::TYPE_ARRAY: {
            std::string out = "[";
            for (size_t i = 0; i < v.as_array->size(); ++i) {
                if (i > 0) out += ", ";
                out += value_to_string((*v.as_array)[i]);
            }
            out += "]";
            return out;
        }
        case Value::TYPE_STRUCT: {
            std::string out = "{";
            bool first = true;
            for (auto& [key, val] : *v.as_struct) {
                if (!first) out += ", ";
                first = false;
                out += key + ": " + value_to_string(val);
            }
            out += "}";
            return out;
        }
        case Value::TYPE_NIL:    return "nil";
    }
    return "unknown";
}

bool Interpreter::values_equal(const Value& a, const Value& b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case Value::TYPE_INT:    return a.as_int == b.as_int;
        case Value::TYPE_FLOAT:  return a.as_float == b.as_float;
        case Value::TYPE_BOOL:   return a.as_bool == b.as_bool;
        case Value::TYPE_STRING: return *a.as_string == *b.as_string;
        case Value::TYPE_ARRAY: {
            if (a.as_array->size() != b.as_array->size()) return false;
            for (size_t i = 0; i < a.as_array->size(); ++i)
                if (!values_equal((*a.as_array)[i], (*b.as_array)[i])) return false;
            return true;
        }
        case Value::TYPE_POINTER:
            return a.as_pointer == b.as_pointer;
        case Value::TYPE_STRUCT: {
            if (a.as_struct->size() != b.as_struct->size()) return false;
            for (auto& [key, val] : *a.as_struct) {
                auto found = b.as_struct->find(key);
                if (found == b.as_struct->end()) return false;
                if (!values_equal(val, found->second)) return false;
            }
            return true;
        }
        case Value::TYPE_NIL:    return true;
    }
    return false;
}

Value Interpreter::make_default_value(const TypeInfo& type_info) {
    if (type_info.is_pointer) {
        return Value::make_pointer(nullptr); // "null" pointe
    }
    if (type_info.is_array) {
        return Value::make_array(intern_array({}));
    }
    switch (type_info.base_type) {
        case TokenType::IntKeyword:    return Value::make_int(0);
        case TokenType::FloatKeyword:  return Value::make_float(0.0);
        case TokenType::BoolKeyword:   return Value::make_bool(false);
        case TokenType::StringKeyword: return Value::make_string(intern_string(""));
        case TokenType::Identifier:
            if (!type_info.struct_name.empty())
                return instantiate_struct(type_info.struct_name);
            error("unknown type for default value: " + type_info.struct_name);
            return Value::make_nil();
        case TokenType::VoidKeyword:
        default:
            return Value::make_nil();
    }
}

Value Interpreter::instantiate_struct(const std::string& struct_name) {
    auto it = structs.find(struct_name);
    if (it == structs.end()) {
        error("unknown struct type: " + struct_name);
        return Value::make_nil();
    }
    StructDeclNode* decl = it->second;

    std::unordered_map<std::string, Value> fields;
    for (auto& field_node : decl->contents) {
        auto* field = dynamic_cast<AssignNode*>(field_node.get());
        if (!field) continue; // shouldnt happen cuz struct contents are always AssignNodes
        fields[field->identifier] = make_default_value(field->type_info);
    }

    return Value::make_struct(intern_struct(std::move(fields)));
}

Value Interpreter::visit_arrow(ArrowNode* node) {
    Value* obj = find_variable(node->left);
    if (!obj) {
        error("undefined variable: " + node->left);
        return Value::make_nil();
    }
    if (obj->type != Value::TYPE_STRUCT) {
        error(node->left + " is not a struct");
        return Value::make_nil();
    }
    auto found = obj->as_struct->find(node->right);
    if (found == obj->as_struct->end()) {
        error("struct '" + node->left + "' has no field '" + node->right + "'");
        return Value::make_nil();
    }
    return found->second;
}

// master dispatcher
Value Interpreter::visit(ASTNode* node) {
    if (!node) return Value::make_nil();

    if (auto lit = dynamic_cast<LiteralNode*>(node))       return visit_literal(lit);
    if (auto bin = dynamic_cast<BinOpNode*>(node))         return visit_binop(bin);
    if (auto assign = dynamic_cast<AssignNode*>(node))     return visit_assign(assign);
    if (auto call = dynamic_cast<FunctionCallNode*>(node)) return visit_call(call);
    if (auto grp = dynamic_cast<GroupingNode*>(node))      return visit_grouping(grp);
    if (auto arr = dynamic_cast<ArrayLiteralNode*>(node))  return visit_array_literal(arr);
    if (auto idx = dynamic_cast<IndexAccessNode*>(node))   return visit_index_access(idx);
    if (auto drf = dynamic_cast<DereferenceNode*>(node))   return visit_dereference(drf);
    if (auto arw = dynamic_cast<ArrowNode*>(node))         return visit_arrow(arw);

    error("cannot evaluate this expression type");
    return Value::make_nil();
}

Value Interpreter::visit_literal(LiteralNode* node) {
    std::string val = node->value;

    if (val == "true")  return Value::make_bool(true);
    if (val == "false") return Value::make_bool(false);

    // String literal (still wrapped in quotes rn)
    if (!val.empty() && val.front() == '"') {
        std::string str = val.substr(1, val.length() - 2);

        std::string result;
        result.reserve(str.length());
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '\\' && i + 1 < str.length()) {
                switch (str[i + 1]) {
                    case 'n':  result.push_back('\n'); ++i; break;
                    case 't':  result.push_back('\t'); ++i; break;
                    case 'r':  result.push_back('\r'); ++i; break;
                    case '\\': result.push_back('\\'); ++i; break;
                    case '"':  result.push_back('"');  ++i; break;
                    case '0':  result.push_back('\0'); ++i; break;
                    default:   result.push_back(str[i]); break;
                }
            } else {
                result.push_back(str[i]);
            }
        }

        return Value::make_string(intern_string(result));
    }

    // Float literal
    if (val.find('.') != std::string::npos) {
        try {
            return Value::make_float(std::stod(val));
        } catch (...) {}
    }

    // Integer literal
    if (!val.empty() && (std::isdigit((unsigned char)val[0]) || val[0] == '-')) {
        try {
            return Value::make_int(std::stoll(val));
        } catch (...) {}
    }

    // Otherwise treat it as a variable reference
    Value* v = find_variable(val);
    if (v) return *v;
    error("undefined variable: " + val);
    return Value::make_nil();
}

Value Interpreter::visit_binop(BinOpNode* node) {
    Value left = visit(node->left.get());
    Value right = visit(node->right.get());
    const std::string& op = node->op;

    if (op == "+") {
        if (left.type == Value::TYPE_INT && right.type == Value::TYPE_INT)
            return Value::make_int(left.as_int + right.as_int);
        if (left.type == Value::TYPE_STRING && right.type == Value::TYPE_STRING)
            return Value::make_string(intern_string(*left.as_string + *right.as_string));
        if (is_numeric(left) && is_numeric(right)) {
            double l = (left.type == Value::TYPE_INT) ? (double)left.as_int : left.as_float;
            double r = (right.type == Value::TYPE_INT) ? (double)right.as_int : right.as_float;
            return Value::make_float(l + r);
        }
        error("+ operands must be numeric or strings");
        return Value::make_nil();
    }

    if (op == "-" || op == "*" || op == "/") {
        if (!is_numeric(left) || !is_numeric(right)) {
            error(op + " operands must be numeric");
            return Value::make_nil();
        }
        bool both_int = (left.type == Value::TYPE_INT && right.type == Value::TYPE_INT);
        double l = (left.type == Value::TYPE_INT) ? (double)left.as_int : left.as_float;
        double r = (right.type == Value::TYPE_INT) ? (double)right.as_int : right.as_float;

        if (op == "/" && r == 0) {
            error("division by zero");
            return Value::make_nil();
        }

        double result = (op == "-") ? (l - r) : (op == "*") ? (l * r) : (l / r);
        return both_int ? Value::make_int((int64_t)result) : Value::make_float(result);
    }

    if (op == "==") return Value::make_bool(values_equal(left, right));
    if (op == "!=") return Value::make_bool(!values_equal(left, right));

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!is_numeric(left) || !is_numeric(right)) {
            error("comparison requires numeric operands");
            return Value::make_nil();
        }
        double l = (left.type == Value::TYPE_INT) ? (double)left.as_int : left.as_float;
        double r = (right.type == Value::TYPE_INT) ? (double)right.as_int : right.as_float;
        bool result =
            (op == "<")  ? (l < r)  :
            (op == ">")  ? (l > r)  :
            (op == "<=") ? (l <= r) : (l >= r);
        return Value::make_bool(result);
    }

    if (op == "and") {
        bool l = (left.type == Value::TYPE_BOOL) ? left.as_bool : false;
        bool r = (right.type == Value::TYPE_BOOL) ? right.as_bool : false;
        return Value::make_bool(l && r);
    }

    if (op == "or") {
        bool l = (left.type == Value::TYPE_BOOL) ? left.as_bool : false;
        bool r = (right.type == Value::TYPE_BOOL) ? right.as_bool : false;
        return Value::make_bool(l || r);
    }

    error("unknown binary operator: " + op);
    return Value::make_nil();
}

Value Interpreter::visit_assign(AssignNode* node) {
    Value val;
    if (node->is_declaration && !node->initialized) {
        val = make_default_value(node->type_info);
    } else {
        val = visit(node->expression.get());
    }

    if (node->is_declaration) {
        declare_variable(node->identifier, val);
    } else {
        Value* existing = find_variable(node->identifier);
        if (existing) *existing = val;
        else error("assignment to undeclared variable: " + node->identifier);
    }

    return val;
}

Value Interpreter::visit_call(FunctionCallNode* node) {
    // i guess ill rplace this if-chain with a name -> handler map
    // once i get more builtins

    if (node->name == "print") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        std::cout << (value_to_string(arg));
        return Value::make_nil();
    }

    if (node->name == "println") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        std::cout << (value_to_string(arg)) << "\n";
        return Value::make_nil();
    }

    if (node->name == "to_string") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        std::string result = value_to_string(arg); // materialize temporaryly
        return Value::make_string(intern_string(result));   // now its a stable pool owned pointer
    }

    if (node->name == "to_int") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        if (arg.type == Value::TYPE_INT)   return arg;
        if (arg.type == Value::TYPE_FLOAT) return Value::make_int((int64_t)arg.as_float); // truncates toward zero
        if (arg.type == Value::TYPE_BOOL)  return Value::make_int(arg.as_bool ? 1 : 0);
        if (arg.type == Value::TYPE_STRING) {
            try {
                return Value::make_int(std::stoll(*arg.as_string));
            } catch (...) {
                error("to_int: cannot convert string '" + *arg.as_string + "' to int");
                return Value::make_nil();
            }
        }
        error("to_int: cannot convert this type to int");
        return Value::make_nil();
    }

    if (node->name == "to_float") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        if (arg.type == Value::TYPE_FLOAT) return arg;
        if (arg.type == Value::TYPE_INT)   return Value::make_float((double)arg.as_int);
        if (arg.type == Value::TYPE_STRING) {
            try {
                return Value::make_float(std::stod(*arg.as_string));
            } catch (...) {
                error("to_float: cannot convert string '" + *arg.as_string + "' to float");
                return Value::make_nil();
            }
        }
        error("to_float: cannot convert this type to float");
        return Value::make_nil();
    }

    if (node->name == "to_bool") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        if (arg.type == Value::TYPE_BOOL) return arg;
        error("to_bool: only bool values can be converted to bool");
        return Value::make_nil();
    }

    if (node->name == "input") {
        std::string str;
        std::cin >> str;
        return Value::make_string(intern_string(str));
    }

    if (node->name == "input_line") {
        std::string str;
        std::getline(std::cin, str);
        return Value::make_string(intern_string(str));
    }

    if (node->name == "length") {
        if (node->arguments.empty()) return Value::make_nil();
        Value arg = visit(node->arguments[0].get());
        if (arg.type == Value::TYPE_STRING && arg.as_string) {
            return Value::make_int((int64_t)arg.as_string->length());
        } else if (arg.type == Value::TYPE_ARRAY && arg.as_array) {
            return Value::make_int((int64_t)arg.as_array->size());
        }
        error("length: argument must be a string or array");
        return Value::make_nil();
    }

    if (node->name == "alloc") {
        if (node->arguments.empty()) {
            error("alloc requires one argument");
            return Value::make_nil();
        }
        Value initial = visit(node->arguments[0].get());
        Value* boxed = allocate_box(initial);
        return Value::make_pointer(boxed);
    }

    // Not a builtin - check if it's a user-defined function.
    auto it = functions.find(node->name);
    if (it != functions.end()) {
        return call_user_function(it->second, node);
    }

    error("unknown function: " + node->name);
    return Value::make_nil();
}

Value* Interpreter::allocate_box(const Value& val) {
    value_pool.push_back(val); // copy the value into the pool
    return &value_pool.back(); // deque gives stable addresses
}

Value Interpreter::call_user_function(FunctionDeclNode* fn, FunctionCallNode* call_node) {
    if (call_node->arguments.size() != fn->parameters.size()) {
        error("function '" + fn->name + "' expects " +
              std::to_string(fn->parameters.size()) + " arguments, got " +
              std::to_string(call_node->arguments.size()));
        return Value::make_nil();
    }

    // Evaluate every argument bwfore pushing the new scope 
    // are expressions from the caller scope, not the calleee
    std::vector<Value> arg_values;
    arg_values.reserve(call_node->arguments.size());
    for (auto& arg_node : call_node->arguments) {
        arg_values.push_back(visit(arg_node.get()));
    }

    ScopeGuard guard(*this);
    for (size_t i = 0; i < fn->parameters.size(); ++i) {
        declare_variable(fn->parameters[i].name, arg_values[i]);
    }

    Value result = Value::make_nil();
    try {
        for (auto& stmt : fn->body_block) {
            visit_statement(stmt.get());
        }
        // fell off the end with no explicit return nil, same as builtins
    } catch (ReturnSignal& sig) {
        result = sig.value;
    }

    return result;
}

Value Interpreter::visit_dereference(DereferenceNode* node) {
    Value ptr_val = visit(node->target.get());
    if (ptr_val.type != Value::TYPE_POINTER) {
        error("cannot dereference non-pointer");
        return Value::make_nil();
    }
    if (ptr_val.as_pointer == nullptr) {
        error("dereferenced a null pointer");
        return Value::make_nil();
    }
    return *ptr_val.as_pointer; // read boxed value
}

Value Interpreter::visit_grouping(GroupingNode* node) {
    // just unwrap and reevaluate
    return visit(node->expression.get());
}

Value Interpreter::visit_array_literal(ArrayLiteralNode* node) {
    std::vector<Value> elements;
    elements.reserve(node->elements.size());
    for (auto& elem_node : node->elements) {
        elements.push_back(visit(elem_node.get()));
    }
    return Value::make_array(intern_array(std::move(elements)));
}

Value Interpreter::visit_index_access(IndexAccessNode* node) {
    Value* arr_var = find_variable(node->target);
    if (!arr_var) {
        error("undefined variable: " + node->target);
        return Value::make_nil();
    }
    if (arr_var->type != Value::TYPE_ARRAY) {
        error(node->target + " is not an array");
        return Value::make_nil();
    }

    Value idx_val = visit(node->index.get());
    if (idx_val.type != Value::TYPE_INT) {
        error("array index must be an integer");
        return Value::make_nil();
    }

    auto& elements = *arr_var->as_array;
    if (idx_val.as_int < 0 || (size_t)idx_val.as_int >= elements.size()) {
        error("array index out of bounds");
        return Value::make_nil();
    }

    return elements[idx_val.as_int];
}

void Interpreter::visit_statement(ASTNode* node) {
    if (!node) return;


    if (auto lc = dynamic_cast<LoopControlNode*>(node)) {
        if (lc->type == "break") throw BreakSignal{};
        else throw ContinueSignal{};
    }

    if (auto ifn = dynamic_cast<IfNode*>(node)) {
        Value cond = visit(ifn->condition.get());
        if (cond.type == Value::TYPE_BOOL && cond.as_bool) {
            ScopeGuard guard(*this);
            for (auto& stmt : ifn->then_block)
                visit_statement(stmt.get());
            return;
        }

        for (auto& elif_pair : ifn->elif_blocks) {
            Value elif_cond = visit(elif_pair.first.get());
            if (elif_cond.type == Value::TYPE_BOOL && elif_cond.as_bool) {
                ScopeGuard guard(*this);
                for (auto& stmt : elif_pair.second)
                    visit_statement(stmt.get());
                return;
            }
        }

        ScopeGuard guard(*this);
        for (auto& stmt : ifn->else_block)
            visit_statement(stmt.get());
        return;
    }

    if (auto deref_assign = dynamic_cast<DerefAssignNode*>(node)) {
        // Evaluate the target to get the pointer
        Value ptr_val = visit(deref_assign->target.get());
        if (ptr_val.type != Value::TYPE_POINTER) {
            error("left side of @= must be a pointer");
            return;
        }
        // Evaluate the RHS
        Value new_val = visit(deref_assign->expression.get());
        
        // Write into the box
        *ptr_val.as_pointer = new_val;
        return;
    }

    if (auto whl = dynamic_cast<WhileNode*>(node)) {
        ScopeGuard guard(*this);
        while (true) {
            Value cond = visit(whl->condition.get());
            if (!(cond.type == Value::TYPE_BOOL && cond.as_bool)) break;

            try {
                for (auto& stmt : whl->body)
                    visit_statement(stmt.get());
            } catch (BreakSignal&) {
                break;
            } catch (ContinueSignal&) {
                continue;
            }
        }
        return;
    }

    if (auto ret = dynamic_cast<ReturnNode*>(node)) {
        Value val = ret->expression ? visit(ret->expression.get()) : Value::make_nil();
        throw ReturnSignal{ val };
    }

    if (auto f = dynamic_cast<FunctionDeclNode*>(node)) {
        functions[f->name] = f;
        return;
    }

    if (auto sd = dynamic_cast<StructDeclNode*>(node)) {
        structs[sd->name] = sd;
        return;
    }

    if (auto aa = dynamic_cast<ArrowAssignNode*>(node)) {
        Value* obj = find_variable(aa->left);
        if (!obj) {
            error("undefined variable: " + aa->left);
            return;
        }
        if (obj->type != Value::TYPE_STRUCT) {
            error(aa->left + " is not a struct");
            return;
        }
        Value new_val = visit(aa->expression.get());
        (*obj->as_struct)[aa->right] = new_val;
        return;
    }

    // everything else is expression-shaped i guess well let visit() sort out which kind
    visit(node);
}

void Interpreter::visit_program(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    for (const auto& node : ast) {
        visit_statement(node.get());
    }
}

void Interpreter::interpret(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    scopes.clear();
    scopes.emplace_back(); // global scope always scopes[0]
    string_pool.clear();
    array_pool.clear();
    struct_pool.clear();
    functions.clear();
    structs.clear();
    try {
        visit_program(ast);
    } catch (ReturnSignal&) {
        error("'return' used outside of a function");
    }
}