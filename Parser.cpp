#include "Parser.h"
#include "Lexer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

Token Parser::peek(size_t offset) {
    if (pos + offset >= tokens.size()) return { TokenType::EndOfFile, "" };
    return tokens[pos + offset];
}

Parser::Parser(std::vector<Token> token_list) : tokens(token_list), pos(0) {}

void Parser::error(const std::string& msg) {
    error_count++;
    Token tok = current_token();
    std::cerr << tok.location.filename 
              << ":" << tok.location.line 
              << ":" << tok.location.column 
              << " error: " << msg << "\n";
    // we want to try to continue
}

void Parser::fatal_error(const std::string& msg) {
    Token tok = current_token();
    std::cerr << tok.location.filename 
              << ":" << tok.location.line 
              << ":" << tok.location.column 
              << " fatal: " << msg << "\n";
    exit(1);
}

Token Parser::current_token() {
    if (pos >= tokens.size()) return { TokenType::EndOfFile, "" };
    return tokens[pos];
}

void Parser::advance() {
    if (pos < tokens.size()) pos++;
}

std::string Parser::token_type_to_string(TokenType type) {
    switch(type) {
        case TokenType::Lbrace: return "Lbrace";
        case TokenType::Rbrace: return "Rbrace";
        case TokenType::Identifier: return "Identifier";
        case TokenType::IntKeyword: return "IntKeyword";
        case TokenType::StringKeyword: return "StringKeyword";
        case TokenType::BoolKeyword: return "BoolKeyword";
        case TokenType::FloatKeyword: return "FloatKeyword";
        case TokenType::VoidKeyword: return "VoidKeyword";
        case TokenType::ArrayKeyword: return "ArrayKeyword";
        case TokenType::AutoKeyword: return "AutoKeyword";
        case TokenType::FuncDefine: return "FuncDefine";
        case TokenType::Return: return "Return";
        case TokenType::If: return "If";
        case TokenType::While: return "While";
        case TokenType::Struct: return "Struct";
        case TokenType::Number: return "Number";
        case TokenType::Float: return "Float";
        case TokenType::String: return "String";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::At: return "At";
        case TokenType::EndOfFile: return "EOF";
        case TokenType::Rarrow: return "Rarrow";
        case TokenType::Equals: return "Equals";
        case TokenType::DoubleEquals: return "DoubleEquals";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Mult: return "Mult";
        case TokenType::Div: return "Div";
        case TokenType::Lparen: return "Lparen";
        case TokenType::Rparen: return "Rparen";
        case TokenType::Lbracket: return "Lbracket";
        case TokenType::Rbracket: return "Rbracket";
        case TokenType::LessThan: return "LessThan";
        case TokenType::GreaterThan: return "GreaterThan";
        case TokenType::Comma: return "Comma";
        case TokenType::Colon: return "Colon";
        case TokenType::Use: return "Use";
        case TokenType::And: return "And";
        case TokenType::Or: return "Or";
        case TokenType::ExclamationMark: return "ExclamationMark";
        case TokenType::NotEquals: return "NotEquals";
        case TokenType::LessThanEqualTo: return "LessThanEqualTo";
        case TokenType::GreaterThanEqualTo: return "GreaterThanEqualTo";
        case TokenType::Elif: return "Elif";
        case TokenType::Else: return "Else";
        case TokenType::Const: return "Const";
        default: return "Unknown";
    }
}

// expect will consume the expected token, or report error and skip it
void Parser::expect(TokenType type) {
    if (current_token().type == type) {
        advance();
    } else {
        error("expected " + token_type_to_string(type) + 
              ", got '" + current_token().value + "'");
        // instead of advancing, let's let the caller handle recovery
        // but to avoid infinite loops, we'll skip the bad token
        advance();
    }
}

bool Parser::is_type_keyword(TokenType t) const {
    return t == TokenType::IntKeyword    ||
           t == TokenType::StringKeyword ||
           t == TokenType::BoolKeyword   ||
           t == TokenType::FloatKeyword  ||
           t == TokenType::VoidKeyword   ||
           t == TokenType::ArrayKeyword  ||
           t == TokenType::AutoKeyword;
}

// lib loading

