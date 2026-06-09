#include <gtest/gtest.h>
#include "../Parser.h"
#include "../Expr.h"
#include "../Stmt.h"
#include "../Token.h"
#include "../Value.h"

// ================================================================
// 토큰 생성 헬퍼
// ================================================================
static Token tok(TokenType type, const std::string& lexeme, Value literal = {}, int line = 1)
{
    return Token{ type, lexeme, line, literal };
}

static Token eof() { return tok(TokenType::EOF_TOKEN, ""); }

// ================================================================
// Fixture
// ================================================================
class ExprParserFixture : public ::testing::Test
{
protected:
    Parser parser;

    // parse() 결과의 첫 번째 Stmt 를 ExpressionStmt 로 꺼낸다.
    Expr* firstExpr(const std::vector<std::unique_ptr<Stmt>>& stmts)
    {
        if (stmts.empty()) return nullptr;
        auto* es = dynamic_cast<ExpressionStmt*>(stmts[0].get());
        if (!es) return nullptr;
        return es->expression.get();
    }
};

// ================================================================
// TC-01 : 숫자 리터럴 파싱
// 입력  : 42;
// 기대  : LiteralExpr(42.0)
// ================================================================
TEST_F(ExprParserFixture, ParsesNumberLiteral)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "42", 42.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<double>(lit->value), 42.0);
}

// ================================================================
// TC-02 : 변수 참조 파싱
// 입력  : a;
// 기대  : VariableExpr(name.lexeme == "a")
// ================================================================
TEST_F(ExprParserFixture, ParsesVariableExpr)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* var = dynamic_cast<VariableExpr*>(firstExpr(stmts));
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "a");
}

// ================================================================
// TC-03 : 사칙연산 우선순위 (* 가 + 보다 먼저)
// 입력  : 1 + 2 * 3;
// 기대  : BinaryExpr(PLUS, LiteralExpr(1), BinaryExpr(STAR, 2, 3))
// ================================================================
TEST_F(ExprParserFixture, RespectsMulBeforeAdd)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "1", 1.0),
        tok(TokenType::PLUS,   "+"),
        tok(TokenType::NUMBER, "2", 2.0),
        tok(TokenType::STAR,   "*"),
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert: 루트가 PLUS, 우측 자식이 STAR (우선순위 검증)
    auto* add = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op.type, TokenType::PLUS);

    auto* addLeft = dynamic_cast<LiteralExpr*>(add->left.get());
    ASSERT_NE(addLeft, nullptr);
    EXPECT_EQ(std::get<double>(addLeft->value), 1.0);

    auto* mul = dynamic_cast<BinaryExpr*>(add->right.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op.type, TokenType::STAR);

    auto* mulLeft  = dynamic_cast<LiteralExpr*>(mul->left.get());
    auto* mulRight = dynamic_cast<LiteralExpr*>(mul->right.get());
    ASSERT_NE(mulLeft, nullptr);
    ASSERT_NE(mulRight, nullptr);
    EXPECT_EQ(std::get<double>(mulLeft->value),  2.0);
    EXPECT_EQ(std::get<double>(mulRight->value), 3.0);
}

// ================================================================
// TC-04 : 괄호로 우선순위 변경
// 입력  : (1 + 2) * 3;
// 기대  : BinaryExpr(STAR, GroupingExpr(BinaryExpr(PLUS, 1, 2)), LiteralExpr(3))
// ================================================================
TEST_F(ExprParserFixture, GroupingOverridesPrecedence)
{
    // Arrange
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

    // Act
    auto stmts = parser.parse(tokens);

    // Assert: 루트가 STAR, 좌측 자식이 GroupingExpr(PLUS) (괄호 우선순위 역전 검증)
    auto* mul = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op.type, TokenType::STAR);

    auto* grp = dynamic_cast<GroupingExpr*>(mul->left.get());
    ASSERT_NE(grp, nullptr);

    auto* add = dynamic_cast<BinaryExpr*>(grp->expression.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op.type, TokenType::PLUS);

    auto* mulRight = dynamic_cast<LiteralExpr*>(mul->right.get());
    ASSERT_NE(mulRight, nullptr);
    EXPECT_EQ(std::get<double>(mulRight->value), 3.0);
}

// ================================================================
// TC-05 : 대입 표현식 파싱
// 입력  : a = 10;
// 기대  : AssignExpr(name.lexeme == "a", LiteralExpr(10))
// ================================================================
TEST_F(ExprParserFixture, ParsesAssignExpr)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::NUMBER,     "10", 10.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* assign = dynamic_cast<AssignExpr*>(firstExpr(stmts));
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name.lexeme, "a");

    auto* val = dynamic_cast<LiteralExpr*>(assign->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(std::get<double>(val->value), 10.0);
}

