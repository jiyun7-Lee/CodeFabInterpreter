// OptimizationTest.cpp — Chapter 4: 정적 바인딩(SB) 테스트
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Checker.h"
#include "../Executor.h"
#include "../Shell.h"

// ================================================================
// MockEnvironment — get / getAt 호출 행위를 검증하는 Test Double
//
// PDF 15p 요구사항:
//   정적 바인딩: 스코프를 거슬러 올라가지 않고 계산된 위치로 즉시 접근했는지 검증
//   → distance >= 0 이면 getAt(d, name) 경로만 사용, get() 은 호출되지 않아야 한다.
// ================================================================
class MockEnvironment : public Environment
{
public:
    MOCK_METHOD(Value, get,   (const std::string& name, int line), (const, override));
    MOCK_METHOD(Value, getAt, (int distance, const std::string& name), (const, override));

    // 실제 구현 위임용 헬퍼 (ON_CALL WillByDefault 에서 사용)
    Value realGet(const std::string& name, int line = 0) const
    { return Environment::get(name, line); }
    Value realGetAt(int distance, const std::string& name) const
    { return Environment::getAt(distance, name); }
};

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

// SB-TC-10 : 글로벌 스코프 변수 참조 시 getAt(0) 호출, get() 미호출 (Test Double 행위 검증)
// var x = 1; print x;
// distance=0 → mock 이 globalEnv 역할 → mock.getAt(0, "x") 호출 / mock.get 미호출
TEST(OptimizationTest, SB_TC_10_GlobalVar_UsesGetAt_NotGet)
{
    MockEnvironment mock;

    ON_CALL(mock, getAt(testing::_, testing::_))
        .WillByDefault([&mock](int d, const std::string& n) { return mock.realGetAt(d, n); });
    ON_CALL(mock, get(testing::_, testing::_))
        .WillByDefault([&mock](const std::string& n, int line) { return mock.realGet(n, line); });

    EXPECT_CALL(mock, getAt(0, std::string("x"))).Times(1);
    EXPECT_CALL(mock, get(testing::_, testing::_)).Times(0);

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makePrint(makeVar("x"))
    );
    Checker checker;
    checker.check(stmts);

    Executor exec;
    exec.execute(stmts, mock);
}

// SB-TC-11 : 블록 내부에서 외부 변수 참조 시 체인 순회(get) 미호출 (Test Double 행위 검증)
// var x = 1; { print x; }
// distance=1 → blockEnv.getAt(1,"x") 로 즉시 접근, mock.get 은 호출되지 않아야 한다.
TEST(OptimizationTest, SB_TC_11_BlockVar_NoChainTraversal)
{
    MockEnvironment mock;

    ON_CALL(mock, get(testing::_, testing::_))
        .WillByDefault([&mock](const std::string& n, int line) { return mock.realGet(n, line); });

    EXPECT_CALL(mock, get(testing::_, testing::_)).Times(0);

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makePrint(makeVar("x"))))
    );
    Checker checker;
    checker.check(stmts);

    Executor exec;
    exec.execute(stmts, mock);
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