std::string Parser::find_library(const std::string& name) {
    std::vector<std::string> search_paths = {
        source_dir + "/" + name,
        source_dir + "/lib/" + name,
    };
    for (const auto& path : search_paths) {
        std::ifstream test(path);
        if (test.good()) return path;
    }
    return "";
}

void Parser::load_and_parse_file(const std::string& path,
                                 std::vector<std::unique_ptr<ASTNode>>& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fatal_error("could not open file '" + path + "'");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    Lexer lexer(buffer.str(), path); // pass filename for location tracking
    std::vector<Token> tokens = lexer.tokenize();

    // Give the sub-parser the directory of the file being imported
    // so that nested #use paths resolve relative to it
    std::string file_dir = path.substr(0, path.find_last_of("\\/"));
    if (file_dir.empty()) file_dir = ".";

    Parser sub_parser(std::move(tokens));
    sub_parser.set_source_dir(file_dir);

    auto sub_ast = sub_parser.parse_program();
    for (auto& node : sub_ast)
        out.push_back(std::move(node));
}

// parse_program entry point thing

std::vector<std::unique_ptr<ASTNode>> Parser::parse_program() {
    // std::cout << "Total tokens: " << tokens.size() << "\n";
    
    // // Dump all tokens first
    // for (size_t i = 0; i < tokens.size(); i++) {
    //     std::cout << "Token " << i << ": " << token_type_to_string(tokens[i].type) 
    //               << " Value: '" << tokens[i].value << "'\n";
    // }
    // std::cout << "end of token dump\n\n";

    std::vector<std::unique_ptr<ASTNode>> program;

    // Auto-import builtins.px from the lib/ folder (if it hasn't been loaded yet)
    std::string builtins_path = find_library("lib/builtins.px");
    if (!builtins_path.empty() && processed_files.find(builtins_path) == processed_files.end()) {
        processed_files.insert(builtins_path);
        load_and_parse_file(builtins_path, program);
    }

    while (current_token().type != TokenType::EndOfFile) {

        // #use "file.px" import and parse another Pixel source file
        if (current_token().type == TokenType::Use) {
            advance(); // consume 'use'

            if (current_token().type != TokenType::String) {
                error("#use expects a file path string");
                advance(); // skip the bad token
                continue;
            }
            std::string lib_name = current_token().value;
            advance(); // consume the path string

            std::string full_path = find_library(lib_name);
            if (full_path.empty()) {
                error("could not find '" + lib_name + "'");
                continue;
            }

            // Guard against circular imports (A uses B, B uses A)
            if (processed_files.find(full_path) == processed_files.end()) {
                processed_files.insert(full_path);
                load_and_parse_file(full_path, program);
            }
            continue;
        }

        if (auto stmt = parse_statement()) {
            program.push_back(std::move(stmt));
        } else {
            // If parse_statement returns nullptr then we had an error
            // lets try to recover by skipping to the next thing
            while (current_token().type != TokenType::EndOfFile &&
                   current_token().type != TokenType::Rbrace) { // i'd put a semicolon here if i actually USED them
                // Let's just skip to the next statement start
                if (current_token().type == TokenType::Return ||
                    current_token().type == TokenType::If ||
                    current_token().type == TokenType::While ||
                    current_token().type == TokenType::FuncDefine ||
                    current_token().type == TokenType::Struct) {
                    break;
                }
                advance();
            }
        }
    }

    return program;
}

// one statement at current pos

std::unique_ptr<ASTNode> Parser::parse_funccall(std::string name) {
    advance(); // consume '('
    auto call_node = std::make_unique<FunctionCallNode>();
    call_node->name = name;
    call_node->is_statement = true;

    while (current_token().type != TokenType::Rparen) {
        call_node->arguments.push_back(parse_expression());
        if (current_token().type == TokenType::Comma) advance();
    }
    expect(TokenType::Rparen);
    return call_node;
}

