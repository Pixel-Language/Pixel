#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Token.h"

// base node every ast inherits from this
struct ASTNode {
    virtual ~ASTNode() = default;
};


// raw value; number literal, string literal, bool or identifier
struct LiteralNode : public ASTNode {
    std::string value;
};

// A binary operation between two expressions
struct BinOpNode : public ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

// is statement controls whether ; is added
struct FunctionCallNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    bool is_statement = false;
};

struct ArrayLiteralNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;
};

// Array index access
struct IndexAccessNode : public ASTNode {
    std::string target;                // name of the array variable
    std::unique_ptr<ASTNode> index;    // the index expression
};



struct AssignNode : public ASTNode {
    std::string identifier;
    bool is_declaration = false;
    bool initialized = true;
    TypeInfo    type_info;
    std::unique_ptr<ASTNode> expression;
};

// return <expression>
struct ReturnNode : public ASTNode {
    std::unique_ptr<ASTNode> expression;
};

struct IfNode : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> then_block;
    bool is_unless = false;
};

struct WhileNode : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> body;
};

// ext { <raw C code> }
struct ExtBlockNode : public ASTNode {
    std::string raw_c_code;
};

// #bind "file.h" will adds a C #include to the generated output
struct BindNode : public ASTNode {
    std::string filepath;
};


// One parameter in a function signature: name and type
struct Parameter {
    std::string name;
    TypeInfo    type_info;
};

// fn name(params) -> ReturnType { body }
struct FunctionDeclNode : public ASTNode {
    std::string name;
    std::vector<Parameter> parameters;
    TypeInfo return_type;
    std::vector<std::unique_ptr<ASTNode>> body_block;
};

struct DereferenceNode : public ASTNode {
    std::unique_ptr<ASTNode> target;
    // std::string target;
};

struct StructDeclNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> contents;
};

struct ArrowNode : public ASTNode {
    std::string left;
    std::string right;
};

// val_thing->x = <expression>
struct ArrowAssignNode : public ASTNode {
    std::string left;
    std::string right; 
    std::unique_ptr<ASTNode> expression;
};

// this one represents explicit parenthesis
struct GroupingNode : public ASTNode {
    std::unique_ptr<ASTNode> expression;
};