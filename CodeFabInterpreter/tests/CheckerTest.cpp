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

static std::unique_ptr<Stmt> makeExprStmt(std::unique_ptr<Expr> expr)
{
    auto s = std::make_unique<ExpressionStmt>();
    s->expression = std::move(expr);
    return s;
}

static std::unique_ptr<Expr> makeFuncCall(const std::string& name,
                                           std::vector<std::unique_ptr<Expr>> args = {})
{
    auto e = std::make_unique<FunctionCallExpr>();
    e->callee = idTok(name);
    e->args   = std::move(args);
    return e;
}

static std::unique_ptr<Expr> makeArrayLiteral(std::vector<std::unique_ptr<Expr>> elems)
{
    auto e = std::make_unique<ArrayLiteralExpr>();
    e->elements = std::move(elems);
    return e;
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

// ================================================================
// reset() 동작 검증 (C-TC-24~25)
// ================================================================

// C-TC-24 : REPL 시뮬레이션 — reset() 후 스코프 초기화로 재선언 허용
// check() 는 scopes_/funcs_ 를 누적한다. reset() 만이 이를 초기화한다.
TEST_F(CheckerFixture, C_TC_24_ResetClearsScope)
{
    checker.check(S(makeVarDecl("a", makeLit(1.0))));               // a 스코프 등록
    EXPECT_FALSE(checker.check(S(makeVarDecl("a", makeLit(2.0))))); // 누적 스코프: 중복 에러
    checker.reset();
    EXPECT_TRUE(checker.check(S(makeVarDecl("a", makeLit(3.0)))));  // 리셋 후 재선언 OK
    EXPECT_TRUE(checker.getErrors().empty());
}

// C-TC-25 : reset() 후 함수 레지스트리 초기화 → 인자 개수 검사 없어짐
TEST_F(CheckerFixture, C_TC_25_ResetClearsFuncRegistry)
{
    // foo(a, b) 등록
    checker.check(S(makeFuncDecl("foo", {"a", "b"}, makeBlock(S()))));

    // 리셋 전: foo(1) → 인자 개수 불일치 에러
    std::vector<std::unique_ptr<Expr>> oneArg;
    oneArg.push_back(makeLit(1.0));
    EXPECT_FALSE(checker.check(S(makeExprStmt(makeFuncCall("foo", std::move(oneArg))))));

    // 리셋 후: funcs_ 소거 → foo 는 미등록 상태, Checker 에러 없음 (Runtime 이 담당)
    checker.reset();
    std::vector<std::unique_ptr<Expr>> oneArg2;
    oneArg2.push_back(makeLit(1.0));
    EXPECT_TRUE(checker.check(S(makeExprStmt(makeFuncCall("foo", std::move(oneArg2))))));
}

// ================================================================
// 배열 표현식 노드 — checkExpr + foldExpr 경로 검증 (C-TC-26~28)
// ArrayTest.cpp 의 runSource() 는 Checker 를 경유하지 않으므로
// 여기서 AST 를 직접 구성해 checkExpr/foldExpr 의 배열 브랜치를 커버한다.
// ================================================================

// C-TC-26 : ArrayLiteralExpr — checkExpr 원소 순회 + foldExpr 원소 폴딩
// var arr = [1+2, 3+4]; → elements folded to [3.0, 7.0]
TEST_F(CheckerFixture, C_TC_26_ArrayLiteralElementsFolded)
{
    std::vector<std::unique_ptr<Expr>> elems;
    elems.push_back(makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0)));
    elems.push_back(makeBin(makeLit(3.0), TokenType::PLUS, makeLit(4.0)));

    auto arrLit  = makeArrayLiteral(std::move(elems));
    auto* rawArr = dynamic_cast<ArrayLiteralExpr*>(arrLit.get());

    auto stmts = S(makeVarDecl("arr", std::move(arrLit)));
    EXPECT_TRUE(checker.check(stmts));

    ASSERT_NE(rawArr, nullptr);
    ASSERT_EQ(rawArr->elements.size(), 2u);
    auto* e0 = dynamic_cast<LiteralExpr*>(rawArr->elements[0].get());
    auto* e1 = dynamic_cast<LiteralExpr*>(rawArr->elements[1].get());
    ASSERT_NE(e0, nullptr);
    ASSERT_NE(e1, nullptr);
    EXPECT_EQ(std::get<double>(e0->value), 3.0);
    EXPECT_EQ(std::get<double>(e1->value), 7.0);
}

// C-TC-27 : ArrayAccessExpr — checkExpr 순회 + foldExpr index 폴딩
// arr[1+2] → index folded to 3.0
TEST_F(CheckerFixture, C_TC_27_ArrayAccessIndexFolded)
{
    auto accessExpr    = std::make_unique<ArrayAccessExpr>();
    accessExpr->array  = makeVar("arr");
    accessExpr->index  = makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0));
    auto* rawAccess    = accessExpr.get();

    auto stmts = S(
        makeVarDecl("arr", makeLit(0.0)),
        makeExprStmt(std::move(accessExpr))
    );
    EXPECT_TRUE(checker.check(stmts));

    auto* idx = dynamic_cast<LiteralExpr*>(rawAccess->index.get());
    ASSERT_NE(idx, nullptr);
    EXPECT_EQ(std::get<double>(idx->value), 3.0);
}

// C-TC-28 : ArrayWriteExpr — checkExpr 순회 + foldExpr value/index 폴딩
// arr[0] = 1+2; → value folded to 3.0
TEST_F(CheckerFixture, C_TC_28_ArrayWriteValueFolded)
{
    auto writeExpr    = std::make_unique<ArrayWriteExpr>();
    writeExpr->array  = makeVar("arr");
    writeExpr->index  = makeLit(0.0);
    writeExpr->value  = makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0));
    auto* rawWrite    = writeExpr.get();

    auto stmts = S(
        makeVarDecl("arr", makeLit(0.0)),
        makeExprStmt(std::move(writeExpr))
    );
    EXPECT_TRUE(checker.check(stmts));

    auto* val = dynamic_cast<LiteralExpr*>(rawWrite->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(std::get<double>(val->value), 3.0);
}