std::unique_ptr<ASTNode> Parser::parse_statement() {

    if (current_token().type == TokenType::At) {
        advance(); // consume '@'
        auto target = parse_expression(); // the pointer expr
        expect(TokenType::At);
        expect(TokenType::Equals);
        auto deref_assign = std::make_unique<DerefAssignNode>();
        deref_assign->target = std::move(target);
        deref_assign->expression = parse_expression();
        return deref_assign;
    }

    // fn name(...) -> Type { ... }
    if (current_token().type == TokenType::FuncDefine) {
        return parse_function_definition();
    }

    if (current_token().type == TokenType::Struct) {
        return parse_struct_definition();
    }

    // return <expression>
    if (current_token().type == TokenType::Return) {
        advance(); // consume 'return'
        auto ret = std::make_unique<ReturnNode>();
        ret->expression = parse_expression();
        return ret;
    }

    // while (<condition>) { ... }
    if (current_token().type == TokenType::While) {
        advance(); // consume 'while'
        auto while_node = std::make_unique<WhileNode>();

        // expect(TokenType::Lparen);
        while_node->condition = parse_expression();
        // expect(TokenType::Rparen);

        expect(TokenType::Lbrace);
        while (current_token().type != TokenType::Rbrace &&
               current_token().type != TokenType::EndOfFile) {
            if (auto stmt = parse_statement())
                while_node->body.push_back(std::move(stmt));
        }
        expect(TokenType::Rbrace);

        return while_node;
    }

    // if (<condition>) { ... } and else and the other thing
    if (current_token().type == TokenType::If) {
        TokenType t = current_token().type;

        advance(); // consume 'if'
        auto if_node = std::make_unique<IfNode>();

        // parse main block
        // expect(TokenType::Lparen);
        if_node->condition = parse_expression();
        // expect(TokenType::Rparen);

        expect(TokenType::Lbrace);
        while (current_token().type != TokenType::Rbrace &&
            current_token().type != TokenType::EndOfFile) {
            if (auto stmt = parse_statement())
                if_node->then_block.push_back(std::move(stmt));
        }
        expect(TokenType::Rbrace);

        // Parse any elif blocks
        while (current_token().type == TokenType::Elif) {
            advance(); // consume 'elif'

            // expect(TokenType::Lparen); //cnsume ')'
            auto elif_condition = parse_expression();
            // expect(TokenType::Rparen); // consume ')'
            
            expect(TokenType::Lbrace);
            std::vector<std::unique_ptr<ASTNode>> elif_body;
            while (current_token().type != TokenType::Rbrace &&
                current_token().type != TokenType::EndOfFile) {
                if (auto stmt = parse_statement())
                    elif_body.push_back(std::move(stmt));
            }
            expect(TokenType::Rbrace);
            
            if_node->elif_blocks.push_back(std::make_pair(std::move(elif_condition), std::move(elif_body)));
        }

        // Parse optional else block
        if (current_token().type == TokenType::Else) {
            advance(); // consume 'else'
            
            expect(TokenType::Lbrace);
            while (current_token().type != TokenType::Rbrace &&
                current_token().type != TokenType::EndOfFile) {
                if (auto stmt = parse_statement())
                    if_node->else_block.push_back(std::move(stmt));
            }
            expect(TokenType::Rbrace);
        }

        return if_node;
    }

    // Int x = ....  or  String x = ...  /  Array(Int) x = ...  /  Int* x  etc etc
    if (is_type_keyword(current_token().type)) {
        TypeInfo type_info = parse_type(); // consumes the type tokens
        return parse_declaration(type_info);
    }

 // Identifier it can do a lot of stuff
    if (current_token().type == TokenType::Identifier) {
        // MyStruct var_name struct type followed by a variable name
        if (known_structs.count(current_token().value) &&
            pos + 1 < tokens.size() &&
            tokens[pos + 1].type == TokenType::Identifier) {

            TypeInfo type_info = parse_type(); // consumes struct name
            return parse_declaration(type_info);
        }

        std::string name = current_token().value;
        advance();

        // name(...) function call as a statement
        if (current_token().type == TokenType::Lparen) {
            return parse_funccall(name);
        }

        // name->field = expr  struct field assignment
        if (current_token().type == TokenType::Rarrow) {
            advance(); // consume '->'

            if (current_token().type != TokenType::Identifier) {
                error("expected field name after '->'");
                return nullptr;
            }
            std::string field = current_token().value;
            advance(); // consume field name

            expect(TokenType::Equals);

            auto arrow_assign = std::make_unique<ArrowAssignNode>();
            arrow_assign->left       = name;
            arrow_assign->right      = field;
            arrow_assign->expression = parse_expression();
            return arrow_assign;
        }

        // name = expr reassignment (variable must already be declared)
        expect(TokenType::Equals);

        if (declared_vars.find(name) == declared_vars.end()) {
            error("variable '" + name + "' used before declaration");
            // create an assign node anyway to consume the expression
            auto assign = std::make_unique<AssignNode>();
            assign->identifier = name;
            assign->is_declaration = false;
            assign->expression = parse_expression();
            return assign;
        }

        auto assign = std::make_unique<AssignNode>();
        assign->identifier     = name;
        assign->is_declaration = false;
        assign->expression     = parse_expression();
        return assign;
    }

    // Unknown token so skip it so we don't infinite-loop
    advance();
    return nullptr;
}

