#pragma once
#include "AST.h"
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>
#include <string>
#include <cstdint>

struct Value {
    enum Type { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_POINTER, TYPE_STRING, TYPE_ARRAY, TYPE_STRUCT, TYPE_NIL };
    Type type = TYPE_NIL;
    union {
        int64_t as_int;
        double  as_float;
        bool    as_bool;
        std::string* as_string;
        std::vector<Value>* as_array; // points into Interpreter::array_pool, stable like string_pool
        Value* as_pointer;
        std::unordered_map<std::string, Value>* as_struct; // points into Interpreter::struct_pool
    };

    static Value make_int(int64_t v)      { Value val; val.type = TYPE_INT;    val.as_int = v;    return val; }
    static Value make_float(double v)     { Value val; val.type = TYPE_FLOAT;  val.as_float = v;  return val; }
    static Value make_bool(bool v)        { Value val; val.type = TYPE_BOOL;   val.as_bool = v;   return val; }
    static Value make_string(std::string* v) { Value val; val.type = TYPE_STRING; val.as_string = v; return val; }
    static Value make_array(std::vector<Value>* v) { Value val; val.type = TYPE_ARRAY; val.as_array = v; return val; }
    static Value make_pointer(Value* v) { Value val; val.type = TYPE_POINTER; val.as_pointer = v; return val; }
    static Value make_struct(std::unordered_map<std::string, Value>* v) { Value val; val.type = TYPE_STRUCT; val.as_struct = v; return val; }
    static Value make_nil()               { Value val; val.type = TYPE_NIL;   return val; }
};

struct ReturnSignal {
    Value value;
};

struct BreakSignal {};
struct ContinueSignal {};

class Interpreter {
public:
    void interpret(const std::vector<std::unique_ptr<ASTNode>>& ast);

private:
    // var storage
    std::unordered_map<std::string, Value> variables;
    std::vector<std::unordered_map<std::string, Value>> scopes;

    // same thing but with struct instances (field maps)
    std::deque<std::unordered_map<std::string, Value>> struct_pool;

    // declared struct types (name = its field list/defaults)
    std::unordered_map<std::string, StructDeclNode*> structs;

    Value* allocate_box(const Value& val);// push a copy, return stable pointer

    void push_scope() { scopes.emplace_back(); }
    void pop_scope()  { scopes.pop_back(); }

    // RAII wrapperthat pops the scope in its destructor so a scope always
    // gets cleaned up even if a returnsignal exception unwinds through it.
    // ordinary push_scope()/pop_scope() pairs arent exception safe tho a
    // return from inside an if/while body would skip the pop_scope() call
    struct ScopeGuard {
        Interpreter& interp;
        ScopeGuard(Interpreter& i) : interp(i) { interp.push_scope(); }
        ~ScopeGuard() { interp.pop_scope(); }
    };

    // always writes into innermost scope
    void declare_variable(const std::string& name, const Value& val) {
        scopes.back()[name] = val;
    }

    // lookup: walk innermost then outermost, return pointer (a ref) or nullptr
    Value* find_variable(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }

    // Backing storage for string Vvalues deque keeps pointers stable
    // even as more strings get added unlike vector which can reallocate
    std::deque<std::string> string_pool;

    // value pool for our unreal pointers
    std::deque<Value> value_pool;

    // same thin but wirh array values
    std::deque<std::vector<Value>> array_pool;

    // declared funcs
    std::unordered_map<std::string, FunctionDeclNode*> functions;

    // Generic dispatcher so given any expression node, figure out its
    // concrete type and call the matching visit_X returning a Value.
    Value visit(ASTNode* node);

    // expression visitors
    Value visit_literal(LiteralNode* node);
    Value visit_binop(BinOpNode* node);
    Value visit_cast(CastNode* node);
    Value visit_assign(AssignNode* node);
    Value visit_call(FunctionCallNode* node);
    Value visit_ternary(TernaryNode* node);
    Value visit_grouping(GroupingNode* node);
    Value visit_array_literal(ArrayLiteralNode* node);
    Value visit_index_access(IndexAccessNode* node);
    Value visit_dereference(DereferenceNode* node);
    Value visit_arrow(ArrowNode* node); // obj->field read
    // Builds a fresh struct instance with every field set to its types
    // zero value (0 / 0.0 / false / "" / nil array / nil pointer / bla bla bla
    Value instantiate_struct(const std::string& struct_name);
    // Zero value for any TypeInfo used for uninitialized decls, struct fields and whatever
    Value make_default_value(const TypeInfo& type_info);

    // Actually invokes a user-defined function new scope, bind params,
    // run the body, catch ReturnSignal for the result
    Value call_user_function(FunctionDeclNode* fn, FunctionCallNode* call_node);

    // statement level
    // this add IfNode/WhileNode handling or whatever
    void visit_statement(ASTNode* node);
    void visit_program(const std::vector<std::unique_ptr<ASTNode>>& ast);

    // helpers
    void error(const std::string& msg);
    bool is_numeric(const Value& v);
    bool values_equal(const Value& a, const Value& b);
    std::string value_to_string(const Value& v);
    std::string* intern_string(const std::string& s); // adds to pool, returns stable pointer
    std::vector<Value>* intern_array(std::vector<Value> v); // whatsup for arrays
    std::unordered_map<std::string, Value>* intern_struct(std::unordered_map<std::string, Value> m);
};