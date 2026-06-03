#pragma once
#include <string>
#include "Value.h"

enum class TokenType
{
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, SEMICOLON,
    PLUS, MINUS, STAR, SLASH,
    BANG,
    EQUAL, GREATER, LESS,
    IDENTIFIER, STRING, NUMBER,
    VAR, PRINT,
    IF, ELSE,
    FOR,
    TRUE, FALSE,
    AND, OR,
    EOF_TOKEN
};

struct Token
{
    TokenType   type;
    std::string lexeme;
    int         line;
    Value       literal;
};