// parse_type parses any type annotation, returns a filled TypeInfo

TypeInfo Parser::parse_type() {
    TypeInfo info;

    if (current_token().type == TokenType::ArrayKeyword) {
        advance(); // consume 'Array'
        expect(TokenType::Lbracket);

        if (!is_type_keyword(current_token().type)) {
            error("expected element type inside Array(...)");
            // return a default type
            info.base_type = TokenType::IntKeyword;
            info.is_array = true;
            // skip the bad token
            advance();
            expect(TokenType::Rparen);
            return info;
        }
        info.base_type = current_token().type;
        info.is_array  = true;

        advance(); // consume element type

        expect(TokenType::Rbracket);

        if (current_token().type == TokenType::Mult) {
            info.is_pointer = true;
            advance(); // consume '*'
        }
    }
    else if (is_type_keyword(current_token().type)) {
        info.base_type = current_token().type;
        advance(); // consume type keyword

        // Optional pointer modifier
        if (current_token().type == TokenType::Mult) {
            info.is_pointer = true;
            advance(); // consume '*'
        }
    }
    else if (current_token().type == TokenType::Identifier &&
             known_structs.count(current_token().value)) {
        info.base_type   = TokenType::Identifier;
        info.struct_name = current_token().value;
        advance(); // consume struct name
    }
    else {
        error("expected a type, got '" + current_token().value + "'");
        // return a default type and skip the bad token
        info.base_type = TokenType::IntKeyword;
        advance();
    }

    return info;
}

std::unique_ptr<ASTNode> Parser::parse_declaration(TypeInfo type_info) {
    // Variable name
    bool is_const = false;
    if (current_token().type == TokenType::Const) {
        is_const = true;
        advance(); //consume const
    }

    if (current_token().type != TokenType::Identifier) {
        error("expected variable name after type");
        return nullptr;
    }

    auto assign = std::make_unique<AssignNode>();
    assign->is_declaration = true;
    assign->type_info      = type_info;
    assign->is_const = is_const;

    std::string var_name = current_token().value;
    assign->identifier   = var_name;
    advance(); // consume variable name

    if (current_token().type == TokenType::Equals) {
        advance(); // consume '='
        assign->expression = parse_expression();
        assign->initialized = true;
    } else {
        assign->initialized = false;
    }

    declared_vars.insert(var_name);
    return assign;
}

// called when fn is current token
std::unique_ptr<ASTNode> Parser::parse_function_definition() {
    advance(); // consume 'fn'

    if (current_token().type != TokenType::Identifier) {
        error("expected function name after 'fn'");
        return nullptr;
    }
    auto func_decl = std::make_unique<FunctionDeclNode>();
    func_decl->name = current_token().value;
    advance(); // consume function name

    expect(TokenType::Lparen);
    func_decl->parameters = parse_parameters();
    expect(TokenType::Rparen);

    if (current_token().type == TokenType::Rarrow) {
        advance();

        if (!is_type_keyword(current_token().type)) {
            error("expected return type after '->'");
            // set a default return type
            func_decl->return_type.base_type = TokenType::VoidKeyword;
        }

        func_decl->return_type = parse_type();
    } else {
        func_decl->return_type.base_type = TokenType::VoidKeyword;
    }

    //expect(TokenType::Rarrow);

    expect(TokenType::Lbrace);
    while (current_token().type != TokenType::Rbrace &&
           current_token().type != TokenType::EndOfFile) {
        if (auto stmt = parse_statement())
            func_decl->body_block.push_back(std::move(stmt));
    }
    expect(TokenType::Rbrace);

    return func_decl;
}

