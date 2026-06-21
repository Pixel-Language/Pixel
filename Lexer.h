#pragma once
#include <string>
#include <vector>
#include "Token.h"

class Lexer {
public:
    Lexer(std::string source_code);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t      index;
    char        current_char;

    void  advance();
    void  skip_whitespace();
    void  skip_line_comment();
    void  skip_block_comment();
    Token parse_number();
    Token parse_identifier();
    Token parse_string();
};