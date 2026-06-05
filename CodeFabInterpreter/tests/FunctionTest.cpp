#include "gtest/gtest.h"
#include "TestHelpers.h"
#include "../Parser.h"
#include "../Checker.h"
#include "../Executor.h"
#include "../Stmt.h"
#include "../Expr.h"
#include "../Tokenizer.h"

// Checker 오류 검사용 헬퍼 — 소스 문자열을 파싱 후 Checker 오류 목록 반환
static std::vector<std::string> checkerErrors(const std::string& source)
{
    Tokenizer tz;
    auto tokens = tz.tokenize(source);
    Parser parser;
    auto stmts = parser.parse(tokens);
    Checker checker;
    checker.check(stmts);
    return checker.getErrors();
}

// -----------------------------------------------------------------------
// TC-FN-01: func 선언 파싱
// func add(a, b) { return a+b; }
// → FunctionDeclareStmt, name="add", params=["a","b"]
// -----------------------------------------------------------------------
TEST(FunctionTest, ParsesFunctionDeclaration)
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

// -----------------------------------------------------------------------
// TC-FN-02: 함수 호출 파싱
// add(1, 2);
// → ExpressionStmt → FunctionCallExpr, callee="add", args.size()==2
// -----------------------------------------------------------------------
TEST(FunctionTest, ParsesFunctionCall)
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

// -----------------------------------------------------------------------
// TC-FN-03: return 문 파싱
// return 42;
// → ReturnStmt, value = LiteralExpr(42.0)
// -----------------------------------------------------------------------
TEST(FunctionTest, ParsesReturnStatement)
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

// -----------------------------------------------------------------------
// TC-FN-03b: 빈 return 문 파싱
// return;
// → ReturnStmt, value == nullptr (null 반환)
// -----------------------------------------------------------------------
TEST(FunctionTest, ParsesEmptyReturnStatement)
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
    EXPECT_EQ(ret->value.get(), nullptr); // 값 없는 return
}

// -----------------------------------------------------------------------
// TC-FN-04: 함수 실행 — add(3, 4) → stdout "7\n"
// -----------------------------------------------------------------------
TEST(FunctionTest, ExecutesFunctionCall)
{
    Token aParam; aParam.lexeme = "a"; aParam.type = TokenType::IDENTIFIER;
    Token bParam; bParam.lexeme = "b"; bParam.type = TokenType::IDENTIFIER;
    Token addName; addName.lexeme = "add"; addName.type = TokenType::IDENTIFIER;
    Token plusOp; plusOp.type = TokenType::PLUS;

    // return a+b;
    auto aVar = std::make_unique<VariableExpr>(); aVar->name = aParam;
    auto bVar = std::make_unique<VariableExpr>(); bVar->name = bParam;
    auto bin  = std::make_unique<BinaryExpr>();
    bin->left = std::move(aVar); bin->op = plusOp; bin->right = std::move(bVar);
    auto retStmt = std::make_unique<ReturnStmt>();
    retStmt->value = std::move(bin);
    auto body = std::make_unique<BlockStmt>();
    body->statements.push_back(std::move(retStmt));

    // func add(a, b) { ... }
    auto fnDecl = std::make_unique<FunctionDeclareStmt>();
    fnDecl->name = addName; fnDecl->params = {aParam, bParam};
    fnDecl->body = std::move(body);

    // print add(3, 4);
    auto arg1 = std::make_unique<LiteralExpr>(); arg1->value = 3.0;
    auto arg2 = std::make_unique<LiteralExpr>(); arg2->value = 4.0;
    auto callExpr = std::make_unique<FunctionCallExpr>();
    callExpr->callee = addName;
    callExpr->args.push_back(std::move(arg1));
    callExpr->args.push_back(std::move(arg2));
    auto printStmt = std::make_unique<PrintStmt>();
    printStmt->expression = std::move(callExpr);

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(fnDecl));
    stmts.push_back(std::move(printStmt));

    Executor executor;
    testing::internal::CaptureStdout();
    executor.execute(stmts);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "7\n");
}

// -----------------------------------------------------------------------
// TC-FN-05: 함수 스코프 격리
// var x = 10; func setLocal(){ var x = 99; } setLocal(); print x;
// → stdout "10\n" (outer x 불변)
// -----------------------------------------------------------------------
TEST(FunctionTest, FunctionScopeIsolation)
{
    Token xToken; xToken.lexeme = "x"; xToken.type = TokenType::IDENTIFIER;
    Token fnName; fnName.lexeme = "setLocal"; fnName.type = TokenType::IDENTIFIER;

    // func setLocal() { var x = 99; }
    auto innerLit  = std::make_unique<LiteralExpr>(); innerLit->value = 99.0;
    auto innerDecl = std::make_unique<VarDeclareStmt>();
    innerDecl->name = xToken; innerDecl->initializer = std::move(innerLit);
    auto body = std::make_unique<BlockStmt>();
    body->statements.push_back(std::move(innerDecl));
    auto fnDecl = std::make_unique<FunctionDeclareStmt>();
    fnDecl->name = fnName; fnDecl->body = std::move(body);

    // var x = 10;
    auto outerLit  = std::make_unique<LiteralExpr>(); outerLit->value = 10.0;
    auto outerDecl = std::make_unique<VarDeclareStmt>();
    outerDecl->name = xToken; outerDecl->initializer = std::move(outerLit);

    // setLocal();
    auto callExpr = std::make_unique<FunctionCallExpr>();
    callExpr->callee = fnName;
    auto callStmt = std::make_unique<ExpressionStmt>();
    callStmt->expression = std::move(callExpr);

    // print x;
    auto varExpr = std::make_unique<VariableExpr>(); varExpr->name = xToken;
    auto printStmt = std::make_unique<PrintStmt>();
    printStmt->expression = std::move(varExpr);

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(outerDecl));
    stmts.push_back(std::move(fnDecl));
    stmts.push_back(std::move(callStmt));
    stmts.push_back(std::move(printStmt));

    Executor executor;
    testing::internal::CaptureStdout();
    executor.execute(stmts);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// -----------------------------------------------------------------------
