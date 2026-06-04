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
    std::string lexeme;   // 소스코드 원문 텍스트 (예: 변수명 "age", 연산자 "+", 숫자 "37")
    int         line;
    Value       literal;  // 계산된 값 — 숫자/문자열/bool 리터럴에만 사용 (그 외 monostate)
};