// ================================================================
// TC-06 : 문자열 리터럴 파싱
// 입력  : "hello";
// 기대  : LiteralExpr(value == "hello")
// ================================================================
TEST_F(ExprParserFixture, ParsesStringLiteral)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::STRING, "\"hello\"", std::string("hello")),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<std::string>(lit->value), "hello");
}

// ================================================================
// TC-07 : 불리언 true 리터럴 파싱
// 입력  : true;
// 기대  : LiteralExpr(value == true)
// ================================================================
TEST_F(ExprParserFixture, ParsesBooleanTrue)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::TRUE, "true"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<bool>(lit->value), true);
}

// ================================================================
// TC-08 : 불리언 false 리터럴 파싱
// 입력  : false;
// 기대  : LiteralExpr(value == false)
// ================================================================
TEST_F(ExprParserFixture, ParsesBooleanFalse)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::FALSE, "false"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<bool>(lit->value), false);
}

// ================================================================
// TC-09 : 단항 음수 파싱
// 입력  : -5;
// 기대  : UnaryExpr(MINUS, LiteralExpr(5.0))
// ================================================================
TEST_F(ExprParserFixture, ParsesUnaryMinus)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::MINUS, "-"),
        tok(TokenType::NUMBER, "5", 5.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* unary = dynamic_cast<UnaryExpr*>(firstExpr(stmts));
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op.type, TokenType::MINUS);

    auto* operand = dynamic_cast<LiteralExpr*>(unary->right.get());
    ASSERT_NE(operand, nullptr);
    EXPECT_EQ(std::get<double>(operand->value), 5.0);
}

// ================================================================
// TC-10 : 비교 연산자 > 파싱
// 입력  : a > 3;
// 기대  : BinaryExpr(GREATER, VariableExpr(a), LiteralExpr(3.0))
// ================================================================
TEST_F(ExprParserFixture, ParsesComparisonGreater)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::GREATER,    ">"),
        tok(TokenType::NUMBER,     "3", 3.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::GREATER);

    auto* left = dynamic_cast<VariableExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->name.lexeme, "a");

    auto* right = dynamic_cast<LiteralExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<double>(right->value), 3.0);
}

// ================================================================
// TC-11 : 뺄셈 파싱
// 입력  : 5 - 3;
// 기대  : BinaryExpr(MINUS, LiteralExpr(5.0), LiteralExpr(3.0))
// ================================================================
TEST_F(ExprParserFixture, ParsesSubtraction)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "5", 5.0),
        tok(TokenType::MINUS,  "-"),
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::MINUS);

    auto* left = dynamic_cast<LiteralExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(std::get<double>(left->value), 5.0);

    auto* right = dynamic_cast<LiteralExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<double>(right->value), 3.0);
}

// ================================================================
// TC-12 : 논리 and 파싱
// 입력  : a and b;
// 기대  : BinaryExpr(AND, VariableExpr(a), VariableExpr(b))
// ================================================================
TEST_F(ExprParserFixture, ParsesLogicalAnd)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::AND,        "and"),
        tok(TokenType::IDENTIFIER, "b"),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::AND);

    auto* left = dynamic_cast<VariableExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->name.lexeme, "a");

    auto* right = dynamic_cast<VariableExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->name.lexeme, "b");
}

// ================================================================
// TC-13 : 대입 우결합 파싱
// 입력  : a = b = 3;
// 기대  : AssignExpr(a, AssignExpr(b, LiteralExpr(3.0)))  → a = (b = 3)
// ================================================================
TEST_F(ExprParserFixture, AssignIsRightAssociative)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::IDENTIFIER, "b"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::NUMBER,     "3", 3.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert: 외부 AssignExpr(a, ...) 내부에 AssignExpr(b, 3) 중첩
    auto* outer = dynamic_cast<AssignExpr*>(firstExpr(stmts));
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->name.lexeme, "a");

    auto* inner = dynamic_cast<AssignExpr*>(outer->value.get());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name.lexeme, "b");

    auto* val = dynamic_cast<LiteralExpr*>(inner->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(std::get<double>(val->value), 3.0);
}

// ================================================================
// TC-14 : 논리 부정 단항 연산자 파싱
// 입력  : !true;
// 기대  : UnaryExpr(BANG, LiteralExpr(true))
// ================================================================
TEST_F(ExprParserFixture, ParsesUnaryBang)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::BANG,      "!"),
        tok(TokenType::TRUE,      "true"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* unary = dynamic_cast<UnaryExpr*>(firstExpr(stmts));
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op.type, TokenType::BANG);

    auto* operand = dynamic_cast<LiteralExpr*>(unary->right.get());
    ASSERT_NE(operand, nullptr);
    EXPECT_EQ(std::get<bool>(operand->value), true);
}