// ================================================================
// 상수 폴딩 헬퍼
// ================================================================
static std::unique_ptr<Expr> makeBin(std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
{
    auto e   = std::make_unique<BinaryExpr>();
    e->left  = std::move(left);
    e->op    = Token{op, "", 1, std::monostate{}};
    e->right = std::move(right);
    return e;
}

static std::unique_ptr<Expr> makeUnary(TokenType op, std::unique_ptr<Expr> right)
{
    auto e   = std::make_unique<UnaryExpr>();
    e->op    = Token{op, "", 1, std::monostate{}};
    e->right = std::move(right);
    return e;
}

static std::unique_ptr<Expr> makeGrouping(std::unique_ptr<Expr> inner)
{
    auto e        = std::make_unique<GroupingExpr>();
    e->expression = std::move(inner);
    return e;
}

static bool isLiteralDouble(const std::unique_ptr<Expr>& e, double expected)
{
    auto* lit = dynamic_cast<LiteralExpr*>(e.get());
    return lit && std::holds_alternative<double>(lit->value)
               && std::get<double>(lit->value) == expected;
}

static bool isLiteralBool(const std::unique_ptr<Expr>& e, bool expected)
{
    auto* lit = dynamic_cast<LiteralExpr*>(e.get());
    return lit && std::holds_alternative<bool>(lit->value)
               && std::get<bool>(lit->value) == expected;
}

// BinaryExpr 노드 수를 재귀적으로 센다 — 상수 폴딩 전/후 계산 횟수 검증용
static int countBinaryExprInExpr(const Expr* e)
{
    if (!e) return 0;
    if (auto* bin = dynamic_cast<const BinaryExpr*>(e))
        return 1
            + countBinaryExprInExpr(bin->left.get())
            + countBinaryExprInExpr(bin->right.get());
    if (auto* grp = dynamic_cast<const GroupingExpr*>(e))
        return countBinaryExprInExpr(grp->expression.get());
    if (auto* unary = dynamic_cast<const UnaryExpr*>(e))
        return countBinaryExprInExpr(unary->right.get());
    return 0;
}

static int countBinaryExpr(const std::vector<std::unique_ptr<Stmt>>& stmts)
{
    int total = 0;
    for (const auto& s : stmts)
    {
        if (auto* d = dynamic_cast<const VarDeclareStmt*>(s.get()))
            total += countBinaryExprInExpr(d->initializer.get());
        else if (auto* p = dynamic_cast<const PrintStmt*>(s.get()))
            total += countBinaryExprInExpr(p->expression.get());
        else if (auto* e = dynamic_cast<const ExpressionStmt*>(s.get()))
            total += countBinaryExprInExpr(e->expression.get());
    }
    return total;
}

// ================================================================
// 상수 폴딩 — foldExpr 동작 검증 (Test Double: AST 직접 검사)
//
// check() 호출 후 AST 노드가 LiteralExpr 로 교체되거나
// 항등식에 의해 단순화됐는지 확인한다.
// ================================================================

// CF-TC-01 : var x = 1.0 + 2.0;  →  initializer = LiteralExpr{3.0}  (패턴 A: +)
TEST(OptimizationTest, CF_TC_01_BinaryPlus_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 3.0));
}

// CF-TC-02 : var x = 10.0 - 3.0;  →  LiteralExpr{7.0}  (패턴 A: -)
TEST(OptimizationTest, CF_TC_02_BinaryMinus_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(10.0), TokenType::MINUS, makeLit(3.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 7.0));
}

// CF-TC-03 : var x = 4.0 * 5.0;  →  LiteralExpr{20.0}  (패턴 A: *)
TEST(OptimizationTest, CF_TC_03_BinaryStar_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(4.0), TokenType::STAR, makeLit(5.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 20.0));
}

// CF-TC-04 : var x = 10.0 / 2.0;  →  LiteralExpr{5.0}  (패턴 A: /)
TEST(OptimizationTest, CF_TC_04_BinarySlash_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(10.0), TokenType::SLASH, makeLit(2.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 5.0));
}

// CF-TC-05 : var x = 2.0 < 5.0;  →  LiteralExpr{true}  (패턴 A: <, bool 결과)
TEST(OptimizationTest, CF_TC_05_BinaryLess_FoldsToBool)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(2.0), TokenType::LESS, makeLit(5.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralBool(decl->initializer, true));
}

// CF-TC-06 : var x = -7.0;  →  LiteralExpr{-7.0}  (패턴 B: 단항 -)
TEST(OptimizationTest, CF_TC_06_UnaryMinus_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeUnary(TokenType::MINUS, makeLit(7.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, -7.0));
}

