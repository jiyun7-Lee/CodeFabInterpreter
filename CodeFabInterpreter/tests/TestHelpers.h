#pragma once
#include <gtest/gtest.h>
#include <stdexcept>
#include "../Token.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"

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

// ================================================================
// ExecutorFixture — Tokenizer / Parser / Executor 기반 테스트 공통 베이스
// ArrayFixture, FunctionFixture 등 실행기 테스트에서 상속하여 사용한다.
// ================================================================
class ExecutorFixture : public ::testing::Test
{
protected:
    std::string runSource(const std::string& source)
    {
        Tokenizer tz;
        auto tokens = tz.tokenize(source);
        Parser parser;
        auto stmts = parser.parse(tokens);
        Executor executor;
        testing::internal::CaptureStdout();
        executor.execute(stmts);
        return testing::internal::GetCapturedStdout();
    }

    void runSourceExpectThrow(const std::string& source)
    {
        Tokenizer tz;
        auto tokens = tz.tokenize(source);
        Parser parser;
        auto stmts = parser.parse(tokens);
        Executor executor;
        ASSERT_THROW(executor.execute(stmts), std::runtime_error);
    }
};