// ================================================================
// TC-15 : 나눗셈 파싱
// 입력  : 6 / 2;
// 기대  : BinaryExpr(SLASH, LiteralExpr(6.0), LiteralExpr(2.0))
// ================================================================
TEST_F(ExprParserFixture, ParsesDivision)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "6", 6.0),
        tok(TokenType::SLASH,  "/"),
        tok(TokenType::NUMBER, "2", 2.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::SLASH);

    auto* left = dynamic_cast<LiteralExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(std::get<double>(left->value), 6.0);

    auto* right = dynamic_cast<LiteralExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<double>(right->value), 2.0);
}

// ================================================================
// TC-16 : 비교 연산자 < 파싱
// 입력  : 3 < 5;
// 기대  : BinaryExpr(LESS, LiteralExpr(3.0), LiteralExpr(5.0))
// ================================================================
TEST_F(ExprParserFixture, ParsesComparisonLess)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::LESS,   "<"),
        tok(TokenType::NUMBER, "5", 5.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::LESS);

    auto* left = dynamic_cast<LiteralExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(std::get<double>(left->value), 3.0);

    auto* right = dynamic_cast<LiteralExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<double>(right->value), 5.0);
}

// ================================================================
// TC-17 : 논리 or 파싱
// 입력  : a or b;
// 기대  : BinaryExpr(OR, VariableExpr(a), VariableExpr(b))
// ================================================================
TEST_F(ExprParserFixture, ParsesLogicalOr)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::OR,         "or"),
        tok(TokenType::IDENTIFIER, "b"),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::OR);

    auto* left = dynamic_cast<VariableExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->name.lexeme, "a");

    auto* right = dynamic_cast<VariableExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->name.lexeme, "b");
}

// ================================================================
// TC-18 : 이항 연산자 오른쪽 피연산자 누락 → runtime_error
// 입력  : 1 +
// ================================================================
TEST_F(ExprParserFixture, MissingRightOperandThrows)
{
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "1", 1.0),
        tok(TokenType::PLUS,   "+"),
        eof()
    };
    ASSERT_THROW(parser.parse(tokens), std::runtime_error);
}

// ================================================================
// TC-19 : 닫는 괄호 없는 그루핑 → runtime_error
// 입력  : (1 + 2
// ================================================================
TEST_F(ExprParserFixture, UnterminatedGroupingThrows)
{
    std::vector<Token> tokens = {
        tok(TokenType::LEFT_PAREN, "("),
        tok(TokenType::NUMBER, "1", 1.0),
        tok(TokenType::PLUS,   "+"),
        tok(TokenType::NUMBER, "2", 2.0),
        eof()
    };
    ASSERT_THROW(parser.parse(tokens), std::runtime_error);
}

// ================================================================
// TC-20 : 빈 괄호 그루핑 → runtime_error
// 입력  : ();
// ================================================================
TEST_F(ExprParserFixture, EmptyGroupingThrows)
{
    std::vector<Token> tokens = {
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON,   ";"),
        eof()
    };
    ASSERT_THROW(parser.parse(tokens), std::runtime_error);
}

// ================================================================
// TC-21 : 대입 연산자 오른쪽 값 누락 → runtime_error
// 입력  : a =;
// ================================================================
TEST_F(ExprParserFixture, MissingAssignValueThrows)
{
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    ASSERT_THROW(parser.parse(tokens), std::runtime_error);
}

// ================================================================
// TC-22 : 모듈러 연산자 파싱
// 입력  : 10 % 3;
// 기대  : BinaryExpr(PERCENT, LiteralExpr(10.0), LiteralExpr(3.0))
// ================================================================
TEST_F(ExprParserFixture, ParsesModulo)
{
    // Arrange
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER,    "10", 10.0),
        tok(TokenType::PERCENT,   "%"),
        tok(TokenType::NUMBER,    "3",  3.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };

    // Act
    auto stmts = parser.parse(tokens);

    // Assert
    auto* bin = dynamic_cast<BinaryExpr*>(firstExpr(stmts));
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::PERCENT);

    auto* left = dynamic_cast<LiteralExpr*>(bin->left.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(std::get<double>(left->value), 10.0);

    auto* right = dynamic_cast<LiteralExpr*>(bin->right.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<double>(right->value), 3.0);
}