// TC-FN-06: 재귀 호출 실행 — fact(5) → stdout "120\n"
// -----------------------------------------------------------------------
TEST(FunctionTest, RecursiveFunctionCall)
{
    // func fact(n) { if (n <= 1) return 1; return n * fact(n - 1); }
    // print fact(5);
    Tokenizer tz;
    auto tokens = tz.tokenize(
        "func fact(n) { if (n < 2) { return 1; } return n * fact(n - 1); }"
        "print fact(5);");
    Parser parser;
    auto stmts = parser.parse(tokens);

    Executor executor;
    testing::internal::CaptureStdout();
    executor.execute(stmts);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "120\n");
}

// -----------------------------------------------------------------------
// TC-FN-07: 반환값을 변수에 대입 — ret = add(3, 7); print ret; → "10\n"
// -----------------------------------------------------------------------
TEST(FunctionTest, ReturnValueAssignment)
{
    Tokenizer tz;
    auto tokens = tz.tokenize(
        "func add(a, b) { return a + b; }"
        "var ret = add(3, 7);"
        "print ret;");
    Parser parser;
    auto stmts = parser.parse(tokens);

    Executor executor;
    testing::internal::CaptureStdout();
    executor.execute(stmts);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// -----------------------------------------------------------------------
// TC-FN-08: Checker — 함수 외부 return → 오류 감지
// -----------------------------------------------------------------------
TEST(FunctionTest, CheckerReturnOutsideFunction)
{
    auto errors = checkerErrors("return 5;");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("함수 외부"), std::string::npos);
}

// -----------------------------------------------------------------------
// TC-FN-09: Checker — 파라미터 이름 중복 → 오류 감지
// -----------------------------------------------------------------------
TEST(FunctionTest, CheckerDuplicateParameters)
{
    auto errors = checkerErrors("func foo(a, a) { return a; }");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("중복"), std::string::npos);
}

// -----------------------------------------------------------------------
// TC-FN-10: Checker — 함수가 아닌 대상 호출 → 오류 감지
// -----------------------------------------------------------------------
TEST(FunctionTest, CheckerCallingNonFunction)
{
    auto errors = checkerErrors("var x = 10; x();");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("함수가 아닙니다"), std::string::npos);
}

// -----------------------------------------------------------------------
// TC-FN-11: Checker — 인자 개수 불일치 → 오류 감지
// -----------------------------------------------------------------------
TEST(FunctionTest, CheckerArgumentCountMismatch)
{
    auto errors = checkerErrors(
        "func foo(a, b, c) { return a; }"
        "foo(1, 2);");
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("불일치"), std::string::npos);
}

// -----------------------------------------------------------------------
// Negative UT
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// TC-FN-12: Executor — 미선언 함수 호출 → runtime_error
// foo();
// -----------------------------------------------------------------------
TEST(FunctionTest, UndefinedFunctionCallThrows)
{
    Tokenizer tz;
    auto tokens = tz.tokenize("foo();");
    Parser parser;
    auto stmts = parser.parse(tokens);
    Executor executor;
    ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// -----------------------------------------------------------------------
// TC-FN-13: Executor — 인자 개수 초과 호출 → runtime_error (Checker 우회 시에도 독립 검증)
// func f(a) { return a; } f(1, 2);
// -----------------------------------------------------------------------
TEST(FunctionTest, CallWithTooManyArgsThrows)
{
    Tokenizer tz;
    auto tokens = tz.tokenize("func f(a) { return a; } f(1, 2);");
    Parser parser;
    auto stmts = parser.parse(tokens);
    Executor executor;
    ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// -----------------------------------------------------------------------
// TC-FN-14: Executor — null 반환 함수 결과를 산술 연산에 사용 → runtime_error
// func f() {} var r = f() + 1;
// -----------------------------------------------------------------------
TEST(FunctionTest, NullReturnUsedInExprThrows)
{
    Tokenizer tz;
    auto tokens = tz.tokenize("func f() {} var r = f() + 1;");
    Parser parser;
    auto stmts = parser.parse(tokens);
    Executor executor;
    ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}
