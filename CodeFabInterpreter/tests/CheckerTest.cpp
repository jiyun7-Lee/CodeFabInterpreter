#include <gtest/gtest.h>
#include "../Checker.h"

// ================================================================
// AST 노드 생성 헬퍼  (Checker 테스트용)
// ================================================================
static Token idTok(const std::string& name, int line = 1)
{
    return Token{ TokenType::IDENTIFIER, name, line, std::monostate{} };
}

static std::unique_ptr<Expr> makeLit(double v = 0.0)
{
    auto e   = std::make_unique<LiteralExpr>();
    e->value = v;
    return e;
}

static std::unique_ptr<Expr> makeVar(const std::string& name, int line = 1)
{
    auto e  = std::make_unique<VariableExpr>();
    e->name = idTok(name, line);
    return e;
}

static std::unique_ptr<Expr> makeBin(std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
{
    auto e   = std::make_unique<BinaryExpr>();
    e->left  = std::move(left);
    e->op    = Token{ op, "", 1, std::monostate{} };
    e->right = std::move(right);
    return e;
}

static std::unique_ptr<Stmt> makeVarDecl(const std::string& name, std::unique_ptr<Expr> init = nullptr, int line = 1)
{
    auto s        = std::make_unique<VarDeclareStmt>();
    s->name       = idTok(name, line);
    s->initializer = std::move(init);
    return s;
}

static std::unique_ptr<Stmt> makeBlock(std::vector<std::unique_ptr<Stmt>> stmts)
{
    auto s       = std::make_unique<BlockStmt>();
    s->statements = std::move(stmts);
    return s;
}

static std::unique_ptr<Stmt> makeIf(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_, std::unique_ptr<Stmt> else_ = nullptr)
{
    auto s       = std::make_unique<IfStmt>();
    s->condition  = std::move(cond);
    s->thenBranch = std::move(then_);
    s->elseBranch = std::move(else_);
    return s;
}

// vector<unique_ptr<Stmt>> 인라인 빌더
// unique_ptr 는 복사 불가이므로 initializer_list 대신 사용
template<typename... Args>
static std::vector<std::unique_ptr<Stmt>> S(Args&&... args)
{
    std::vector<std::unique_ptr<Stmt>> v;
    (v.push_back(std::forward<Args>(args)), ...);
    return v;
}

// ================================================================
// C-D : 중복 선언
// ================================================================

// C-TC-01 : { var a = 1; var a = 2; }  →  Error
TEST(CheckerTest, C_TC_01_DuplicateVarSameBlock)
{
    Checker checker;
    EXPECT_FALSE(checker.check(S(
        makeBlock(S(
            makeVarDecl("a", makeLit(1.0)),
            makeVarDecl("a", makeLit(2.0))
        ))
    )));
}

// C-TC-02 : var a = 1; var a = 2;  (글로벌)  →  Error
TEST(CheckerTest, C_TC_02_DuplicateVarGlobal)
{
    Checker checker;
    EXPECT_FALSE(checker.check(S(
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("a", makeLit(2.0))
    )));
}

// C-TC-03 : var a = 1; { var a = 2; }  (shadowing)  →  OK
TEST(CheckerTest, C_TC_03_ShadowingAllowed)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeVarDecl("a", makeLit(1.0)),
        makeBlock(S(makeVarDecl("a", makeLit(2.0))))
    )));
}

// C-TC-04 : { var a = 1; } { var a = 2; }  (다른 블록)  →  OK
TEST(CheckerTest, C_TC_04_SameNameDifferentBlocks)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeBlock(S(makeVarDecl("a", makeLit(1.0)))),
        makeBlock(S(makeVarDecl("a", makeLit(2.0))))
    )));
}

// C-TC-05 : { var a = 1; var b = 2; }  →  OK
TEST(CheckerTest, C_TC_05_DifferentVarsSameBlock)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeBlock(S(
            makeVarDecl("a", makeLit(1.0)),
            makeVarDecl("b", makeLit(2.0))
        ))
    )));
}

// ================================================================
// C-S : 자기 초기화 참조
// ================================================================

// C-TC-06 : var a = a;  →  Error
TEST(CheckerTest, C_TC_06_SelfReference)
{
    Checker checker;
    EXPECT_FALSE(checker.check(S(
        makeVarDecl("a", makeVar("a"))
    )));
}

// C-TC-07 : var a = a + 1;  →  Error
TEST(CheckerTest, C_TC_07_SelfReferenceInBinary)
{
    Checker checker;
    EXPECT_FALSE(checker.check(S(
        makeVarDecl("a", makeBin(makeVar("a"), TokenType::PLUS, makeLit(1.0)))
    )));
}

// C-TC-08 : var a = 1; var b = a;  →  OK  (이미 선언된 변수 참조)
TEST(CheckerTest, C_TC_08_ReferenceAlreadyDeclared)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("b", makeVar("a"))
    )));
}

// C-TC-09 : var b = 1; var a = b;  →  OK
TEST(CheckerTest, C_TC_09_ReferenceOtherVar)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeVarDecl("b", makeLit(1.0)),
        makeVarDecl("a", makeVar("b"))
    )));
}

// ================================================================
// C-N : 정상 케이스
// ================================================================

// C-TC-10 : 빈 AST  →  OK
TEST(CheckerTest, C_TC_10_EmptyAST)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S()));
}

// C-TC-11 : var a = 1; var b = 2;  →  OK
TEST(CheckerTest, C_TC_11_MultipleDistinctVars)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("b", makeLit(2.0))
    )));
}

// C-TC-12 : if (true) { var x = 1; }  →  OK
TEST(CheckerTest, C_TC_12_VarInsideIfBlock)
{
    Checker checker;
    EXPECT_TRUE(checker.check(S(
        makeIf(
            makeLit(1.0),
            makeBlock(S(makeVarDecl("x", makeLit(1.0))))
        )
    )));
}
