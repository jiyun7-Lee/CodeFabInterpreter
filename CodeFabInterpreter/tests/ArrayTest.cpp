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
// TC-AR-01: 배열 리터럴 파싱
// [1, 2, 3];
// → ExpressionStmt → ArrayLiteralExpr, elements.size()==3, elements[0]==1.0
// -----------------------------------------------------------------------
TEST(ArrayTest, ParsesArrayLiteral)
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

// -----------------------------------------------------------------------
// TC-AR-02: 배열 접근 파싱
// arr[0];
// → ExpressionStmt → ArrayAccessExpr, array=VariableExpr("arr"), index=LiteralExpr(0.0)
// -----------------------------------------------------------------------
TEST(ArrayTest, ParsesArrayAccess)
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

// -----------------------------------------------------------------------
// TC-AR-03: 배열 요소 접근 실행
// var arr = [10, 20, 30]; print arr[1];
// → stdout "20\n"
// -----------------------------------------------------------------------
TEST(ArrayTest, ExecutesArrayAccess)
{
    Token arrToken; arrToken.lexeme = "arr"; arrToken.type = TokenType::IDENTIFIER;

    // [10, 20, 30]
    auto e1 = std::make_unique<LiteralExpr>(); e1->value = 10.0;
    auto e2 = std::make_unique<LiteralExpr>(); e2->value = 20.0;
    auto e3 = std::make_unique<LiteralExpr>(); e3->value = 30.0;
    auto arrLit = std::make_unique<ArrayLiteralExpr>();
    arrLit->elements.push_back(std::move(e1));
    arrLit->elements.push_back(std::move(e2));
    arrLit->elements.push_back(std::move(e3));

    // var arr = [10, 20, 30];
    auto varDecl = std::make_unique<VarDeclareStmt>();
    varDecl->name = arrToken; varDecl->initializer = std::move(arrLit);

    // arr[1]
    auto arrVar = std::make_unique<VariableExpr>(); arrVar->name = arrToken;
    auto idxLit = std::make_unique<LiteralExpr>(); idxLit->value = 1.0;
    auto accessExpr = std::make_unique<ArrayAccessExpr>();
    accessExpr->array = std::move(arrVar); accessExpr->index = std::move(idxLit);

    // print arr[1];
    auto printStmt = std::make_unique<PrintStmt>();
    printStmt->expression = std::move(accessExpr);

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(varDecl));
    stmts.push_back(std::move(printStmt));

    Executor executor;
    testing::internal::CaptureStdout();
    executor.execute(stmts);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "20\n");
}

// -----------------------------------------------------------------------
// TC-AR-04: 배열 범위 초과 접근 → RuntimeError
// var arr = [1, 2]; print arr[5];
// -----------------------------------------------------------------------
TEST(ArrayTest, ArrayAccessOutOfBounds)
{
    Token arrToken; arrToken.lexeme = "arr"; arrToken.type = TokenType::IDENTIFIER;

    auto e1 = std::make_unique<LiteralExpr>(); e1->value = 1.0;
    auto e2 = std::make_unique<LiteralExpr>(); e2->value = 2.0;
    auto arrLit = std::make_unique<ArrayLiteralExpr>();
    arrLit->elements.push_back(std::move(e1));
    arrLit->elements.push_back(std::move(e2));

    auto varDecl = std::make_unique<VarDeclareStmt>();
    varDecl->name = arrToken; varDecl->initializer = std::move(arrLit);

    auto arrVar = std::make_unique<VariableExpr>(); arrVar->name = arrToken;
    auto idxLit = std::make_unique<LiteralExpr>(); idxLit->value = 5.0;
    auto accessExpr = std::make_unique<ArrayAccessExpr>();
    accessExpr->array = std::move(arrVar); accessExpr->index = std::move(idxLit);

    auto printStmt = std::make_unique<PrintStmt>();
    printStmt->expression = std::move(accessExpr);

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(varDecl));
    stmts.push_back(std::move(printStmt));

    Executor executor;
    ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}