std::unique_ptr<ASTNode> Parser::parse_struct_definition() {
    advance(); // consume 'struct'

    if (current_token().type != TokenType::Identifier) {
        error("expected struct name");
        return nullptr;
    }

    auto _struct = std::make_unique<StructDeclNode>();
    _struct->name = current_token().value;
    advance(); // consume struct name

    // Register so parse_type() and parse_statement() recognise this as a type
    known_structs.insert(_struct->name);

    expect(TokenType::Lbrace);

    // Each line inside the struct is:  Type fieldName
    // (no initializers, no reassignment pure field declarations)
    while (current_token().type != TokenType::Rbrace &&
           current_token().type != TokenType::EndOfFile) {

        // Parse field type
        TypeInfo field_type = parse_type();

        if (current_token().type != TokenType::Identifier) {
            error("expected field name in struct");
            // skip the bad token
            advance();
            continue;
        }
        std::string field_name = current_token().value;
        advance(); // consume field name

        auto field = std::make_unique<AssignNode>();
        field->identifier    = field_name;
        field->is_declaration = true;
        field->initialized   = false;
        field->type_info     = field_type;
        _struct->contents.push_back(std::move(field));
    }

    expect(TokenType::Rbrace);
    return _struct;
}

// parse func params
std::vector<Parameter> Parser::parse_parameters() {
    std::vector<Parameter> params;
    while (current_token().type != TokenType::Rparen &&
           current_token().type != TokenType::EndOfFile) {

        if (!is_type_keyword(current_token().type) &&
            !(current_token().type == TokenType::Identifier &&
              known_structs.count(current_token().value))) {
            error("expected parameter type, got '" + current_token().value + "'");
            // skip to comma or ')'
            while (current_token().type != TokenType::Comma &&
                   current_token().type != TokenType::Rparen &&
                   current_token().type != TokenType::EndOfFile) {
                advance();
            }
            if (current_token().type == TokenType::Comma) {
                advance();
                continue;
            }
            break;
        }

        TypeInfo type_info = parse_type();

        if (current_token().type != TokenType::Identifier) {
            error("expected parameter name, got '" + current_token().value + "'");
            // skip to comma or ')'
            while (current_token().type != TokenType::Comma &&
                   current_token().type != TokenType::Rparen &&
                   current_token().type != TokenType::EndOfFile) {
                advance();
            }
            if (current_token().type == TokenType::Comma) {
                advance();
                continue;
            }
            break;
        }

        std::string param_name = current_token().value;
        advance(); // consume parameter name

        Parameter param;
        param.name      = param_name;
        param.type_info = type_info;
        params.push_back(std::move(param));

        if (current_token().type == TokenType::Comma) advance();
    }

    return params;
}

// parse_expression  returns a single expression node. true/false, sting literal, float, number, identifier, array, etc etc

