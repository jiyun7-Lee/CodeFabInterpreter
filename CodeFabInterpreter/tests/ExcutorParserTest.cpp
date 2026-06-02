#include <gtest/gtest.h>
#include "../Parser.h"
#include "../Expr.h"
#include "../Stmt.h"
#include "../Token.h"
#include "../Value.h"

// -----------------------------------------------------------------------
// 헬퍼
// -----------------------------------------------------------------------

static Token tok(TokenType type, const std::string& lexeme, Value literal = {}, int line = 1)
{
    return Token{ type, lexeme, line, literal };
}

static Token eof() { return tok(TokenType::EOF_TOKEN, ""); }

// parse() 결과의 첫 번째 Stmt를 ExpressionStmt로 꺼낸다.
static Expr* firstExpr(const std::vector<Stmt*>& stmts)
{
    if (stmts.empty()) return nullptr;
    auto* es = dynamic_cast<ExpressionStmt*>(stmts[0]);
    if (!es) return nullptr;
    return es->expression;
}

// -----------------------------------------------------------------------
// TC 1 : 숫자 리터럴 파싱
// 입력  : 42;
// 기대  : ExpressionStmt → LiteralExpr(42.0)
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesNumberLiteral)
{
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "42", 42.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    Parser parser;
    auto stmts = parser.parse(tokens);

    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<double>(lit->value), 42.0);
}

// -----------------------------------------------------------------------
// TC 2 : 변수 참조 파싱
// 입력  : a;
// 기대  : ExpressionStmt → VariableExpr(name.lexeme == "a")
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesVariableExpr)
{
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    Parser parser;
    auto stmts = parser.parse(tokens);

    auto* var = dynamic_cast<VariableExpr*>(firstExpr(stmts));
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "a");
}

// -----------------------------------------------------------------------
// TC 3 : 사칙연산 우선순위 (* 가 + 보다 먼저)
// 입력  : 1 + 2 * 3;
// 기대  : BinaryExpr(PLUS
//              left  = LiteralExpr(1)
//              right = BinaryExpr(STAR, LiteralExpr(2), LiteralExpr(3)))
// -----------------------------------------------------------------------
TEST(ExprParser, RespectsMulBeforeAdd)
{
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "1", 1.0),
        tok(TokenType::PLUS,   "+"),
        tok(TokenType::NUMBER, "2", 2.0),
        tok(TokenType::STAR,   "*"),
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    Parser parser;
    auto stmts = parser.parse(tokens);

    auto* add = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op.type, TokenType::PLUS);

    auto* lhs = dynamic_cast<LiteralExpr*>(add->left);
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(std::get<double>(lhs->value), 1.0);

    auto* mul = dynamic_cast<BinaryExpr*>(add->right);
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op.type, TokenType::STAR);

    auto* mulL = dynamic_cast<LiteralExpr*>(mul->left);
    auto* mulR = dynamic_cast<LiteralExpr*>(mul->right);
    ASSERT_NE(mulL, nullptr);
    ASSERT_NE(mulR, nullptr);
    EXPECT_EQ(std::get<double>(mulL->value), 2.0);
    EXPECT_EQ(std::get<double>(mulR->value), 3.0);
}

// -----------------------------------------------------------------------
// TC 4 : 괄호로 우선순위 변경
// 입력  : (1 + 2) * 3;
// 기대  : BinaryExpr(STAR
//              left  = GroupingExpr(BinaryExpr(PLUS, 1, 2))
//              right = LiteralExpr(3))
// -----------------------------------------------------------------------
TEST(ExprParser, GroupingOverridesPrecedence)
{
    std::vector<Token> tokens = {
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::NUMBER, "1", 1.0),
        tok(TokenType::PLUS,   "+"),
        tok(TokenType::NUMBER, "2", 2.0),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::STAR,   "*"),
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    Parser parser;
    auto stmts = parser.parse(tokens);

    auto* mul = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op.type, TokenType::STAR);

    auto* grp = dynamic_cast<GroupingExpr*>(mul->left);
    ASSERT_NE(grp, nullptr);

    auto* add = dynamic_cast<BinaryExpr*>(grp->expression);
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op.type, TokenType::PLUS);

    auto* rhs = dynamic_cast<LiteralExpr*>(mul->right);
    ASSERT_NE(rhs, nullptr);
    EXPECT_EQ(std::get<double>(rhs->value), 3.0);
}

// -----------------------------------------------------------------------
// TC 5 : 대입 표현식 파싱
// 입력  : a = 10;
// 기대  : ExpressionStmt → AssignExpr(name.lexeme == "a", LiteralExpr(10))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesAssignExpr)
{
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::NUMBER,     "10", 10.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };

    Parser parser;
    auto stmts = parser.parse(tokens);

    auto* assign = dynamic_cast<AssignExpr*>(firstExpr(stmts));
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name.lexeme, "a");

    auto* val = dynamic_cast<LiteralExpr*>(assign->value);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(std::get<double>(val->value), 10.0);
}
