#include "gtest/gtest.h"
#include "TestHelpers.h"
#include "../Parser.h"
#include "../Checker.h"
#include "../Executor.h"
#include "../Stmt.h"
#include "../Expr.h"
#include "../Tokenizer.h"

// ================================================================
// Fixture
// ================================================================
class FunctionFixture : public ::testing::Test
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

    // 소스 문자열 파싱 후 Checker 오류 목록 반환
    std::vector<std::string> checkerErrors(const std::string& source)
    {
        Tokenizer tz;
        auto tokens = tz.tokenize(source);
        Parser parser;
        auto stmts = parser.parse(tokens);
        Checker checker;
        checker.check(stmts);
        return checker.getErrors();
    }
};

// ================================================================
// 파싱 테스트 (TC-FN-01~03b)
// ================================================================

// TC-FN-01: func add(a, b) { return a+b; } → FunctionDeclareStmt, name="add", params=["a","b"]
TEST_F(FunctionFixture, ParsesFunctionDeclaration)
{
    std::vector<Token> tokens = {
        tok(TokenType::FUNC,        "func"),
        tok(TokenType::IDENTIFIER,  "add"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::IDENTIFIER,  "a"),
        tok(TokenType::COMMA,       ","),
        tok(TokenType::IDENTIFIER,  "b"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE,  "{"),
        tok(TokenType::RETURN,      "return"),
        tok(TokenType::IDENTIFIER,  "a"),
        tok(TokenType::PLUS,        "+"),
        tok(TokenType::IDENTIFIER,  "b"),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* fn = dynamic_cast<FunctionDeclareStmt*>(stmts[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name.lexeme, "add");
    ASSERT_EQ(fn->params.size(), 2u);
    EXPECT_EQ(fn->params[0].lexeme, "a");
    EXPECT_EQ(fn->params[1].lexeme, "b");
}

// TC-FN-02: add(1, 2); → FunctionCallExpr, callee="add", args.size()==2
TEST_F(FunctionFixture, ParsesFunctionCall)
{
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER,  "add"),
        tok(TokenType::LEFT_PAREN,  "("),
        numTok(1.0),
        tok(TokenType::COMMA,       ","),
        numTok(2.0),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON,   ";"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* exprStmt = dynamic_cast<ExpressionStmt*>(stmts[0].get());
    ASSERT_NE(exprStmt, nullptr);
    auto* call = dynamic_cast<FunctionCallExpr*>(exprStmt->expression.get());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->callee.lexeme, "add");
    ASSERT_EQ(call->args.size(), 2u);
}

// TC-FN-03: return 42; → ReturnStmt, value = LiteralExpr(42.0)
TEST_F(FunctionFixture, ParsesReturnStatement)
{
    std::vector<Token> tokens = {
        tok(TokenType::RETURN,    "return"),
        numTok(42.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    ASSERT_NE(ret, nullptr);
    auto* lit = dynamic_cast<LiteralExpr*>(ret->value.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<double>(lit->value), 42.0);
}

// TC-FN-03b: return; → ReturnStmt, value == nullptr (null 반환)
TEST_F(FunctionFixture, ParsesEmptyReturnStatement)
{
    std::vector<Token> tokens = {
        tok(TokenType::RETURN,    "return"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;
    auto stmts = parser.parse(tokens);
    ASSERT_EQ(stmts.size(), 1u);
    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    ASSERT_NE(ret, nullptr);
    EXPECT_EQ(ret->value.get(), nullptr);
}

// ================================================================
// 실행 — 정상 케이스 (TC-FN-04~07, TC-FN-15)
// ================================================================

// TC-FN-04: func add(a, b) { return a+b; } print add(3, 4); → "7\n"
TEST_F(FunctionFixture, ExecutesFunctionCall)
{
    EXPECT_EQ(runSource(
        "func add(a, b) { return a + b; }"
        "print add(3, 4);"), "7\n");
}

// TC-FN-05: var x = 10; func setLocal(){ var x = 99; } setLocal(); print x; → "10\n"
TEST_F(FunctionFixture, FunctionScopeIsolation)
{
    EXPECT_EQ(runSource(
        "var x = 10;"
        "func setLocal() { var x = 99; }"
        "setLocal();"
        "print x;"), "10\n");
}

// TC-FN-06: fact(5) → "120\n"
TEST_F(FunctionFixture, RecursiveFunctionCall)
{
    EXPECT_EQ(runSource(
        "func fact(n) { if (n < 2) { return 1; } return n * fact(n - 1); }"
        "print fact(5);"), "120\n");
}

// TC-FN-07: var ret = add(3, 7); print ret; → "10\n"
TEST_F(FunctionFixture, ReturnValueAssignment)
{
    EXPECT_EQ(runSource(
        "func add(a, b) { return a + b; }"
        "var ret = add(3, 7);"
        "print ret;"), "10\n");
}

// TC-FN-15: func f() { return; } var r = f(); print r; → "null\n"
TEST_F(FunctionFixture, EmptyReturnYieldsNull)
{
    EXPECT_EQ(runSource(
        "func f() { return; }"
        "var r = f();"
        "print r;"), "null\n");
}

// ================================================================
// Checker 오류 케이스 (TC-FN-08~11, TC-FN-16)
// ================================================================

// TC-FN-08: return 5; → 함수 외부 return 오류
TEST_F(FunctionFixture, CheckerReturnOutsideFunction)
{
    auto errors = checkerErrors("return 5;");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("함수 외부"), std::string::npos);
}

// TC-FN-09: func foo(a, a) { return a; } → 파라미터 이름 중복 오류
TEST_F(FunctionFixture, CheckerDuplicateParameters)
{
    auto errors = checkerErrors("func foo(a, a) { return a; }");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("중복"), std::string::npos);
}

// TC-FN-10: var x = 10; x(); → 함수 아닌 대상 호출 오류
TEST_F(FunctionFixture, CheckerCallingNonFunction)
{
    auto errors = checkerErrors("var x = 10; x();");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("함수가 아닙니다"), std::string::npos);
}

// TC-FN-11: func foo(a, b, c) {} foo(1, 2); → 인자 개수 불일치 오류
TEST_F(FunctionFixture, CheckerArgumentCountMismatch)
{
    auto errors = checkerErrors(
        "func foo(a, b, c) { return a; }"
        "foo(1, 2);");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("불일치"), std::string::npos);
}

// TC-FN-16: return 5; (1번째 줄) → 오류 메시지에 "[1번째 줄]"과 "함수 외부" 포함
TEST_F(FunctionFixture, ReturnOutsideFunctionErrorIncludesLineNumber)
{
    auto errors = checkerErrors("return 5;");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("[1번째 줄]"),  std::string::npos);
    EXPECT_NE(errors[0].find("함수 외부"), std::string::npos);
}

// ================================================================
// Executor 오류 케이스 (TC-FN-12~14)
// ================================================================

// TC-FN-12: foo(); → 미선언 함수 호출 runtime_error
TEST_F(FunctionFixture, UndefinedFunctionCallThrows)
{
    runSourceExpectThrow("foo();");
}

// TC-FN-13: func f(a) { return a; } f(1, 2); → 인자 개수 초과 runtime_error
TEST_F(FunctionFixture, CallWithTooManyArgsThrows)
{
    runSourceExpectThrow("func f(a) { return a; } f(1, 2);");
}

// TC-FN-14: func f() {} var r = f() + 1; → null 반환값 산술 연산 runtime_error
TEST_F(FunctionFixture, NullReturnUsedInExprThrows)
{
    runSourceExpectThrow("func f() {} var r = f() + 1;");
}
