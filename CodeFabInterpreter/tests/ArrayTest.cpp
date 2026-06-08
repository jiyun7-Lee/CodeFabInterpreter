#include "gtest/gtest.h"
#include "TestHelpers.h"
#include "../Parser.h"
#include "../Executor.h"
#include "../Tokenizer.h"
#include "../Stmt.h"
#include "../Expr.h"

// ================================================================
// Fixture
// ================================================================
class ArrayFixture : public ::testing::Test
{
protected:
    // 소스 문자열 파싱 후 실행 → stdout 반환
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

    // 소스 문자열 파싱 후 실행 → runtime_error 기대
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

// ================================================================
// 파싱 테스트 (TC-AR-01~02, TC-AR-13)
// ================================================================

// TC-AR-01: [1, 2, 3]; → ArrayLiteralExpr, elements.size()==3, elements[0]==1.0
TEST_F(ArrayFixture, ParsesArrayLiteral)
{
    std::vector<Token> tokens = {
        tok(TokenType::LEFT_BRACKET,  "["),
        numTok(1.0),
        tok(TokenType::COMMA,         ","),
        numTok(2.0),
        tok(TokenType::COMMA,         ","),
        numTok(3.0),
        tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::SEMICOLON,     ";"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* exprStmt = dynamic_cast<ExpressionStmt*>(stmts[0].get());
    ASSERT_NE(exprStmt, nullptr);
    auto* arr = dynamic_cast<ArrayLiteralExpr*>(exprStmt->expression.get());
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(arr->elements.size(), 3u);
    auto* elem0 = dynamic_cast<LiteralExpr*>(arr->elements[0].get());
    ASSERT_NE(elem0, nullptr);
    EXPECT_EQ(std::get<double>(elem0->value), 1.0);
}

// TC-AR-02: arr[0]; → ArrayAccessExpr, array=VariableExpr("arr"), index=LiteralExpr(0.0)
TEST_F(ArrayFixture, ParsesArrayAccess)
{
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER,    "arr"),
        tok(TokenType::LEFT_BRACKET,  "["),
        numTok(0.0),
        tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::SEMICOLON,     ";"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* exprStmt = dynamic_cast<ExpressionStmt*>(stmts[0].get());
    ASSERT_NE(exprStmt, nullptr);
    auto* access = dynamic_cast<ArrayAccessExpr*>(exprStmt->expression.get());
    ASSERT_NE(access, nullptr);
    auto* arrVar = dynamic_cast<VariableExpr*>(access->array.get());
    ASSERT_NE(arrVar, nullptr);
    EXPECT_EQ(arrVar->name.lexeme, "arr");
    auto* idx = dynamic_cast<LiteralExpr*>(access->index.get());
    ASSERT_NE(idx, nullptr);
    EXPECT_EQ(std::get<double>(idx->value), 0.0);
}

// TC-AR-13: arr[0] = 10; → ArrayWriteExpr
TEST_F(ArrayFixture, ParsesArrayWrite)
{
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER,    "arr"),
        tok(TokenType::LEFT_BRACKET,  "["),
        numTok(0.0),
        tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::EQUAL,         "="),
        numTok(10.0),
        tok(TokenType::SEMICOLON,     ";"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* exprStmt = dynamic_cast<ExpressionStmt*>(stmts[0].get());
    ASSERT_NE(exprStmt, nullptr);
    auto* write = dynamic_cast<ArrayWriteExpr*>(exprStmt->expression.get());
    ASSERT_NE(write, nullptr);
    auto* arrVar = dynamic_cast<VariableExpr*>(write->array.get());
    ASSERT_NE(arrVar, nullptr);
    EXPECT_EQ(arrVar->name.lexeme, "arr");
    auto* idx = dynamic_cast<LiteralExpr*>(write->index.get());
    ASSERT_NE(idx, nullptr);
    EXPECT_EQ(std::get<double>(idx->value), 0.0);
}

// ================================================================
// 실행 — 정상 케이스 (TC-AR-03, TC-AR-05~06)
// ================================================================

// TC-AR-03: var arr = [10, 20, 30]; print arr[1]; → "20\n"
TEST_F(ArrayFixture, ExecutesArrayAccess)
{
    EXPECT_EQ(runSource("var arr = [10, 20, 30]; print arr[1];"), "20\n");
}

// TC-AR-05: var arr = Array(3); print arr[0]; → "null\n"
TEST_F(ArrayFixture, ArrayCreation)
{
    EXPECT_EQ(runSource("var arr = Array(3); print arr[0];"), "null\n");
    EXPECT_EQ(runSource("var arr = Array(3); print arr[2];"), "null\n");
}

// TC-AR-06: arr[i] = val 후 읽기 → 정상 값 출력
TEST_F(ArrayFixture, ArrayWrite)
{
    EXPECT_EQ(runSource(
        "var arr = Array(3);"
        "arr[0] = 10;"
        "arr[1] = 20;"
        "arr[2] = 30;"
        "print arr[1];"), "20\n");

    EXPECT_EQ(runSource(
        "var arr = Array(3);"
        "var i = 2;"
        "arr[i] = 42;"
        "print arr[2];"), "42\n");
}

// ================================================================
// 실행 — 오류 케이스 (TC-AR-04, TC-AR-07~12)
// ================================================================

// TC-AR-04: var arr = [1, 2]; print arr[5]; → RuntimeError (범위 초과)
TEST_F(ArrayFixture, ArrayAccessOutOfBounds)
{
    runSourceExpectThrow("var arr = [1, 2]; print arr[5];");
}

// TC-AR-07: print arr["hello"]; → RuntimeError (인덱스 숫자 아님)
TEST_F(ArrayFixture, ArrayIndexNotNumber)
{
    runSourceExpectThrow("var arr = Array(3); print arr[\"hello\"];");
}

// TC-AR-08: var x = 10; print x[0]; → RuntimeError (배열 아닌 대상)
TEST_F(ArrayFixture, IndexOnNonArray)
{
    runSourceExpectThrow("var x = 10; print x[0];");
}

// TC-AR-09: var arr = Array("hi"); → RuntimeError (크기 숫자 아님)
TEST_F(ArrayFixture, ArraySizeNotNumber)
{
    runSourceExpectThrow("var arr = Array(\"hi\");");
}

// TC-AR-10: var arr = Array(3); print arr[-1]; → RuntimeError (음수 인덱스)
TEST_F(ArrayFixture, NegativeIndexThrows)
{
    runSourceExpectThrow("var arr = Array(3); print arr[-1];");
}

// TC-AR-11: var arr = Array(-1); → RuntimeError (음수 크기)
TEST_F(ArrayFixture, NegativeArraySizeThrows)
{
    runSourceExpectThrow("var arr = Array(-1);");
}

// TC-AR-12: var arr = Array(0); print arr[0]; → RuntimeError (크기 0 배열 접근)
TEST_F(ArrayFixture, ZeroSizeArrayAccessThrows)
{
    runSourceExpectThrow("var arr = Array(0); print arr[0];");
}
