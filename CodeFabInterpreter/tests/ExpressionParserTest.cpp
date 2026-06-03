#include <gtest/gtest.h>
#include "../Parser.h"
#include "../Expr.h"
#include "../Stmt.h"
#include "../Token.h"
#include "../Value.h"

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static Token tok(TokenType type, const std::string& lexeme, Value literal = {}, int line = 1)
{
    return Token{ type, lexeme, line, literal };
}

static Token eof() { return tok(TokenType::EOF_TOKEN, ""); }

// Extract the Expr from the first ExpressionStmt in the parse result.
static Expr* firstExpr(const std::vector<Stmt*>& stmts)
{
    if (stmts.empty()) return nullptr;
    auto* es = dynamic_cast<ExpressionStmt*>(stmts[0]);
    if (!es) return nullptr;
    return es->expression;
}

// -----------------------------------------------------------------------
// TC 1 : Number literal
// Input  : 42;
// Expect : ExpressionStmt -> LiteralExpr(42.0)
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesNumberLiteral)
{
    // Arrange: single number literal token sequence
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "42", 42.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: parse token sequence into AST
    auto stmts = parser.parse(tokens);

    // Assert: first Stmt is ExpressionStmt wrapping LiteralExpr(42.0)
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<double>(lit->value), 42.0);
}

// -----------------------------------------------------------------------
// TC 2 : Variable expression
// Input  : a;
// Expect : ExpressionStmt -> VariableExpr(name.lexeme == "a")
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesVariableExpr)
{
    // Arrange: single identifier token sequence
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: parse token sequence into AST
    auto stmts = parser.parse(tokens);

    // Assert: Expr is VariableExpr with name.lexeme == "a"
    auto* var = dynamic_cast<VariableExpr*>(firstExpr(stmts));
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "a");
}

// -----------------------------------------------------------------------
// TC 3 : Operator precedence (* before +)
// Input  : 1 + 2 * 3;
// Expect : BinaryExpr(PLUS, LiteralExpr(1), BinaryExpr(STAR, LiteralExpr(2), LiteralExpr(3)))
// -----------------------------------------------------------------------
TEST(ExprParser, RespectsMulBeforeAdd)
{
    // Arrange: "1 + 2 * 3" token sequence
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

    // Act: parse token sequence into AST
    auto stmts = parser.parse(tokens);

    // Assert: root is PLUS, right child is STAR (precedence verification)
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
// TC 4 : Grouping overrides precedence
// Input  : (1 + 2) * 3;
// Expect : BinaryExpr(STAR, GroupingExpr(BinaryExpr(PLUS, 1, 2)), LiteralExpr(3))
// -----------------------------------------------------------------------
TEST(ExprParser, GroupingOverridesPrecedence)
{
    // Arrange: "(1 + 2) * 3" token sequence
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

    // Act: parse token sequence into AST
    auto stmts = parser.parse(tokens);

    // Assert: root is STAR, left child is GroupingExpr(PLUS) (precedence override)
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
// TC 5 : Assignment expression
// Input  : a = 10;
// Expect : ExpressionStmt -> AssignExpr(name.lexeme == "a", LiteralExpr(10))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesAssignExpr)
{
    // Arrange: "a = 10" token sequence
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::NUMBER,     "10", 10.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    Parser parser;

    // Act: parse token sequence into AST
    auto stmts = parser.parse(tokens);

    // Assert: Expr is AssignExpr with correct name and value
    auto* assign = dynamic_cast<AssignExpr*>(firstExpr(stmts));
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name.lexeme, "a");

    auto* val = dynamic_cast<LiteralExpr*>(assign->value);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(std::get<double>(val->value), 10.0);
}
