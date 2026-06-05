#pragma once
#include "../Token.h"

// FunctionTest, ArrayTest 등 파서/실행기 단위 테스트에서 공유하는 토큰 생성 헬퍼.
// static 대신 inline 을 사용하여 각 번역 단위에 복사본이 생기지 않도록 한다.

inline Token tok(TokenType type, const std::string& lexeme = "")
{
    Token t; t.type = type; t.lexeme = lexeme; t.line = 1; t.literal = {};
    return t;
}

inline Token numTok(double val)
{
    Token t; t.type = TokenType::NUMBER; t.lexeme = std::to_string(val);
    t.line = 1; t.literal = val;
    return t;
}

inline Token eof() { return tok(TokenType::EOF_TOKEN); }
