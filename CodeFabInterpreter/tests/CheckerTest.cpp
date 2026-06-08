#include <gtest/gtest.h>
#include "../Checker.h"

// ================================================================
// AST 노드 생성 헬퍼
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

static std::unique_ptr<Expr> makeBin(std::unique_ptr<Expr> left, TokenType op,
                                      std::unique_ptr<Expr> right)
{
    auto e   = std::make_unique<BinaryExpr>();
    e->left  = std::move(left);
    e->op    = Token{ op, "", 1, std::monostate{} };
    e->right = std::move(right);
    return e;
}

static std::unique_ptr<Expr> makeGrouping(std::unique_ptr<Expr> inner)
{
    auto e        = std::make_unique<GroupingExpr>();
    e->expression = std::move(inner);
    return e;
}

static std::unique_ptr<Expr> makeUnary(TokenType op, std::unique_ptr<Expr> right)
{
    auto e   = std::make_unique<UnaryExpr>();
    e->op    = Token{ op, "", 1, std::monostate{} };
    e->right = std::move(right);
    return e;
}

static std::unique_ptr<Stmt> makeVarDecl(const std::string& name,
                                          std::unique_ptr<Expr> init = nullptr,
                                          int line = 1)
{
    auto s         = std::make_unique<VarDeclareStmt>();
    s->name        = idTok(name, line);
    s->initializer = std::move(init);
    return s;
}

static std::unique_ptr<Stmt> makeBlock(std::vector<std::unique_ptr<Stmt>> stmts)
{
    auto s        = std::make_unique<BlockStmt>();
    s->statements = std::move(stmts);
    return s;
}

static std::unique_ptr<Stmt> makeIf(std::unique_ptr<Expr> cond,
                                     std::unique_ptr<Stmt> then_,
                                     std::unique_ptr<Stmt> else_ = nullptr)
{
    auto s        = std::make_unique<IfStmt>();
    s->condition  = std::move(cond);
    s->thenBranch = std::move(then_);
    s->elseBranch = std::move(else_);
    return s;
}

static std::unique_ptr<Stmt> makeReturn(std::unique_ptr<Expr> val = nullptr)
{
    auto s   = std::make_unique<ReturnStmt>();
    s->value = std::move(val);
    return s;
}

static std::unique_ptr<Stmt> makeFuncDecl(const std::string& name,
                                           std::vector<std::string> paramNames,
                                           std::unique_ptr<Stmt> body)
{
    auto s  = std::make_unique<FunctionDeclareStmt>();
    s->name = idTok(name);
    for (const auto& pn : paramNames)
        s->params.push_back(idTok(pn));
    s->body = std::move(body);
    return s;
}

static std::unique_ptr<Stmt> makeFor(std::unique_ptr<Stmt> init,
                                      std::unique_ptr<Expr> cond,
                                      std::unique_ptr<Expr> inc,
                                      std::unique_ptr<Stmt> body)
{
    auto s       = std::make_unique<ForStmt>();
    s->init      = std::move(init);
    s->condition = std::move(cond);
    s->increment = std::move(inc);
    s->body      = std::move(body);
    return s;
}

template<typename... Args>
static std::vector<std::unique_ptr<Stmt>> S(Args&&... args)
{
    std::vector<std::unique_ptr<Stmt>> v;
    (v.push_back(std::forward<Args>(args)), ...);
    return v;
}

// ================================================================
// Fixture
// ================================================================
class CheckerFixture : public ::testing::Test
{
protected:
    Checker checker;

    void checkExpectError(std::vector<std::unique_ptr<Stmt>> stmts)
    {
        EXPECT_FALSE(checker.check(stmts));
        EXPECT_GE(checker.getErrors().size(), 1u);
    }

    void checkExpectOk(std::vector<std::unique_ptr<Stmt>> stmts)
    {
        EXPECT_TRUE(checker.check(stmts));
    }
};

// ================================================================
// 중복 선언 (C-TC-01~05)
// ================================================================

