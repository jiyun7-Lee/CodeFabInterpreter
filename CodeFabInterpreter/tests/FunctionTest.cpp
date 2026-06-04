#include "gtest/gtest.h"
#include "../Parser.h"
#include "../Executor.h"
#include "../Stmt.h"
#include "../Expr.h"

// -----------------------------------------------------------------------
// 헬퍼
// -----------------------------------------------------------------------
static Token tok(TokenType type, const std::string& lexeme = "")
{
    Token t; t.type = type; t.lexeme = lexeme; t.line = 1; t.literal = {};
    return t;
}
static Token numTok(double val)
{
    Token t; t.type = TokenType::NUMBER; t.lexeme = std::to_string(val);
    t.line = 1; t.literal = val;
    return t;
}
static Token eof() { return tok(TokenType::EOF_TOKEN); }

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
