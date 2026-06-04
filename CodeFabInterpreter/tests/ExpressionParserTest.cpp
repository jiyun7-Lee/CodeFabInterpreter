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

// parse() 결과의 첫 번째 Stmt 를 ExpressionStmt 로 꺼낸다.
// unique_ptr 소유권은 stmts 에 있으므로 raw pointer 로 반환 (읽기 전용).
static Expr* firstExpr(const std::vector<std::unique_ptr<Stmt>>& stmts)
{
    if (stmts.empty()) return nullptr;
    auto* es = dynamic_cast<ExpressionStmt*>(stmts[0].get());
    if (!es) return nullptr;
    return es->expression.get();
}

// -----------------------------------------------------------------------
// TC 1 : 숫자 리터럴 파싱
// 입력  : 42;
// 기대  : ExpressionStmt → LiteralExpr(42.0)
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesNumberLiteral)
{
    // Arrange: 숫자 리터럴 42 하나짜리 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "42", 42.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: 첫 번째 Stmt 가 ExpressionStmt 이고, 그 안의 Expr 이 LiteralExpr(42.0) 인지 확인
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
    // Arrange: 식별자 "a" 하나짜리 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: Expr 이 VariableExpr 이고, name.lexeme 가 "a" 인지 확인
    auto* var = dynamic_cast<VariableExpr*>(firstExpr(stmts));
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "a");
}

// -----------------------------------------------------------------------
// TC 3 : 사칙연산 우선순위 (* 가 + 보다 먼저)
// 입력  : 1 + 2 * 3;
// 기대  : BinaryExpr(PLUS, LiteralExpr(1), BinaryExpr(STAR, LiteralExpr(2), LiteralExpr(3)))
// -----------------------------------------------------------------------
TEST(ExprParser, RespectsMulBeforeAdd)
{
    // Arrange: "1 + 2 * 3" 에 해당하는 토큰 시퀀스 구성
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

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: 루트가 PLUS, 우측 자식이 STAR 인 트리 구조인지 확인 (우선순위 검증)
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
    EXPECT_EQ(std::get<double>(mulLeft->value), 2.0);
    EXPECT_EQ(std::get<double>(mulRight->value), 3.0);
}

// -----------------------------------------------------------------------
// TC 4 : 괄호로 우선순위 변경
// 입력  : (1 + 2) * 3;
// 기대  : BinaryExpr(STAR, GroupingExpr(BinaryExpr(PLUS, 1, 2)), LiteralExpr(3))
// -----------------------------------------------------------------------
TEST(ExprParser, GroupingOverridesPrecedence)
{
    // Arrange: "(1 + 2) * 3" 에 해당하는 토큰 시퀀스 구성
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

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: 루트가 STAR 이고, 좌측 자식이 GroupingExpr(PLUS) 인지 확인 (괄호 우선순위 역전 검증)
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

// -----------------------------------------------------------------------
// TC 5 : 대입 표현식 파싱
// 입력  : a = 10;
// 기대  : ExpressionStmt → AssignExpr(name.lexeme == "a", LiteralExpr(10))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesAssignExpr)
{
    // Arrange: "a = 10" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::NUMBER,     "10", 10.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: Expr 이 AssignExpr 이고, name 과 value 가 올바른지 확인
    auto* assign = dynamic_cast<AssignExpr*>(firstExpr(stmts));
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name.lexeme, "a");

    auto* val = dynamic_cast<LiteralExpr*>(assign->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(std::get<double>(val->value), 10.0);
}

// -----------------------------------------------------------------------
// TC 6 : 문자열 리터럴 파싱
// 입력  : "hello";
// 기대  : ExpressionStmt → LiteralExpr(value == "hello")
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesStringLiteral)
{
    // Arrange: 문자열 리터럴 토큰 시퀀스 구성 (literal 필드에 실제 문자열 값 저장)
    std::vector<Token> tokens = {
        tok(TokenType::STRING, "\"hello\"", std::string("hello")),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: LiteralExpr 이고 value 가 문자열 "hello" 인지 확인
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<std::string>(lit->value), "hello");
}

// -----------------------------------------------------------------------
// TC 7 : 불리언 true 리터럴 파싱
// 입력  : true;
// 기대  : ExpressionStmt → LiteralExpr(value == true)
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesBooleanTrue)
{
    // Arrange: TRUE 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::TRUE, "true"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: LiteralExpr 이고 value 가 bool true 인지 확인
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<bool>(lit->value), true);
}

// -----------------------------------------------------------------------
// TC 8 : 불리언 false 리터럴 파싱
// 입력  : false;
// 기대  : ExpressionStmt → LiteralExpr(value == false)
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesBooleanFalse)
{
    // Arrange: FALSE 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::FALSE, "false"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: LiteralExpr 이고 value 가 bool false 인지 확인
    auto* lit = dynamic_cast<LiteralExpr*>(firstExpr(stmts));
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<bool>(lit->value), false);
}

// -----------------------------------------------------------------------
// TC 9 : 단항 음수 파싱
// 입력  : -5;
// 기대  : ExpressionStmt → UnaryExpr(MINUS, LiteralExpr(5.0))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesUnaryMinus)
{
    // Arrange: 단항 음수 "-5" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::MINUS, "-"),
        tok(TokenType::NUMBER, "5", 5.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: UnaryExpr 이고 operator 가 MINUS, 피연산자가 LiteralExpr(5.0) 인지 확인
    auto* unary = dynamic_cast<UnaryExpr*>(firstExpr(stmts));
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op.type, TokenType::MINUS);

    auto* operand = dynamic_cast<LiteralExpr*>(unary->right.get());
    ASSERT_NE(operand, nullptr);
    EXPECT_EQ(std::get<double>(operand->value), 5.0);
}

