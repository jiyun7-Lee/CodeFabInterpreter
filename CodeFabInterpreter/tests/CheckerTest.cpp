#include <gtest/gtest.h>
#include "../Checker.h"

// ================================================================
// AST 노드 생성 헬퍼  (Checker 테스트용)
// ================================================================
static Token idTok(const std::string& name, int line = 1)
{
    return Token{ TokenType::IDENTIFIER, name, line, std::monostate{} };
}

static LiteralExpr* makeLit(double v = 0.0)
{
    auto* e = new LiteralExpr();
    e->value = v;
    return e;
}

static VariableExpr* makeVar(const std::string& name, int line = 1)
{
    auto* e = new VariableExpr();
    e->name = idTok(name, line);
    return e;
}

static BinaryExpr* makeBin(Expr* left, TokenType op, Expr* right)
{
    auto* e = new BinaryExpr();
    e->left  = left;
    e->op    = Token{ op, "", 1, std::monostate{} };
    e->right = right;
    return e;
}

static VarDeclareStmt* makeVarDecl(const std::string& name, Expr* init = nullptr, int line = 1)
{
    auto* s = new VarDeclareStmt();
    s->name        = idTok(name, line);
    s->initializer = init;
    return s;
}

static BlockStmt* makeBlock(std::vector<Stmt*> stmts)
{
    auto* s = new BlockStmt();
    s->statements = std::move(stmts);
    return s;
}

static IfStmt* makeIf(Expr* cond, Stmt* then_, Stmt* else_ = nullptr)
{
    auto* s = new IfStmt();
    s->condition  = cond;
    s->thenBranch = then_;
    s->elseBranch = else_;
    return s;
}

static ForStmt* makeFor(Stmt* init, Expr* cond, Expr* incr, Stmt* body)
{
    auto* s = new ForStmt();
    s->init      = init;
    s->condition = cond;
    s->increment = incr;
    s->body      = body;
    return s;
}

// ================================================================
// C-D : 중복 선언
// ================================================================

// C-D-01 : { var a = 1; var a = 2; }  →  Error
TEST(CheckerTest, C_D_01_DuplicateVarSameBlock)
{
    Checker checker;
    EXPECT_FALSE(checker.check({
        makeBlock({
            makeVarDecl("a", makeLit(1.0)),
            makeVarDecl("a", makeLit(2.0))
        })
    }));
}

// C-D-02 : var a = 1; var a = 2;  (글로벌)  →  Error
TEST(CheckerTest, C_D_02_DuplicateVarGlobal)
{
    Checker checker;
    EXPECT_FALSE(checker.check({
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("a", makeLit(2.0))
    }));
}

// C-D-03 : var a = 1; { var a = 2; }  (shadowing)  →  OK
TEST(CheckerTest, C_D_03_ShadowingAllowed)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeVarDecl("a", makeLit(1.0)),
        makeBlock({ makeVarDecl("a", makeLit(2.0)) })
    }));
}

// C-D-04 : { var a = 1; } { var a = 2; }  (다른 블록)  →  OK
TEST(CheckerTest, C_D_04_SameNameDifferentBlocks)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeBlock({ makeVarDecl("a", makeLit(1.0)) }),
        makeBlock({ makeVarDecl("a", makeLit(2.0)) })
    }));
}

// C-D-05 : { var a = 1; var b = 2; }  →  OK
TEST(CheckerTest, C_D_05_DifferentVarsSameBlock)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeBlock({
            makeVarDecl("a", makeLit(1.0)),
            makeVarDecl("b", makeLit(2.0))
        })
    }));
}

// ================================================================
// C-S : 자기 초기화 참조
// ================================================================

// C-S-01 : var a = a;  →  Error
TEST(CheckerTest, C_S_01_SelfReference)
{
    Checker checker;
    EXPECT_FALSE(checker.check({
        makeVarDecl("a", makeVar("a"))
    }));
}

// C-S-02 : var a = a + 1;  →  Error
TEST(CheckerTest, C_S_02_SelfReferenceInBinary)
{
    Checker checker;
    EXPECT_FALSE(checker.check({
        makeVarDecl("a", makeBin(makeVar("a"), TokenType::PLUS, makeLit(1.0)))
    }));
}

// C-S-03 : var a = 1; var b = a;  →  OK  (이미 선언된 변수 참조)
TEST(CheckerTest, C_S_03_ReferenceAlreadyDeclared)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("b", makeVar("a"))
    }));
}

// C-S-04 : var b = 1; var a = b;  →  OK
TEST(CheckerTest, C_S_04_ReferenceOtherVar)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeVarDecl("b", makeLit(1.0)),
        makeVarDecl("a", makeVar("b"))
    }));
}

// ================================================================
// C-N : 정상 케이스
// ================================================================

// C-N-01 : 빈 AST  →  OK
TEST(CheckerTest, C_N_01_EmptyAST)
{
    Checker checker;
    EXPECT_TRUE(checker.check({}));
}

// C-N-02 : var a = 1; var b = 2;  →  OK
TEST(CheckerTest, C_N_02_MultipleDistinctVars)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("b", makeLit(2.0))
    }));
}

// C-N-03 : if (true) { var x = 1; }  →  OK
TEST(CheckerTest, C_N_03_VarInsideIfBlock)
{
    Checker checker;
    EXPECT_TRUE(checker.check({
        makeIf(
            makeLit(1.0),
            makeBlock({ makeVarDecl("x", makeLit(1.0)) })
        )
    }));
}