// CF-TC-07 : var x = (9.0);  →  LiteralExpr{9.0}  (패턴 C: 괄호 껍데기 제거)
TEST(OptimizationTest, CF_TC_07_Grouping_UnwrapsLiteral)
{
    auto stmts = S(makeVarDecl("x", makeGrouping(makeLit(9.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 9.0));
}

// CF-TC-08 : var y = x + 0.0;  →  initializer = VariableExpr{"x"}  (패턴 D: x+0)
TEST(OptimizationTest, CF_TC_08_IdentityAdd0_YieldsVar)
{
    auto stmts = S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::PLUS, makeLit(0.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    auto* var = dynamic_cast<VariableExpr*>(decl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
}

// CF-TC-09 : var y = x * 0.0;  →  LiteralExpr{0.0}  (패턴 D: x*0)
TEST(OptimizationTest, CF_TC_09_MultiplyBy0_YieldsZero)
{
    auto stmts = S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::STAR, makeLit(0.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 0.0));
}

// CF-TC-10 : var y = 1.0 * x;  →  VariableExpr{"x"}  (패턴 D: 1*x)
TEST(OptimizationTest, CF_TC_10_MultiplyBy1_YieldsVar)
{
    auto stmts = S(makeVarDecl("y", makeBin(makeLit(1.0), TokenType::STAR, makeVar("x"))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    auto* var = dynamic_cast<VariableExpr*>(decl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
}

// CF-TC-14 : var x = 9.0 % 4.0;  →  LiteralExpr{1.0}  (패턴 A: %)
TEST(OptimizationTest, CF_TC_14_BinaryPercent_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(9.0), TokenType::PERCENT, makeLit(4.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 1.0));
}

// CF-TC-15 : var x = 5.0 % 0.0;  →  BinaryExpr 그대로 유지 (0 나누기 비폴딩)
TEST(OptimizationTest, CF_TC_15_ModuloByZero_NotFolded)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(5.0), TokenType::PERCENT, makeLit(0.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(dynamic_cast<LiteralExpr*>(decl->initializer.get()), nullptr);
    EXPECT_NE(dynamic_cast<BinaryExpr*>(decl->initializer.get()), nullptr);
}

// CF-TC-11 : var x = 5.0 / 0.0;  →  BinaryExpr 그대로 유지 (0 나누기 비폴딩)
TEST(OptimizationTest, CF_TC_11_DivByZero_NotFolded)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(5.0), TokenType::SLASH, makeLit(0.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    // LiteralExpr 로 교체되면 안 됨
    EXPECT_EQ(dynamic_cast<LiteralExpr*>(decl->initializer.get()), nullptr);
    // BinaryExpr 그대로 유지
    EXPECT_NE(dynamic_cast<BinaryExpr*>(decl->initializer.get()), nullptr);
}

// CF-TC-12 : var x = (1.0 + 2.0) * 3.0;  →  LiteralExpr{9.0}  (중첩 폴딩)
TEST(OptimizationTest, CF_TC_12_NestedFold_YieldsLiteral)
{
    // foldExpr 재귀: 먼저 1+2→3, 이후 3*3→9
    auto stmts = S(makeVarDecl("x",
        makeBin(
            makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0)),
            TokenType::STAR,
            makeLit(3.0)
        )
    ));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 9.0));
}

// CF-TC-13 : var y = f() * 0.0;  →  BinaryExpr 유지 (함수 호출 사이드 이펙트 보호)
// x * 0 폴딩은 x 가 순수 표현식(Literal/Variable)일 때만 허용
TEST(OptimizationTest, CF_TC_13_FuncCallTimesZero_NotFolded)
{
    auto callExpr = std::make_unique<FunctionCallExpr>();
    callExpr->callee = idTok("f");

    auto stmts = S(makeVarDecl("y", makeBin(std::move(callExpr), TokenType::STAR, makeLit(0.0))));
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(dynamic_cast<LiteralExpr*>(decl->initializer.get()), nullptr);
    EXPECT_NE(dynamic_cast<BinaryExpr*>(decl->initializer.get()), nullptr);
}

// CF-TC-16 : var x = ((1+2)*(3-4))+((5*6)-(7/2))+(8+1);  — BinaryExpr 5개 → 폴딩 후 0개
// PDF 15p 요구사항: "계산 횟수가 N회에서 0회로 줄었는지 검증"
// 결과값: (3*(-1))+(30-3.5)+9 = -3+26.5+9 = 32.5
TEST(OptimizationTest, CF_TC_16_BinaryCount_NToZero)
{
    // BinaryExpr 5개: (1+2), (3-4), (1+2)*(3-4), (5*6), ((1+2)*(3-4))+(5*6)
    auto stmts = S(makeVarDecl("x",
        makeBin(
            makeBin(
                makeBin(makeLit(1.0), TokenType::PLUS,  makeLit(2.0)),
                TokenType::STAR,
                makeBin(makeLit(3.0), TokenType::MINUS, makeLit(4.0))),
            TokenType::PLUS,
            makeBin(makeLit(5.0), TokenType::STAR, makeLit(6.0)))
    ));

    EXPECT_EQ(countBinaryExpr(stmts), 5);  // 폴딩 전: BinaryExpr 5개 (N = 5)

    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    EXPECT_EQ(countBinaryExpr(stmts), 0);  // 폴딩 후: 0개 (런타임 계산 횟수 = 0)
    // (1+2)*(3-4) + (5*6) = 3*(-1) + 30 = -3 + 30 = 27
    EXPECT_TRUE(isLiteralDouble(
        dynamic_cast<VarDeclareStmt*>(stmts[0].get())->initializer, 27.0));
}

// ================================================================
// 교차 검증 (MX) — 정적 바인딩 × 상수 폴딩 복합 시나리오
//
// CF 가 노드를 교체한 뒤에도 SB distance 가 보존되는지,
// 두 최적화가 동시에 적용됐을 때 런타임 결과가 올바른지 확인한다.
// ================================================================

// MX-TC-01 : x+0 폴딩 후 VariableExpr distance 보존
// var x = 1.0; { var y = x + 0.0; }
// CF: x+0 → x  |  SB: x.distance = 1
TEST(OptimizationTest, MX_TC_01_IdentityFold_PreservesDistance)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::PLUS, makeLit(0.0)))))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* block = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(block->statements[0].get());
    ASSERT_NE(yDecl, nullptr);

    auto* var = dynamic_cast<VariableExpr*>(yDecl->initializer.get());
    ASSERT_NE(var, nullptr) << "x+0 은 VariableExpr{x} 로 폴딩돼야 함";
    EXPECT_EQ(var->name.lexeme, "x");
    EXPECT_EQ(var->distance, 1);
}

