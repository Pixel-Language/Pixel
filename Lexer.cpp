#include "Lexer.h"
#include <cctype>
#include <iostream>

Lexer::Lexer(std::string source_code) : src(source_code), index(0) {
    current_char = src.empty() ? '\0' : src[0];
}

void Lexer::advance() {
    index++;
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

Token Lexer::parse_number() {
    std::string result;
    bool is_float = false;

    while (current_char != '\0' && (std::isdigit(current_char) || current_char == '.')) {
        if (current_char == '.') {
            if (is_float) break; // second dot,  stop
            is_float = true;
        }
        result += current_char;
        advance();
    }

    return { is_float ? TokenType::Float : TokenType::Number, result };
}

Token Lexer::parse_identifier() {
    std::string result;
    while (current_char != '\0' && (std::isalnum(current_char) || current_char == '_')) {
        result += current_char;
        advance();
    }

    // Map keywords
    if (result == "Int")    return { TokenType::IntKeyword,    result };
    if (result == "Float")  return { TokenType::FloatKeyword,  result };
    if (result == "String") return { TokenType::StringKeyword, result };
    if (result == "Bool")   return { TokenType::BoolKeyword,   result };
    if (result == "Void")   return { TokenType::VoidKeyword,   result };
    if (result == "Array")  return { TokenType::ArrayKeyword,  result };
    if (result == "Auto")  return { TokenType::AutoKeyword,  result };
    if (result == "nullptr")   return { TokenType::NullPtr,    result };
    if (result == "true")   return { TokenType::True,          result };
    if (result == "false")  return { TokenType::False,         result };
    if (result == "return") return { TokenType::Return,        result };
    if (result == "fn")     return { TokenType::FuncDefine,    result };
    if (result == "if")     return { TokenType::If,            result };
    if (result == "unless") return { TokenType::Unless,        result };
    if (result == "while")  return { TokenType::While,        result };
    if (result == "ext")    return { TokenType::Ext,           result };
    if (result == "struct") return { TokenType::Struct,        result };

    return { TokenType::Identifier, result };
}

Token Lexer::parse_string() {
    advance(); // consume opening "
    std::string result;
    while (current_char != '\0' && current_char != '"') {
        result += current_char;
        advance();
    }
    advance(); // consume closing "
    return { TokenType::String, result };
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

            // parser never has to deal with raw C syntax inside the braces
            if (t.type == TokenType::Ext) {
                skip_whitespace();
                if (current_char == '{') {
                    tokens.push_back({ TokenType::Lbrace, "{" });
                    advance(); // consume '{'

                    std::string raw_c;
                    int depth = 1;
                    while (current_char != '\0' && depth > 0) {
                        if      (current_char == '{') depth++;
                        else if (current_char == '}') depth--;
                        if (depth > 0) { raw_c += current_char; advance(); }
                    }

                    tokens.push_back({ TokenType::RawExtCode, raw_c });
                    tokens.push_back({ TokenType::Rbrace, "}" });
                    advance(); // consume closing '}'
                }
            }
            continue;
        }

        // Strings
        if (current_char == '"') { tokens.push_back(parse_string()); continue; }

        // Directives: #bind / #use
        if (current_char == '#') {
            advance(); // consume '#'
            if (std::isalpha(current_char)) {
                Token directive = parse_identifier();
                if      (directive.value == "bind") tokens.push_back({ TokenType::Bind, "bind" });
                else if (directive.value == "use")  tokens.push_back({ TokenType::Use,  "use"  });
                else std::cerr << "lexer error: unknown directive '#" << directive.value << "'\n";
            }
            continue;
        }

        // Single/double-character operators and punctuation
        switch (current_char) {
            case '+': tokens.push_back({ TokenType::Plus,         "+"  }); break;
            case '*': tokens.push_back({ TokenType::Mult,         "*"  }); break;
            case '/': tokens.push_back({ TokenType::Div,          "/"  }); break;
            case '(': tokens.push_back({ TokenType::Lparen,       "("  }); break;
            case ')': tokens.push_back({ TokenType::Rparen,       ")"  }); break;
            case '{': tokens.push_back({ TokenType::Lbrace,       "{"  }); break;
            case '@': tokens.push_back({ TokenType::At,           "@"  }); break;
            case '}': tokens.push_back({ TokenType::Rbrace,       "}"  }); break;
            case '[': tokens.push_back({ TokenType::Lbracket,     "["  }); break;
            case ']': tokens.push_back({ TokenType::Rbracket,     "]"  }); break;

            case '>':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back({ TokenType::GreaterThanEqualTo, ">=" });
                    advance(); // consume '='
                } else {
                    tokens.push_back({ TokenType::GreaterThan, ">" });
                }
                break;
            
            case '<':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back({ TokenType::LessThanEqualTo, "<=" });
                    advance(); // consume '='
                } else {
                    tokens.push_back({ TokenType::LessThan, "<" });
                }
                break;

            case '!':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back({ TokenType::NotEquals, "!=" });
                    advance(); // consume '='
                } else {
                    tokens.push_back({ TokenType::ExclamationMark, "!" });
                }
                break;
            
            case ',': tokens.push_back({ TokenType::Comma,        ","  }); break;
            case ':': tokens.push_back({ TokenType::Colon,        ":"  }); break;

            case '-':
                if (index + 1 < src.length() && src[index + 1] == '>') {
                    tokens.push_back({ TokenType::Rarrow, "->" });
                    advance(); // consume '>'
                } else {
                    tokens.push_back({ TokenType::Minus, "-" });
                }
                break;

            case '=':
                if (index + 1 < src.length() && src[index + 1] == '=') {
                    tokens.push_back({ TokenType::DoubleEquals, "==" });
                    advance(); // consume second '='
                } else {
                    tokens.push_back({ TokenType::Equals, "=" });
                }
                break;

            default:
                std::cerr << "lexer error: unknown character '" << current_char << "'\n";
                break;
        }

        advance();
    }

    tokens.push_back({ TokenType::EndOfFile, "" });
    return tokens;
}