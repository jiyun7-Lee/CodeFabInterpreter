#include <gtest/gtest.h>
#include "../Parser.h"

// ================================================================
// 토큰 생성 헬퍼
// ================================================================
static Token makeTok(TokenType t, const std::string& lex = "", int line = 1)
{
    return Token{ t, lex, line, std::monostate{} };
}
static Token makeNum(double v, int line = 1)
{
    return Token{ TokenType::NUMBER, std::to_string(v), line, v };
}
static Token makeStr(const std::string& s, int line = 1)
{
    return Token{ TokenType::STRING, "\"" + s + "\"", line, std::string(s) };
}
static Token makeBoolTok(bool v, int line = 1)
{
    return Token{ v ? TokenType::TRUE : TokenType::FALSE,
                  v ? "true" : "false", line, v };
}
static Token makeId(const std::string& name, int line = 1)
{
    return Token{ TokenType::IDENTIFIER, name, line, std::monostate{} };
}
static Token makeEof(int line = 1)
{
    return Token{ TokenType::EOF_TOKEN, "", line, std::monostate{} };
}

// ================================================================
// FakeExprParser
// B파트 미구현 상태에서 Stmt 파싱 테스트용.
// parseExpression() 호출 시 현재 토큰 1개 소비 후 LiteralExpr 반환.
// ================================================================
class FakeExprParser : public Parser
{
protected:
    std::unique_ptr<Expr> parseExpression() override
    {
        auto lit = std::make_unique<LiteralExpr>();
        if (check(TokenType::NUMBER))
            lit->value = std::get<double>(advance().literal);
        else if (check(TokenType::STRING))
            lit->value = std::get<std::string>(advance().literal);
        else if (check(TokenType::TRUE) || check(TokenType::FALSE))
            lit->value = (advance().type == TokenType::TRUE);
        else if (check(TokenType::IDENTIFIER))
            advance();
        while (!isAtEnd()                         &&
               !check(TokenType::SEMICOLON)       &&
               !check(TokenType::RIGHT_PAREN)     &&
               !check(TokenType::RIGHT_BRACE))
            advance();
        return lit;
    }
};

// ================================================================
// Test Fixture
// ================================================================
class ParserStmtTest : public ::testing::Test
{
protected:
    FakeExprParser p;
};

// ================================================================
// VarDeclareStmt
// ================================================================

// P-TC-01 : var a = 3;  →  VarDeclareStmt, name="a", initializer == LiteralExpr(3.0)
TEST_F(ParserStmtTest, P_TC_01_VarWithNumber)
{

    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("a"),
        makeTok(TokenType::EQUAL, "="), makeNum(3.0),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "a");
    ASSERT_NE(s->initializer, nullptr);
    auto* lit = dynamic_cast<LiteralExpr*>(s->initializer.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lit->value), 3.0);
}

// P-TC-02 : var abc = "hello";  →  VarDeclareStmt, name="abc", initializer == LiteralExpr("hello")
TEST_F(ParserStmtTest, P_TC_02_VarWithString)
{

    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("abc"),
        makeTok(TokenType::EQUAL, "="), makeStr("hello"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "abc");
    ASSERT_NE(s->initializer, nullptr);
    auto* lit = dynamic_cast<LiteralExpr*>(s->initializer.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<std::string>(lit->value), "hello");
}

// P-TC-03 : var flag = true;  →  VarDeclareStmt, name="flag", initializer == LiteralExpr(true)
TEST_F(ParserStmtTest, P_TC_03_VarWithBool)
{

    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("flag"),
        makeTok(TokenType::EQUAL, "="), makeBoolTok(true),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "flag");
    ASSERT_NE(s->initializer, nullptr);
    auto* lit = dynamic_cast<LiteralExpr*>(s->initializer.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<bool>(lit->value), true);
}

// P-TC-04 : var a = b + 1;  →  initializer != null
TEST_F(ParserStmtTest, P_TC_04_VarWithExpr)
{

    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("a"),
        makeTok(TokenType::EQUAL, "="), makeId("b"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->initializer, nullptr);
}