// MX-TC-02 : x*1 폴딩 후 VariableExpr distance 보존
// var x = 2.0; { var y = x * 1.0; }
TEST(OptimizationTest, MX_TC_02_MultiplyOne_PreservesDistance)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(2.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::STAR, makeLit(1.0)))))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* block = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(block->statements[0].get());
    ASSERT_NE(yDecl, nullptr);

    auto* var = dynamic_cast<VariableExpr*>(yDecl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
    EXPECT_EQ(var->distance, 1);
}

// MX-TC-03 : 0+x 폴딩 후 VariableExpr distance 보존
// var x = 3.0; { var y = 0.0 + x; }
TEST(OptimizationTest, MX_TC_03_ZeroPlus_PreservesDistance)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(3.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeLit(0.0), TokenType::PLUS, makeVar("x")))))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* block = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(block->statements[0].get());
    ASSERT_NE(yDecl, nullptr);

    auto* var = dynamic_cast<VariableExpr*>(yDecl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
    EXPECT_EQ(var->distance, 1);
}

// MX-TC-04 : 순수 VariableExpr * 0 폴딩 허용
// var x = 5.0; { var y = x * 0.0; }
// x 는 isPure → x*0 → 0.0 허용
TEST(OptimizationTest, MX_TC_04_PureVar_TimesZero_Folds)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(5.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::STAR, makeLit(0.0)))))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* block = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(block->statements[0].get());
    ASSERT_NE(yDecl, nullptr);
    EXPECT_TRUE(isLiteralDouble(yDecl->initializer, 0.0));
}

// MX-TC-05 : 부분 폴딩 — (2+3)+x 에서 2+3 만 폴딩, x distance 보존
// var x = 1.0; { var y = (2.0 + 3.0) + x; }
// CF: 2+3 → 5.0  |  SB: x.distance = 1  |  외부 BinaryExpr 는 유지
TEST(OptimizationTest, MX_TC_05_PartialFold_LiteralLeft_VarRight)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeVarDecl("y",
            makeBin(
                makeBin(makeLit(2.0), TokenType::PLUS, makeLit(3.0)),
                TokenType::PLUS,
                makeVar("x")
            )
        )))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* block = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(block->statements[0].get());
    ASSERT_NE(yDecl, nullptr);

    auto* bin = dynamic_cast<BinaryExpr*>(yDecl->initializer.get());
    ASSERT_NE(bin, nullptr) << "5+x 는 BinaryExpr 로 남아야 함";
    EXPECT_TRUE(isLiteralDouble(bin->left, 5.0));
    auto* varX = dynamic_cast<VariableExpr*>(bin->right.get());
    ASSERT_NE(varX, nullptr);
    EXPECT_EQ(varX->name.lexeme, "x");
    EXPECT_EQ(varX->distance, 1);
}