// C-TC-01 : { var a = 1; var a = 2; }  →  Error
TEST_F(CheckerFixture, C_TC_01_DuplicateVarSameBlock)
{
    checkExpectError(S(
        makeBlock(S(
            makeVarDecl("a", makeLit(1.0)),
            makeVarDecl("a", makeLit(2.0))
        ))
    ));
}

// C-TC-02 : var a = 1; var a = 2;  (글로벌)  →  Error
TEST_F(CheckerFixture, C_TC_02_DuplicateVarGlobal)
{
    checkExpectError(S(
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("a", makeLit(2.0))
    ));
}

// C-TC-03 : var a = 1; { var a = 2; }  (shadowing)  →  OK
TEST_F(CheckerFixture, C_TC_03_ShadowingAllowed)
{
    checkExpectOk(S(
        makeVarDecl("a", makeLit(1.0)),
        makeBlock(S(makeVarDecl("a", makeLit(2.0))))
    ));
}

// C-TC-04 : { var a = 1; } { var a = 2; }  (다른 블록)  →  OK
TEST_F(CheckerFixture, C_TC_04_SameNameDifferentBlocks)
{
    checkExpectOk(S(
        makeBlock(S(makeVarDecl("a", makeLit(1.0)))),
        makeBlock(S(makeVarDecl("a", makeLit(2.0))))
    ));
}

// C-TC-05 : { var a = 1; var b = 2; }  →  OK
TEST_F(CheckerFixture, C_TC_05_DifferentVarsSameBlock)
{
    checkExpectOk(S(
        makeBlock(S(
            makeVarDecl("a", makeLit(1.0)),
            makeVarDecl("b", makeLit(2.0))
        ))
    ));
}

// ================================================================
// 자기 초기화 참조 (C-TC-06~09)
// ================================================================

// C-TC-06 : var a = a;  →  Error
TEST_F(CheckerFixture, C_TC_06_SelfReference)
{
    checkExpectError(S(makeVarDecl("a", makeVar("a"))));
}

// C-TC-07 : var a = a + 1;  →  Error
TEST_F(CheckerFixture, C_TC_07_SelfReferenceInBinary)
{
    checkExpectError(S(
        makeVarDecl("a", makeBin(makeVar("a"), TokenType::PLUS, makeLit(1.0)))
    ));
}

// C-TC-08 : var a = 1; var b = a;  →  OK  (이미 선언된 변수 참조)
TEST_F(CheckerFixture, C_TC_08_ReferenceAlreadyDeclared)
{
    checkExpectOk(S(
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("b", makeVar("a"))
    ));
}

// C-TC-09 : var b = 1; var a = b;  →  OK
TEST_F(CheckerFixture, C_TC_09_ReferenceOtherVar)
{
    checkExpectOk(S(
        makeVarDecl("b", makeLit(1.0)),
        makeVarDecl("a", makeVar("b"))
    ));
}

// ================================================================
// 정상 케이스 (C-TC-10~12)
// ================================================================

// C-TC-10 : 빈 AST  →  OK
TEST_F(CheckerFixture, C_TC_10_EmptyAST)
{
    checkExpectOk(S());
}

// C-TC-11 : var a = 1; var b = 2;  →  OK
TEST_F(CheckerFixture, C_TC_11_MultipleDistinctVars)
{
    checkExpectOk(S(
        makeVarDecl("a", makeLit(1.0)),
        makeVarDecl("b", makeLit(2.0))
    ));
}

// C-TC-12 : if (true) { var x = 1; }  →  OK
TEST_F(CheckerFixture, C_TC_12_VarInsideIfBlock)
{
    checkExpectOk(S(
        makeIf(
            makeLit(1.0),
            makeBlock(S(makeVarDecl("x", makeLit(1.0))))
        )
    ));
}

// ================================================================
// 자기 참조 심화 / 함수 / 제어 흐름 (C-TC-13~23)
// ================================================================

// C-TC-13 : var a = (a + 1);  →  Error (GroupingExpr 내 자기 참조)
TEST_F(CheckerFixture, C_TC_13_SelfReferenceInGrouping)
{
    checkExpectError(S(
        makeVarDecl("a",
            makeGrouping(makeBin(makeVar("a"), TokenType::PLUS, makeLit(1.0))))
    ));
}

