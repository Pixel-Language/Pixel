#pragma once
#include <string>

enum class TokenType {
    // Literals
    Identifier,
    Number,
    Float,
    String,
    True,
    False,

    // Type keywords
    IntKeyword,
    StringKeyword,
    BoolKeyword,
    FloatKeyword,
    VoidKeyword,
    ArrayKeyword,
    AutoKeyword,
    NullPtr,

    // Operators
    Equals,
    NotEquals, //do this
    ExclamationMark,
    DoubleEquals,
    Plus,
    Minus,
    Mult,
    Div,
    LessThan,
    LessThanEqualTo, //do this
    GreaterThan,
    GreaterThanEqualTo, //do this

    // Punctuation
    Lparen,
    Rparen,
    Lbrace,
    Rbrace,
    Lbracket,
    Rbracket,
    Rarrow,
    Comma,
    Colon,
    At,

    // Keywords
    If,
    Unless,
    While,
    Return,
    FuncDefine,
    Ext,
    RawExtCode,
    Struct,

    // Directives
    Bind,
    Use,

    EndOfFile
};

struct Token {
    TokenType type;
    std::string value;
};

// Holds full type info for any variable, parameter, or field
struct TypeInfo {
    TokenType   base_type   = TokenType::IntKeyword;
    bool        is_array    = false;
    bool        is_pointer  = false;
    std::string struct_name = "";   // non-empty when base_type == Identifier
};