std::unique_ptr<ASTNode> Parser::parse_expression() {
    std::unique_ptr<ASTNode> left;

    bool is_unary_minus = false;
    bool is_unary_not = false;
    
    if (current_token().type == TokenType::Minus) {
        is_unary_minus = true;
        advance(); // consume '-'
    } else if (current_token().type == TokenType::ExclamationMark) {
        is_unary_not = true;
        advance(); // consume '!'
    }

    if (current_token().type == TokenType::Lparen) {
        advance();
        auto exp = parse_expression();
        expect(TokenType::Rparen);
        
        // put in a grouping node because bracket
        auto grouping = std::make_unique<GroupingNode>();
        grouping->expression = std::move(exp);
        left = std::move(grouping);
    } else if (current_token().type == TokenType::True) {
        auto lit = std::make_unique<LiteralNode>();
        lit->value = "true";
        advance();
        left = std::move(lit);
    }
    else if (current_token().type == TokenType::False) {
        auto lit = std::make_unique<LiteralNode>();
        lit->value = "false";
        advance();
        left = std::move(lit);
    } else if (current_token().type == TokenType::NullPtr) {
        auto lit = std::make_unique<LiteralNode>();
        lit->value = "nullptr";
        advance();
        left = std::move(lit);
    } else if (current_token().type == TokenType::String) {
        auto lit = std::make_unique<LiteralNode>();
        lit->value = "\"" + current_token().value + "\"";
        advance();
        left = std::move(lit);
    }
    else if (current_token().type == TokenType::Float) {
        auto lit = std::make_unique<LiteralNode>();
        lit->value = current_token().value;
        advance();
        left = std::move(lit);
    }
    else if (current_token().type == TokenType::Number) {
        auto lit = std::make_unique<LiteralNode>();
        lit->value = current_token().value;
        advance();
        left = std::move(lit);
    }
    else if (current_token().type == TokenType::At) {
        advance(); // consume '@'
        auto target = parse_expression();
        expect(TokenType::At);
        auto deref = std::make_unique<DereferenceNode>();
        deref->target = std::move(target);
        left = std::move(deref);
    }
	else if (current_token().type == TokenType::Identifier || is_type_keyword(current_token().type)) {
        std::string id = current_token().value;
        advance();

        if (current_token().type == TokenType::Lparen) {
            // Function call expression: name(arg, arg, ...)
            advance(); // consume '('
            auto call = std::make_unique<FunctionCallNode>();
            call->name = id;

            while (current_token().type != TokenType::Rparen &&
                   current_token().type != TokenType::EndOfFile) {
                call->arguments.push_back(parse_expression());
                if (current_token().type == TokenType::Comma) advance();
            }
            expect(TokenType::Rparen);
            left = std::move(call);
        } else if (current_token().type == TokenType::Rarrow) {
            advance(); //consume arrow

            auto arrow = std::make_unique<ArrowNode>();
            arrow->left = id;
            arrow->right = current_token().value;

            advance(); //consume right value

            left = std::move(arrow);
        } else if (current_token().type == TokenType::Lbracket) {
            // Array index access: name[index]
            advance(); // consume '['
            auto access = std::make_unique<IndexAccessNode>();
            access->target = id;
            access->index  = parse_expression();
            expect(TokenType::Rbracket);
            left = std::move(access);
        }
        else {
            // Plain variable reference
            auto lit = std::make_unique<LiteralNode>();
            lit->value = id;
            left = std::move(lit);
        }
    }
    else if (current_token().type == TokenType::Lbracket) {
        // Array literal: [elem, elem, ...]
        advance(); // consume '['
        auto array_lit = std::make_unique<ArrayLiteralNode>();

        while (current_token().type != TokenType::Rbracket &&
               current_token().type != TokenType::EndOfFile) {
            array_lit->elements.push_back(parse_expression());
            if (current_token().type == TokenType::Comma) advance();
        }
        expect(TokenType::Rbracket);
        left = std::move(array_lit);
    }
    else {
        // Unrecognised token - return an empty literal to avoid a null left
        error("unexpected token in expression: '" + current_token().value + "'");
        left = std::make_unique<LiteralNode>();
        advance(); // skip the bad token
    }

    // unary ops
    if (is_unary_minus) {
        // Create 0,left
        auto zero = std::make_unique<LiteralNode>();
        zero->value = "0";
        auto bin = std::make_unique<BinOpNode>();
        bin->op = "-";
        bin->left = std::move(zero);
        bin->right = std::move(left);
        left = std::move(bin);
    } else if (is_unary_not) {
        auto false_lit = std::make_unique<LiteralNode>();
        false_lit->value = "false";
        auto bin = std::make_unique<BinOpNode>();
        bin->op = "==";
        bin->left = std::move(left);
        bin->right = std::move(false_lit);
        left = std::move(bin);
    }


    // Binary operator: wrap left and right in a BinOpNode
    if (current_token().type == TokenType::Plus        ||
        current_token().type == TokenType::Minus       ||
        current_token().type == TokenType::Mult        ||
        current_token().type == TokenType::Div         ||
        current_token().type == TokenType::GreaterThan ||
        current_token().type == TokenType::LessThan    ||
        current_token().type == TokenType::LessThanEqualTo    ||
        current_token().type == TokenType::GreaterThanEqualTo    ||
        current_token().type == TokenType::NotEquals   ||
        current_token().type == TokenType::And         ||
        current_token().type == TokenType::Or          ||
        current_token().type == TokenType::DoubleEquals)
    {
        auto bin_op = std::make_unique<BinOpNode>();
        bin_op->op    = current_token().value;

        if (current_token().type == TokenType::And) {
            bin_op->op = "&&";
        } else if (current_token().type == TokenType::Or) {
            bin_op->op = "||";
        }

        bin_op->left  = std::move(left);
        advance(); // consume operator
        bin_op->right = parse_expression(); // right side parsed recursively
        return bin_op;
    }

    return left;
}