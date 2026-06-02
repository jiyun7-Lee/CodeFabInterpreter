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
    Expr* parseExpression() override
    {
        auto* lit = new LiteralExpr();
        if (check(TokenType::NUMBER))
            lit->value = std::get<double>(advance().literal);
        else if (check(TokenType::STRING))
            lit->value = std::get<std::string>(advance().literal);
        else if (check(TokenType::TRUE) || check(TokenType::FALSE))
            lit->value = (advance().type == TokenType::TRUE);
        else if (check(TokenType::IDENTIFIER))
            advance();
        return lit;
    }
};

// ================================================================
// VarDeclareStmt
// ================================================================

// P-V-01 : var a = 3;  →  VarDeclareStmt, name="a", initializer != null
TEST(ParserStmtTest, P_V_01_VarWithNumber)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("a"),
        makeTok(TokenType::EQUAL, "="), makeNum(3.0),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "a");
    EXPECT_NE(s->initializer, nullptr);
}

// P-V-02 : var abc = "hello";  →  VarDeclareStmt, name="abc"
TEST(ParserStmtTest, P_V_02_VarWithString)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("abc"),
        makeTok(TokenType::EQUAL, "="), makeStr("hello"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "abc");
}

// P-V-03 : var flag = true;  →  VarDeclareStmt, name="flag"
TEST(ParserStmtTest, P_V_03_VarWithBool)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("flag"),
        makeTok(TokenType::EQUAL, "="), makeBoolTok(true),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name.lexeme, "flag");
    EXPECT_NE(s->initializer, nullptr);
}

// P-V-04 : var a = b + 1;  →  initializer != null  (FakeExprParser가 IDENTIFIER 소비)
TEST(ParserStmtTest, P_V_04_VarWithExpr)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("a"),
        makeTok(TokenType::EQUAL, "="), makeId("b"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    auto* s = dynamic_cast<VarDeclareStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->initializer, nullptr);
}

// P-V-05 : var a = 3  (세미콜론 없음)  →  parse 오류
TEST(ParserStmtTest, P_V_05_VarMissingSemicolon)
{
    FakeExprParser p;
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

// P-P-01 : print a;  →  PrintStmt, expression != null
TEST(ParserStmtTest, P_P_01_PrintVariable)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::PRINT, "print"), makeId("a"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<PrintStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->expression, nullptr);
}

// P-P-02 : print a + b;  →  expression != null  (FakeExprParser가 IDENTIFIER 소비)
TEST(ParserStmtTest, P_P_02_PrintExpression)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::PRINT, "print"), makeId("a"),
        makeTok(TokenType::SEMICOLON, ";"), makeEof()
    });
    EXPECT_NE(dynamic_cast<PrintStmt*>(stmts[0]), nullptr);
}

// P-P-03 : print a  (세미콜론 없음)  →  parse 오류
TEST(ParserStmtTest, P_P_03_PrintMissingSemicolon)
{
    FakeExprParser p;
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

// P-B-01 : {}  →  BlockStmt, statements.size() == 0
TEST(ParserStmtTest, P_B_01_EmptyBlock)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<BlockStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->statements.size(), 0u);
}

// P-B-02 : { print a; }  →  statements.size() == 1
TEST(ParserStmtTest, P_B_02_BlockOneStmt)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    auto* s = dynamic_cast<BlockStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->statements.size(), 1u);
}

// P-B-03 : { print a; print b; }  →  statements.size() == 2
TEST(ParserStmtTest, P_B_03_BlockMultipleStmts)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::PRINT, "print"), makeId("b"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    auto* s = dynamic_cast<BlockStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->statements.size(), 2u);
}

// P-B-04 : { { print a; } }  →  중첩 BlockStmt
TEST(ParserStmtTest, P_B_04_NestedBlock)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::LEFT_BRACE, "{"),
          makeTok(TokenType::LEFT_BRACE, "{"),
            makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
          makeTok(TokenType::RIGHT_BRACE, "}"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    auto* outer = dynamic_cast<BlockStmt*>(stmts[0]);
    ASSERT_NE(outer, nullptr);
    ASSERT_EQ(outer->statements.size(), 1u);
    EXPECT_NE(dynamic_cast<BlockStmt*>(outer->statements[0]), nullptr);
}

// P-B-05 : { print a;  (닫는 중괄호 없음)  →  parse 오류
TEST(ParserStmtTest, P_B_05_BlockMissingClosingBrace)
{
    FakeExprParser p;
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

// P-I-01 : if (a > 0) print a;  →  IfStmt, elseBranch == null
TEST(ParserStmtTest, P_I_01_IfWithoutElse)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<IfStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->condition,  nullptr);
    EXPECT_NE(s->thenBranch, nullptr);
    EXPECT_EQ(s->elseBranch, nullptr);
}

// P-I-02 : if (a > 0) print a; else print b;  →  elseBranch != null
TEST(ParserStmtTest, P_I_02_IfWithElse)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::ELSE, "else"),
        makeTok(TokenType::PRINT, "print"), makeId("b"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    auto* s = dynamic_cast<IfStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->elseBranch, nullptr);
}