// P-TC-05 : var a = 3  (세미콜론 없음)  →  parse 오류
TEST_F(ParserStmtTest, P_TC_05_VarMissingSemicolon)
{

    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::VAR, "var"), makeId("a"),
            makeTok(TokenType::EQUAL, "="), makeNum(3.0),
            makeEof()
        }),
        std::runtime_error
    );
}

// ================================================================
// PrintStmt
// ================================================================

// P-TC-06 : print a;  →  PrintStmt, expression != null
TEST_F(ParserStmtTest, P_TC_06_PrintVariable)
{

    auto stmts = p.parse({
        makeTok(TokenType::PRINT, "print"), makeId("a"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<PrintStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->expression, nullptr);
}

// P-TC-07 : print a + b;  →  PrintStmt, expression != null
TEST_F(ParserStmtTest, P_TC_07_PrintExpression)
{

    auto stmts = p.parse({
        makeTok(TokenType::PRINT, "print"), makeId("a"),
        makeTok(TokenType::PLUS, "+"), makeId("b"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<PrintStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->expression, nullptr);
}

// P-TC-08 : print a  (세미콜론 없음)  →  parse 오류
TEST_F(ParserStmtTest, P_TC_08_PrintMissingSemicolon)
{

    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::PRINT, "print"), makeId("a"),
            makeEof()
        }),
        std::runtime_error
    );
}

// ================================================================
// BlockStmt
// ================================================================

// P-TC-09 : {}  →  BlockStmt, statements.size() == 0
TEST_F(ParserStmtTest, P_TC_09_EmptyBlock)
{

    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<BlockStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->statements.size(), 0u);
}

// P-TC-10 : { print a; }  →  statements.size() == 1
TEST_F(ParserStmtTest, P_TC_10_BlockOneStmt)
{

    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<BlockStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->statements.size(), 1u);
}

// P-TC-11 : { print a; print b; }  →  statements.size() == 2
TEST_F(ParserStmtTest, P_TC_11_BlockMultipleStmts)
{

    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::PRINT, "print"), makeId("b"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<BlockStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->statements.size(), 2u);
}

// P-TC-12 : { { print a; } }  →  중첩 BlockStmt
TEST_F(ParserStmtTest, P_TC_12_NestedBlock)
{

    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
          makeTok(TokenType::LEFT_BRACE, "{"),
            makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
          makeTok(TokenType::RIGHT_BRACE, "}"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* outer = dynamic_cast<BlockStmt*>(stmts[0].get());
    ASSERT_NE(outer, nullptr);
    ASSERT_EQ(outer->statements.size(), 1u);
    EXPECT_NE(dynamic_cast<BlockStmt*>(outer->statements[0].get()), nullptr);
}

// P-TC-13 : { print a;  (닫는 중괄호 없음)  →  parse 오류
TEST_F(ParserStmtTest, P_TC_13_BlockMissingClosingBrace)
{

    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::LEFT_BRACE, "{"),
            makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
            makeEof()
        }),
        std::runtime_error
    );
}

// ================================================================
// IfStmt
// ================================================================

// P-TC-14 : if (a > 0) print a;  →  IfStmt, elseBranch == null
TEST_F(ParserStmtTest, P_TC_14_IfWithoutElse)
{

    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->condition,  nullptr);
    EXPECT_NE(s->thenBranch, nullptr);
    EXPECT_EQ(s->elseBranch, nullptr);
}

// P-TC-15 : if (a > 0) print a; else print b;  →  elseBranch != null
TEST_F(ParserStmtTest, P_TC_15_IfWithElse)
{

    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::ELSE, "else"),
        makeTok(TokenType::PRINT, "print"), makeId("b"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->elseBranch, nullptr);
}

// P-TC-16 : if (a > 0) { print a; }  →  thenBranch == BlockStmt
TEST_F(ParserStmtTest, P_TC_16_IfWithBlockBody)
{

    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->thenBranch.get()), nullptr);
}