// C-TC-14 : var a = -a;  →  Error (UnaryExpr 내 자기 참조)
TEST_F(CheckerFixture, C_TC_14_SelfReferenceInUnary)
{
    checkExpectError(S(
        makeVarDecl("a", makeUnary(TokenType::MINUS, makeVar("a")))
    ));
}

// C-TC-15 : { return 5; }  (함수 외부 블록)  →  Error
TEST_F(CheckerFixture, C_TC_15_ReturnInNestedBlockOutsideFunc)
{
    checkExpectError(S(
        makeBlock(S(makeReturn(makeLit(5.0))))
    ));
}

// C-TC-16 : func foo(a, b, a) {}  (비연속 파라미터 중복)  →  Error
TEST_F(CheckerFixture, C_TC_16_DuplicateParamInFuncExtended)
{
    checkExpectError(S(
        makeFuncDecl("foo", {"a", "b", "a"}, makeBlock(S()))
    ));
}

// C-TC-17 : if 블록 내부 중복 var  →  Error
TEST_F(CheckerFixture, C_TC_17_DuplicateVarInsideIf)
{
    checkExpectError(S(
        makeIf(
            makeLit(1.0),
            makeBlock(S(
                makeVarDecl("x", makeLit(1.0)),
                makeVarDecl("x", makeLit(2.0))
            ))
        )
    ));
}

// C-TC-18 : for 바디 블록 내부 중복 var  →  Error
TEST_F(CheckerFixture, C_TC_18_DuplicateVarInsideFor)
{
    checkExpectError(S(
        makeFor(
            makeVarDecl("i", makeLit(0.0)),
            makeLit(1.0),
            makeLit(0.0),
            makeBlock(S(
                makeVarDecl("x", makeLit(1.0)),
                makeVarDecl("x", makeLit(2.0))
            ))
        )
    ));
}

// C-TC-19 : for init 에서 자기 참조 var  →  Error
TEST_F(CheckerFixture, C_TC_19_ForInitSelfReference)
{
    checkExpectError(S(
        makeFor(
            makeVarDecl("i", makeVar("i")),
            makeLit(1.0),
            makeLit(0.0),
            makeBlock(S())
        )
    ));
}

// C-TC-20 : { { { var a=1; var a=2; } } }  →  Error (3단계 중첩에서도 중복 감지)
TEST_F(CheckerFixture, C_TC_20_NestedBlock3LevelDuplicate)
{
    checkExpectError(S(
        makeBlock(S(
            makeBlock(S(
                makeBlock(S(
                    makeVarDecl("a", makeLit(1.0)),
                    makeVarDecl("a", makeLit(2.0))
                ))
            ))
        ))
    ));
}

// C-TC-21 : var a;  (초기값 없음)  →  OK (자기 참조 검사 대상 없음)
TEST_F(CheckerFixture, C_TC_21_NoInitializerNoError)
{
    checkExpectOk(S(makeVarDecl("a")));
}

// C-TC-22 : for (var i=0; ...) {} 이후 var i=1;  →  OK (for 스코프 정리 후 재선언 가능)
TEST_F(CheckerFixture, C_TC_22_ForScopeCleanupAllowsRedecl)
{
    checkExpectOk(S(
        makeFor(
            makeVarDecl("i", makeLit(0.0)),
            makeLit(1.0),
            makeLit(0.0),
            makeBlock(S())
        ),
        makeVarDecl("i", makeLit(1.0))
    ));
}

// C-TC-23 : { var a=1; if(x){ var a=2; } }  →  OK (중첩 if 내 shadowing 허용)
TEST_F(CheckerFixture, C_TC_23_NestedIfShadowing)
{
    checkExpectOk(S(
        makeBlock(S(
            makeVarDecl("a", makeLit(1.0)),
            makeIf(
                makeLit(1.0),
                makeBlock(S(makeVarDecl("a", makeLit(2.0))))
            )
        ))
    ));
}