// -----------------------------------------------------------------------
// TC 10 : 비교 연산자 파싱
// 입력  : a > 3;
// 기대  : ExpressionStmt → BinaryExpr(GREATER, VariableExpr(a), LiteralExpr(3.0))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesComparisonGreater)
{
    // Arrange: "a > 3" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::GREATER,    ">"),
        tok(TokenType::NUMBER,     "3", 3.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: BinaryExpr 이고 operator 가 GREATER, 좌우 피연산자가 올바른지 확인
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

// -----------------------------------------------------------------------
// TC 11 : 뺄셈 파싱
// 입력  : 5 - 3;
// 기대  : ExpressionStmt → BinaryExpr(MINUS, LiteralExpr(5.0), LiteralExpr(3.0))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesSubtraction)
{
    // Arrange: "5 - 3" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "5", 5.0),
        tok(TokenType::MINUS,  "-"),
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: BinaryExpr 이고 operator 가 MINUS, 좌우 피연산자가 올바른지 확인
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

// -----------------------------------------------------------------------
// TC 12 : 논리 and 파싱
// 입력  : a and b;
// 기대  : ExpressionStmt → BinaryExpr(AND, VariableExpr(a), VariableExpr(b))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesLogicalAnd)
{
    // Arrange: "a and b" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::AND,        "and"),
        tok(TokenType::IDENTIFIER, "b"),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: BinaryExpr 이고 operator 가 AND, 좌우 피연산자가 VariableExpr 인지 확인
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

// -----------------------------------------------------------------------
// TC 14 : 논리 부정 단항 연산자 파싱
// 입력  : !true;
// 기대  : ExpressionStmt → UnaryExpr(BANG, LiteralExpr(true))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesUnaryBang)
{
    // Arrange: 논리 부정 "!true" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::BANG,      "!"),
        tok(TokenType::TRUE,      "true"),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: UnaryExpr 이고 operator 가 BANG, 피연산자가 LiteralExpr(true) 인지 확인
    auto* unary = dynamic_cast<UnaryExpr*>(firstExpr(stmts));
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op.type, TokenType::BANG);

    auto* operand = dynamic_cast<LiteralExpr*>(unary->right.get());
    ASSERT_NE(operand, nullptr);
    EXPECT_EQ(std::get<bool>(operand->value), true);
}

// -----------------------------------------------------------------------
// TC 13 : 대입 우결합 파싱
// 입력  : a = b = 3;
// 기대  : AssignExpr(a, AssignExpr(b, LiteralExpr(3.0)))
//         → a = (b = 3) 으로 파싱되어야 한다
// -----------------------------------------------------------------------
TEST(ExprParser, AssignIsRightAssociative)
{
    // Arrange: "a = b = 3" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::IDENTIFIER, "b"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::NUMBER,     "3", 3.0),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: 외부 AssignExpr(a, ...) 내부에 AssignExpr(b, 3) 이 중첩되어 있는지 확인
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

// -----------------------------------------------------------------------
// TC 15 : 나눗셈 파싱
// 입력  : 6 / 2;
// 기대  : ExpressionStmt → BinaryExpr(SLASH, LiteralExpr(6.0), LiteralExpr(2.0))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesDivision)
{
    // Arrange: "6 / 2" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "6", 6.0),
        tok(TokenType::SLASH,  "/"),
        tok(TokenType::NUMBER, "2", 2.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: BinaryExpr 이고 operator 가 SLASH, 좌우 피연산자가 올바른지 확인
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

// -----------------------------------------------------------------------
// TC 16 : 비교 연산자 < 파싱
// 입력  : 3 < 5;
// 기대  : ExpressionStmt → BinaryExpr(LESS, LiteralExpr(3.0), LiteralExpr(5.0))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesComparisonLess)
{
    // Arrange: "3 < 5" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::NUMBER, "3", 3.0),
        tok(TokenType::LESS,   "<"),
        tok(TokenType::NUMBER, "5", 5.0),
        tok(TokenType::SEMICOLON, ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: BinaryExpr 이고 operator 가 LESS, 좌우 피연산자가 올바른지 확인
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

// -----------------------------------------------------------------------
// TC 17 : 논리 or 파싱
// 입력  : a or b;
// 기대  : ExpressionStmt → BinaryExpr(OR, VariableExpr(a), VariableExpr(b))
// -----------------------------------------------------------------------
TEST(ExprParser, ParsesLogicalOr)
{
    // Arrange: "a or b" 에 해당하는 토큰 시퀀스 구성
    std::vector<Token> tokens = {
        tok(TokenType::IDENTIFIER, "a"),
        tok(TokenType::OR,         "or"),
        tok(TokenType::IDENTIFIER, "b"),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    };
    Parser parser;

    // Act: 토큰 시퀀스를 파싱하여 AST 생성
    auto stmts = parser.parse(tokens);

    // Assert: BinaryExpr 이고 operator 가 OR, 좌우 피연산자가 VariableExpr 인지 확인
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
