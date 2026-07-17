#include "Lexer.h"
#include <cctype>
#include <iostream>

// Lexer::Lexer(std::string source_code) : src(source_code), index(0) {
//     current_char = src.empty() ? '\0' : src[0];
// }

Lexer::Lexer(std::string source_code, std::string fname) 
    : src(source_code), index(0), filename(fname) {
    current_char = src.empty() ? '\0' : src[0];
}

// void Lexer::advance() {
//     index++;
//     current_char = (index < src.length()) ? src[index] : '\0';
// }

void Lexer::advance() {
    if (current_char == '\n') {
        line++;
        column = 0;
    }
    index++;
    column++;
    current_char = (index < src.length()) ? src[index] : '\0';
}

void Lexer::skip_whitespace() {
    while (current_char != '\0' && std::isspace(current_char))
        advance();
}

void Lexer::skip_line_comment() {
    // Skip everything until end of line
    while (current_char != '\0' && current_char != '\n')
        advance();
}

void Lexer::skip_block_comment() {
    // Skip everything until */
    while (current_char != '\0') {
        if (current_char == '*' && index + 1 < src.length() && src[index + 1] == '/') {
            advance(); // consume '*'
            advance(); // consume '/'
            return;
        }
        advance();
    }
}

Token Lexer::make_token(TokenType type, const std::string& value) {
    Token tok;
    tok.type = type;
    tok.value = value;
    tok.location.filename = filename;
    tok.location.line = line;
    tok.location.column = column;
    return tok;
}

Token Lexer::parse_number() {
    std::string result;
    bool is_float = false;
    
    SourceLocation loc;
    loc.filename = filename;
    loc.line = line;
    loc.column = column;

    while (current_char != '\0' && (std::isdigit(current_char) || current_char == '.')) {
        if (current_char == '.') {
            if (is_float) break;
            is_float = true;
        }
        result += current_char;
        advance();
    }

    Token tok = { is_float ? TokenType::Float : TokenType::Number, result };
    tok.location = loc;
    return tok;
}

Token Lexer::parse_identifier() {
    std::string result;
    
    SourceLocation loc;
    loc.filename = filename;
    loc.line = line;
    loc.column = column;
    
    while (current_char != '\0' && (std::isalnum(current_char) || current_char == '_')) {
        result += current_char;
        advance();
    }

    Token tok;
    tok.location = loc;

    // Map keywords
    if (result == "Int")    { tok.type = TokenType::IntKeyword; tok.value = result; return tok; }
    if (result == "Float")  { tok.type = TokenType::FloatKeyword; tok.value = result; return tok; }
    if (result == "String") { tok.type = TokenType::StringKeyword; tok.value = result; return tok; }
    if (result == "const")  { tok.type = TokenType::Const; tok.value = result; return tok; }
    if (result == "Bool")   { tok.type = TokenType::BoolKeyword; tok.value = result; return tok; }
    if (result == "Void")   { tok.type = TokenType::VoidKeyword; tok.value = result; return tok; }
    if (result == "Array")  { tok.type = TokenType::ArrayKeyword; tok.value = result; return tok; }
    if (result == "Auto")   { tok.type = TokenType::AutoKeyword; tok.value = result; return tok; }
    if (result == "nullptr"){ tok.type = TokenType::NullPtr; tok.value = result; return tok; }
    if (result == "true")   { tok.type = TokenType::True; tok.value = result; return tok; }
    if (result == "false")  { tok.type = TokenType::False; tok.value = result; return tok; }
    if (result == "return") { tok.type = TokenType::Return; tok.value = result; return tok; }
    if (result == "fn")     { tok.type = TokenType::FuncDefine; tok.value = result; return tok; }
    if (result == "and")    { tok.type = TokenType::And; tok.value = result; return tok; }
    if (result == "or")     { tok.type = TokenType::Or; tok.value = result; return tok; }
    if (result == "while")  { tok.type = TokenType::While; tok.value = result; return tok; }
    if (result == "elif")    { tok.type = TokenType::Elif; tok.value = result; return tok; }
    if (result == "break")    { tok.type = TokenType::Break; tok.value = result; return tok; }
    if (result == "continue")    { tok.type = TokenType::Continue; tok.value = result; return tok; }
    if (result == "if")    { tok.type = TokenType::If; tok.value = result; return tok; }
    if (result == "else")    { tok.type = TokenType::Else; tok.value = result; return tok; }
    if (result == "struct") { tok.type = TokenType::Struct; tok.value = result; return tok; }

    tok.type = TokenType::Identifier;
    tok.value = result;
    return tok;
}

