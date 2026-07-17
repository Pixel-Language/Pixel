#pragma once
#include <string>
#include <vector>
#include "Token.h"

class Lexer {
public:
    Lexer(std::string source_code, std::string filename = "<input>");
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t      index;
    char        current_char;

    std::string filename;
    int line = 1;
    int column = 1;

    void  advance();
    void  skip_whitespace();
    void  skip_line_comment();
    void  skip_block_comment();
    Token parse_number();
    Token parse_identifier();
    Token parse_string();
    Token make_token(TokenType type, const std::string& value);
};