// P-TC-17 : if (a > 0) { } else { }  →  thenBranch, elseBranch 모두 BlockStmt
TEST_F(ParserStmtTest, P_TC_17_IfElseBothBlocks)
{

    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeTok(TokenType::ELSE, "else"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->thenBranch.get()), nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->elseBranch.get()), nullptr);
}

// P-TC-18 : if (a > 0) if (b > 0) print b;  →  중첩 IfStmt
TEST_F(ParserStmtTest, P_TC_18_NestedIf)
{

    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("b"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("b"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* outer = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(outer, nullptr);
    EXPECT_NE(dynamic_cast<IfStmt*>(outer->thenBranch.get()), nullptr);
}

// P-TC-19 : if a > 0) print a;  (여는 괄호 없음)  →  parse 오류
TEST_F(ParserStmtTest, P_TC_19_IfMissingLeftParen)
{

    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::IF, "if"),
            makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
            makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-20 : if (a > 0 print a;  (닫는 괄호 없음)  →  parse 오류
TEST_F(ParserStmtTest, P_TC_20_IfMissingRightParen)
{

    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::IF, "if"),
            makeTok(TokenType::LEFT_PAREN, "("), makeId("a"),
            makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
            makeEof()
        }),
        std::runtime_error
    );
}

// ================================================================
// ForStmt
// ================================================================