// MX-TC-06 : 초기화식 폴딩 + 참조 distance
// var a = 1.0 + 2.0; { print a; }
// a.init = LiteralExpr{3.0}  |  print 의 VarExpr{a}.distance = 1
TEST(OptimizationTest, MX_TC_06_FoldedInit_ReferenceDistanceCorrect)
{
    auto varExprPtr = makeVar("a");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("a", makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0))),
        makeBlock(S(makePrint(std::move(varExprPtr))))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* aDecl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(aDecl, nullptr);
    EXPECT_TRUE(isLiteralDouble(aDecl->initializer, 3.0));

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 1);
}

// MX-TC-07 : 2단계 중첩 블록에서 x+0 폴딩 후 distance = 2 보존
// var x = 1.0; { { var y = x + 0.0; } }
TEST(OptimizationTest, MX_TC_07_TwoLevelNest_FoldPreservesDistance2)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(
            makeBlock(S(
                makeVarDecl("y", makeBin(makeVar("x"), TokenType::PLUS, makeLit(0.0)))
            ))
        ))
    );
    Checker checker;
    ASSERT_TRUE(checker.check(stmts));

    auto* outer = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* inner = dynamic_cast<BlockStmt*>(outer->statements[0].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(inner->statements[0].get());
    ASSERT_NE(yDecl, nullptr);

    auto* var = dynamic_cast<VariableExpr*>(yDecl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
    EXPECT_EQ(var->distance, 2);
}

// ================================================================
// 교차 검증 End-to-End (Shell 경유)
// ================================================================

// MX-TC-08 : var x = 10; { print x + 0; }  →  "10"
TEST(OptimizationTest, MX_TC_08_E2E_IdentityFold_OuterVar)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var x = 10;");
    shell.runLine("{ print x + 0; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// MX-TC-09 : var a = 1 + 2; print a;  →  "3"  (CF 폴딩 후 런타임 정상 출력)
TEST(OptimizationTest, MX_TC_09_E2E_FoldedInit_PrintsCorrect)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var a = 1 + 2;");
    shell.runLine("print a;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "3\n");
}

// MX-TC-10 : var a = 1 + 2; var b = 3; print a + b;  →  "6"
TEST(OptimizationTest, MX_TC_10_E2E_TwoFoldedVars_Sum)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var a = 1 + 2;");
    shell.runLine("var b = 3;");
    shell.runLine("print a + b;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "6\n");
}

// MX-TC-11 : var x = 4; print x * 1;  →  "4"  (x*1 → x, SB distance=0)
TEST(OptimizationTest, MX_TC_11_E2E_MultiplyOne_PrintsVar)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var x = 4;");
    shell.runLine("print x * 1;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "4\n");
}

// MX-TC-12 : var limit = 2 + 1; for (var i = 0; i < limit; i = i+1) { print i; }  →  "0\n1\n2\n"
// CF 가 limit 초기식을 3.0 으로 폴딩, SB 가 for 루프에서 limit 스코프 정렬
TEST(OptimizationTest, MX_TC_12_E2E_FoldedLimit_ForLoop)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var limit = 2 + 1;");
    shell.runLine("for (var i = 0; i < limit; i = i + 1) { print i; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "0\n1\n2\n");
}

// MX-TC-13 : var x = 3; { var y = x * 0; print y; }  →  "0"
// x 는 순수 VariableExpr → x*0 → 0 폴딩 허용
TEST(OptimizationTest, MX_TC_13_E2E_PureVarTimesZero_PrintsZero)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var x = 3;");
    shell.runLine("{ var y = x * 0; print y; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "0\n");
}

// MX-TC-14 : var x = 5; print (2 + 3) * x;  →  "25"
// CF: (2+3) → 5.0, 이후 5*x 는 런타임 평가
TEST(OptimizationTest, MX_TC_14_E2E_PartialFold_RuntimeEval)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var x = 5;");
    shell.runLine("print (2 + 3) * x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "25\n");
}