// P-I-03 : if (a > 0) { print a; }  →  thenBranch == BlockStmt
TEST(ParserStmtTest, P_I_03_IfWithBlockBody)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    auto* s = dynamic_cast<IfStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->thenBranch), nullptr);
}

// P-I-04 : if (a > 0) { } else { }  →  thenBranch, elseBranch 모두 BlockStmt
TEST(ParserStmtTest, P_I_04_IfElseBothBlocks)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeTok(TokenType::ELSE, "else"),
        makeTok(TokenType::LEFT_BRACE, "{"), makeTok(TokenType::RIGHT_BRACE, "}"),
        makeEof()
    });
    auto* s = dynamic_cast<IfStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->thenBranch), nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->elseBranch), nullptr);
}

// P-I-05 : if (a > 0) if (b > 0) print b;  →  중첩 IfStmt
TEST(ParserStmtTest, P_I_05_NestedIf)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("a"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::IF, "if"),
        makeTok(TokenType::LEFT_PAREN, "("), makeId("b"), makeTok(TokenType::RIGHT_PAREN, ")"),
        makeTok(TokenType::PRINT, "print"), makeId("b"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    auto* outer = dynamic_cast<IfStmt*>(stmts[0]);
    ASSERT_NE(outer, nullptr);
    EXPECT_NE(dynamic_cast<IfStmt*>(outer->thenBranch), nullptr);
}

// P-I-06 : if a > 0) print a;  (여는 괄호 없음)  →  parse 오류
TEST(ParserStmtTest, P_I_06_IfMissingLeftParen)
{
    FakeExprParser p;
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

// P-I-07 : if (a > 0 print a;  (닫는 괄호 없음)  →  parse 오류
TEST(ParserStmtTest, P_I_07_IfMissingRightParen)
{
    FakeExprParser p;
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
// 토큰 구조: for ( <init_stmt> <cond_expr> ; <incr_expr> ) <body>
//   init_stmt : var 선언 (세미콜론 포함)
//   FakeExprParser 가 cond / incr 각각 IDENTIFIER 1개 소비
// ================================================================

// P-F-01 : for (var i = 0; i < 3; i = i+1) print i;  →  모든 필드 != null
TEST(ParserStmtTest, P_F_01_ForAllParts)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::FOR, "for"),
        makeTok(TokenType::LEFT_PAREN, "("),
        makeTok(TokenType::VAR, "var"), makeId("i"),
        makeTok(TokenType::EQUAL, "="), makeNum(0.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeId("i"), makeTok(TokenType::SEMICOLON, ";"),   // condition
        makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"), // increment
        makeTok(TokenType::PRINT, "print"), makeId("i"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* s = dynamic_cast<ForStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s->init,      nullptr);
    EXPECT_NE(s->condition, nullptr);
    EXPECT_NE(s->increment, nullptr);
    EXPECT_NE(s->body,      nullptr);
}

// P-F-02 : for (...) { print i; }  →  body == BlockStmt
TEST(ParserStmtTest, P_F_02_ForWithBlockBody)
{
    FakeExprParser p;
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
    auto* s = dynamic_cast<ForStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(dynamic_cast<BlockStmt*>(s->body), nullptr);
}

// P-F-03 : condition 뒤 세미콜론 없음  →  parse 오류
TEST(ParserStmtTest, P_F_03_ForMissingSemicolon)
{
    FakeExprParser p;
    EXPECT_THROW(
        p.parse({
            makeTok(TokenType::FOR, "for"),
            makeTok(TokenType::LEFT_PAREN, "("),
            makeTok(TokenType::VAR, "var"), makeId("i"),
            makeTok(TokenType::EQUAL, "="), makeNum(0.0),
            makeTok(TokenType::SEMICOLON, ";"),
            makeId("i"),          // condition 소비
            // SEMICOLON 없음
            makeId("i"), makeTok(TokenType::RIGHT_PAREN, ")"),
            makeEof()
        }),
        std::runtime_error
    );
}

// P-F-04 : for (...) {}  →  body == 빈 BlockStmt
TEST(ParserStmtTest, P_F_04_ForWithEmptyBlock)
{
    FakeExprParser p;
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
    auto* s = dynamic_cast<ForStmt*>(stmts[0]);
    ASSERT_NE(s, nullptr);
    auto* block = dynamic_cast<BlockStmt*>(s->body);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->statements.size(), 0u);
}

// ================================================================
// 복수 문장
// ================================================================

// P-M-01 : var a = 1; print a;  →  statements.size() == 2
TEST(ParserStmtTest, P_M_01_MultipleStatements)
{
    FakeExprParser p;
    auto stmts = p.parse({
        makeTok(TokenType::VAR, "var"), makeId("a"),
        makeTok(TokenType::EQUAL, "="), makeNum(1.0),
        makeTok(TokenType::SEMICOLON, ";"),
        makeTok(TokenType::PRINT, "print"), makeId("a"), makeTok(TokenType::SEMICOLON, ";"),
        makeEof()
    });
    EXPECT_EQ(stmts.size(), 2u);
}

// P-M-02 : EOF만 있는 경우  →  statements.size() == 0
TEST(ParserStmtTest, P_M_02_EmptyInput)
{
    FakeExprParser p;
    auto stmts = p.parse({ makeEof() });
    EXPECT_EQ(stmts.size(), 0u);
}
