#pragma once
#include <string>

struct SourceLocation {
    std::string filename;
    int line;
    int column;
    
    std::string to_string() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

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
    Modulo,
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
    Else,
    Elif,
    And,
    Or,
    While,
    Return,
    FuncDefine,
    Struct,
    Const,
    Break,
    Continue,

    // Directives
    Use,

    EndOfFile
};

struct Token {
    TokenType type;
    std::string value;
    SourceLocation location;
};

// Holds full type info for any variable, parameter, or field
struct TypeInfo {
    TokenType   base_type   = TokenType::IntKeyword;
    bool        is_array    = false;
    bool        is_pointer  = false;
    std::string struct_name = "";   // non-empty when base_type == Identifier

    TypeInfo() = default;
    TypeInfo(TokenType t) : base_type(t) {}
};