// P-TC-21 : for (var i = 0; i < 3; i = i+1) print i;  →  모든 필드 != null
TEST_F(ParserStmtTest, P_TC_21_ForAllParts)
{

    auto stmts = p.parse({
        makeTok(TokenType::FOR, "for"),
        makeTok(TokenType::LEFT_PAREN, "("),
        makeTok(TokenType::VAR, "var"), makeId("i"),
        makeTok(TokenType::EQUAL, "="), makeNum(0.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<ForStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->init,      nullptr);
    EXPECT_NE(s->condition, nullptr);
    EXPECT_NE(s->increment, nullptr);
    EXPECT_NE(s->body,      nullptr);
}

// P-TC-22 : for (...) { print i; }  →  body == BlockStmt
TEST_F(ParserStmtTest, P_TC_22_ForWithBlockBody)
{

    auto stmts = p.parse({
        makeTok(TokenType::FOR, "for"),
        makeTok(TokenType::LEFT_PAREN, "("),
        makeTok(TokenType::VAR, "var"), makeId("i"),
        makeTok(TokenType::EQUAL, "="), makeNum(0.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<ForStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->body.get()), nullptr);
}

// P-TC-23 : condition 뒤 세미콜론 없음  →  parse 오류
TEST_F(ParserStmtTest, P_TC_23_ForMissingSemicolon)
{

    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::FOR, "for"),
            makeTok(TokenType::LEFT_PAREN, "("),
            makeTok(TokenType::VAR, "var"), makeId("i"),
            makeTok(TokenType::EQUAL, "="), makeNum(0.0),
            makeTok(TokenType::SEMICOLON, ";"),
            makeId("i"),
            makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-24 : for (...) {}  →  body == 빈 BlockStmt
TEST_F(ParserStmtTest, P_TC_24_ForWithEmptyBlock)
{

    auto stmts = p.parse({
        makeTok(TokenType::FOR, "for"),
        makeTok(TokenType::LEFT_PAREN, "("),
        makeTok(TokenType::VAR, "var"), makeId("i"),
        makeTok(TokenType::EQUAL, "="), makeNum(0.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<ForStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    auto* block = dynamic_cast<BlockStmt*>(s->body.get());
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->statements.size(), 0u);
}

// ================================================================
// 복수 문장
// ================================================================

// P-TC-25 : var a = 1; print a;  →  statements.size() == 2
TEST_F(ParserStmtTest, P_TC_25_MultipleStatements)
{

    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("a"),
        makeTok(TokenType::EQUAL, "="), makeNum(1.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    EXPECT_EQ(stmts.size(), 2u);
}

// P-TC-26 : EOF만 있는 경우  →  statements.size() == 0
TEST_F(ParserStmtTest, P_TC_26_EmptyInput)
{

    auto stmts = p.parse({ makeEof() });
    EXPECT_EQ(stmts.size(), 0u);
}

// ================================================================
// Negative UT
// ================================================================

// P-TC-27 : var = 1;  (이름 누락)  →  parse 오류
TEST_F(ParserStmtTest, VarMissingNameThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::VAR,       "var"),
            makeTok(TokenType::EQUAL,     "="),
            makeNum(1.0),
            makeTok(TokenType::SEMICOLON, ";"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-28 : for var i ...  (여는 괄호 없음)  →  parse 오류
TEST_F(ParserStmtTest, ForMissingLeftParenThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::FOR, "for"),
            makeTok(TokenType::VAR, "var"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-29 : for(var i=0; 1; 2  EOF  (닫는 괄호 없음)  →  parse 오류
TEST_F(ParserStmtTest, ForMissingRightParenThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::FOR,       "for"),
            makeTok(TokenType::LEFT_PAREN,"("),
            makeTok(TokenType::VAR,       "var"), makeId("i"),
            makeTok(TokenType::EQUAL,     "="), makeNum(0.0),
            makeTok(TokenType::SEMICOLON, ";"),
            makeNum(1.0), makeTok(TokenType::SEMICOLON, ";"),
            makeNum(2.0),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-30 : func (a) {}  (이름 누락)  →  parse 오류
TEST_F(ParserStmtTest, FuncMissingNameThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::FUNC,      "func"),
            makeTok(TokenType::LEFT_PAREN,"("),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-31 : func foo a) {}  (여는 괄호 없음)  →  parse 오류
TEST_F(ParserStmtTest, FuncMissingLeftParenThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::FUNC, "func"),
            makeId("foo"),
            makeId("a"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-32 : func foo(a)  EOF  (본문 없음)  →  parse 오류
TEST_F(ParserStmtTest, FuncMissingBodyThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::FUNC,       "func"),
            makeId("foo"),
            makeTok(TokenType::LEFT_PAREN, "("),
            makeId("a"),
            makeTok(TokenType::RIGHT_PAREN,")"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-33 : return 1  EOF  (세미콜론 없음)  →  parse 오류
TEST_F(ParserStmtTest, ReturnMissingSemicolonThrows)
{
    ASSERT_THROW(
        p.parse({
            makeTok(TokenType::RETURN, "return"),
            makeNum(1.0),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-TC-34 : var x;  (초기화 없음)  →  VarDeclareStmt, initializer == nullptr
TEST_F(ParserStmtTest, VarNoInitializerParsed)
{
    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("x"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "x");
    EXPECT_EQ(s->initializer, nullptr);
}

// P-TC-28 : if (a) {} else if (b) {}  →  elseBranch == IfStmt
TEST_F(ParserStmtTest, P_TC_28_ElseIfChaining)
{
    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeTok(TokenType::ELSE, "else"),
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("b"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* outer = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(outer->elseBranch, nullptr);
    EXPECT_NE(dynamic_cast<IfStmt*>(outer->elseBranch.get()), nullptr);
}

// P-TC-29 : for (i = 0; i; i) print i;  →  init == ExpressionStmt (var 없는 초기식)
TEST_F(ParserStmtTest, P_TC_29_ForExpressionStmtInit)
{
    auto stmts = p.parse({
        makeTok(TokenType::FOR, "for"),
        makeTok(TokenType::LEFT_PAREN, "("),
        makeId("i"), makeTok(TokenType::EQUAL, "="), makeNum(0.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<ForStmt*>(stmts[0].get());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(dynamic_cast<VarDeclareStmt*>(s->init.get()), nullptr);
    EXPECT_NE(dynamic_cast<ExpressionStmt*>(s->init.get()), nullptr);
    EXPECT_NE(s->condition,  nullptr);
    EXPECT_NE(s->increment,  nullptr);
    EXPECT_NE(s->body,       nullptr);
}
