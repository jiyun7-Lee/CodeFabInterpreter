// OptimizationTest.cpp — Chapter 4: 정적 바인딩(SB) 테스트
#include <gtest/gtest.h>
#include "../Checker.h"
#include "../Shell.h"

// ================================================================
// AST 노드 생성 헬퍼
// ================================================================
static Token idTok(const std::string& name, int line = 1)
{
    return Token{TokenType::IDENTIFIER, name, line, std::monostate{}};
}

static std::unique_ptr<Expr> makeLit(double v = 0.0)
{
    auto e = std::make_unique<LiteralExpr>();
    e->value = v;
    return e;
}

static std::unique_ptr<Expr> makeVar(const std::string& name, int line = 1)
{
    auto e = std::make_unique<VariableExpr>();
    e->name = idTok(name, line);
    return e;
}

static std::unique_ptr<Expr> makeAssignExpr(const std::string& name,
                                             std::unique_ptr<Expr> val,
                                             int line = 1)
{
    auto e = std::make_unique<AssignExpr>();
    e->name  = idTok(name, line);
    e->value = std::move(val);
    return e;
}

static std::unique_ptr<Stmt> makeVarDecl(const std::string& name,
                                          std::unique_ptr<Expr> init = nullptr,
                                          int line = 1)
{
    auto s = std::make_unique<VarDeclareStmt>();
    s->name        = idTok(name, line);
    s->initializer = std::move(init);
    return s;
}

static std::unique_ptr<Stmt> makePrint(std::unique_ptr<Expr> expr)
{
    auto s = std::make_unique<PrintStmt>();
    s->expression = std::move(expr);
    return s;
}

static std::unique_ptr<Stmt> makeExprStmt(std::unique_ptr<Expr> expr)
{
    auto s = std::make_unique<ExpressionStmt>();
    s->expression = std::move(expr);
    return s;
}

static std::unique_ptr<Stmt> makeBlock(std::vector<std::unique_ptr<Stmt>> ss)
{
    auto s = std::make_unique<BlockStmt>();
    s->statements = std::move(ss);
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
// 정적 바인딩 — Checker distance 행동 검증 (Test Double: AST 직접 검사)
//
// Checker 가 VariableExpr / AssignExpr 의 distance 필드를 올바르게
// 기록하는지 확인한다.  distance 값이 맞아야 Executor 가 O(1) 으로
// 올바른 스코프에 접근할 수 있다.
// ================================================================

// SB-TC-01 : 글로벌 스코프 변수 참조 → distance = 0
// var x = 1;  print x;
// scopes = [{x}], x at index 0 → distance = 1-1-0 = 0
TEST(OptimizationTest, SB_TC_01_GlobalVar_Distance0)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makePrint(std::move(varExprPtr))
    );

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 0);
}

// SB-TC-02 : 블록 1단계 안에서 외부 변수 참조 → distance = 1
// var x = 1;  { print x; }
// scopes = [{x}, {}(block)], x at index 0 → distance = 2-1-0 = 1
TEST(OptimizationTest, SB_TC_02_OneBlockDeep_Distance1)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makePrint(std::move(varExprPtr))))
    );

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 1);
}

// SB-TC-03 : 블록 2단계 안에서 외부 변수 참조 → distance = 2
// var x = 1;  { { print x; } }
// scopes = [{x}, {}, {}], x at index 0 → distance = 3-1-0 = 2
TEST(OptimizationTest, SB_TC_03_TwoBlocksDeep_Distance2)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeBlock(S(makePrint(std::move(varExprPtr))))))
    );

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 2);
}

// SB-TC-04 : 내부 블록에서 섀도잉된 변수 참조 → distance = 0 (자기 스코프)
// var x = 1;  { var x = 2;  print x; }
// 내부 x 선언 후 참조 → 자기 스코프 distance = 0
TEST(OptimizationTest, SB_TC_04_ShadowedVar_Distance0)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(
            makeVarDecl("x", makeLit(2.0)),
            makePrint(std::move(varExprPtr))
        ))
    );

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 0);
}

// SB-TC-05 : 대입 표현식(AssignExpr) 대상 변수 distance 검증
// var x = 1;  { x = 2; }
// x 는 한 블록 바깥 → AssignExpr.distance = 1
TEST(OptimizationTest, SB_TC_05_AssignExpr_Distance1)
{
    auto assignPtr = makeAssignExpr("x", makeLit(2.0));
    auto* rawAssign = dynamic_cast<AssignExpr*>(assignPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeExprStmt(std::move(assignPtr))))
    );

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawAssign, nullptr);
    EXPECT_EQ(rawAssign->distance, 1);
}

// SB-TC-06 : 미선언 변수 참조 → distance = -1 유지 (스코프 탐색 실패)
// print y;  (y 미선언)
// 어떤 스코프에도 없으므로 distance 는 초기값 -1 유지
TEST(OptimizationTest, SB_TC_06_UndeclaredVar_DistanceMinus1)
{
    auto varExprPtr = makeVar("y");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(makePrint(std::move(varExprPtr)));

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, -1);
}

// SB-TC-07 : 같은 이름 변수가 여러 블록에 존재할 때 가장 가까운 스코프 선택
// var x = 1;  { var x = 2;  { print x; } }
// 내부 블록에서 x 참조 → 중간 블록의 x (index 1) → distance = 3-1-1 = 1
TEST(OptimizationTest, SB_TC_07_NearestScope_Selected)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(
            makeVarDecl("x", makeLit(2.0)),
            makeBlock(S(makePrint(std::move(varExprPtr))))
        ))
    );

    Checker checker;
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 1);
}

// ================================================================
// 정적 바인딩 End-to-End (Shell 경유 — 실행 결과로 정확성 검증)
// ================================================================

// SB-TC-08 : 중첩 블록에서 외부 변수 읽기 → 올바른 값 반환
// var x = 10;  { print x; }  →  "10"
TEST(OptimizationTest, SB_TC_08_NestedBlock_ReadsOuterVar)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var x = 10;");
    shell.runLine("{ print x; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// SB-TC-09 : for 루프 조건에서 외부 변수 참조 → 루프 정상 실행
// var limit = 3;  for (var i = 0; i < limit; i = i+1) { print i; }
// ForStmt forEnv 가 도입되어 Checker distance 와 런타임 체인이 일치
TEST(OptimizationTest, SB_TC_09_ForLoop_OuterVarInCondition)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var limit = 3;");
    shell.runLine("for (var i = 0; i < limit; i = i + 1) { print i; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "0\n1\n2\n");
}