Token Lexer::parse_string() {
    advance(); // consume opening "
    std::string result;
    
    SourceLocation loc;
    loc.filename = filename;
    loc.line = line;
    loc.column = column;
    
    while (current_char != '\0' && current_char != '"') {
        result += current_char;
        advance();
    }
    advance(); // consume closing "
    
    Token tok;
    tok.type = TokenType::String;
    tok.value = result;
    tok.location = loc;
    return tok;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (current_char != '\0') {
        // Whitespace
        if (std::isspace(current_char)) { skip_whitespace(); continue; }

        // Comments
        if (current_char == '/' && index + 1 < src.length()) {
            if (src[index + 1] == '/') { advance(); advance(); skip_line_comment();  continue; }
            if (src[index + 1] == '*') { advance(); advance(); skip_block_comment(); continue; }
        }

        // Numbers
        if (std::isdigit(current_char)) { tokens.push_back(parse_number()); continue; }

        // Identifiers and keywords
        if (std::isalpha(current_char) || current_char == '_') {
            Token t = parse_identifier();
            tokens.push_back(t);
            continue;
        }

        // Strings
        if (current_char == '"') { tokens.push_back(parse_string()); continue; }

        // Directives: #bind / #use
        if (current_char == '#') {
            advance(); // consume '#'
            if (std::isalpha(current_char)) {
                Token directive = parse_identifier();
                if (directive.value == "use")  tokens.push_back(make_token(TokenType::Use, "use"));
                else std::cerr << filename << ":" << line << ":" << column 
                               << " lexer error: unknown directive '#" << directive.value << "'\n";
            }
            continue;
        }

        // Single/double-character operators and punctuation
        switch (current_char) {
            case '+': tokens.push_back(make_token(TokenType::Plus, "+")); break;
            case '*': tokens.push_back(make_token(TokenType::Mult, "*")); break;
            case '/': tokens.push_back(make_token(TokenType::Div, "/")); break;
            case '(': tokens.push_back(make_token(TokenType::Lparen, "(")); break;
            case ')': tokens.push_back(make_token(TokenType::Rparen, ")")); break;
            case '{': tokens.push_back(make_token(TokenType::Lbrace, "{")); break;
            case '@': tokens.push_back(make_token(TokenType::At, "@")); break;
            case '}': tokens.push_back(make_token(TokenType::Rbrace, "}")); break;
            case '[': tokens.push_back(make_token(TokenType::Lbracket, "[")); break;
            case ']': tokens.push_back(make_token(TokenType::Rbracket, "]")); break;

            case '>':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back(make_token(TokenType::GreaterThanEqualTo, ">="));
                    advance(); // consume '='
                } else {
                    tokens.push_back(make_token(TokenType::GreaterThan, ">"));
                }
                break;
            
            case '<':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back(make_token(TokenType::LessThanEqualTo, "<="));
                    advance(); // consume '='
                } else {
                    tokens.push_back(make_token(TokenType::LessThan, "<"));
                }
                break;

            case '!':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back(make_token(TokenType::NotEquals, "!="));
                    advance(); // consume '='
                } else {
                    tokens.push_back(make_token(TokenType::ExclamationMark, "!"));
                }
                break;
            
            case ',': tokens.push_back(make_token(TokenType::Comma, ",")); break;

            case ':': tokens.push_back(make_token(TokenType::Colon, ":")); break;

            case '-':
                if (index + 1 < src.length() && src[index + 1] == '>') {
                    tokens.push_back(make_token(TokenType::Rarrow, "->"));
                    advance(); // consume '>'
                } else {
                    tokens.push_back(make_token(TokenType::Minus, "-"));
                }
                break;

            case '=':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back(make_token(TokenType::DoubleEquals, "=="));
                    advance(); // consume second '='
                } else {
                    tokens.push_back(make_token(TokenType::Equals, "="));
                }
                break;

            default:
                std::cerr << filename << ":" << line << ":" << column 
                          << " lexer error: unknown character '" << current_char << "'\n";
                break;
        }

        advance();
    }

    tokens.push_back(make_token(TokenType::EndOfFile, ""));
    return tokens;
}