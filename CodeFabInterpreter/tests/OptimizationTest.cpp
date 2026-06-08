// OptimizationTest.cpp — Chapter 4: 정적 바인딩(SB) + 상수 폴딩(CF) + 교차 검증(MX)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Checker.h"
#include "../Executor.h"
#include "../Shell.h"

// ================================================================
// MockEnvironment — get / getAt 호출 행위를 검증하는 Test Double
// ================================================================
class MockEnvironment : public Environment
{
public:
    MOCK_METHOD(Value, get,   (const std::string& name, int line), (const, override));
    MOCK_METHOD(Value, getAt, (int distance, const std::string& name), (const, override));

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

static std::unique_ptr<Expr> makeBin(std::unique_ptr<Expr> left, TokenType op,
                                      std::unique_ptr<Expr> right)
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
// 검증 헬퍼
// ================================================================
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
        else if (auto* es = dynamic_cast<const ExpressionStmt*>(s.get()))
            total += countBinaryExprInExpr(es->expression.get());
    }
    return total;
}

// ================================================================
// Fixtures
// ================================================================

class OptimizationCheckerFixture : public ::testing::Test
{
protected:
    Checker checker;

    VarDeclareStmt* checkAndGetVarDecl(std::vector<std::unique_ptr<Stmt>>& stmts)
    {
        EXPECT_TRUE(checker.check(stmts));
        return dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    }
};

class ShellE2EFixture : public ::testing::Test
{
protected:
    Shell shell;

    std::string runLines(std::initializer_list<std::string> lines)
    {
        testing::internal::CaptureStdout();
        for (const auto& line : lines) shell.runLine(line);
        return testing::internal::GetCapturedStdout();
    }
};

// ================================================================
// 정적 바인딩 (SB-TC-01~11)
// ================================================================

// SB-TC-01 : 글로벌 스코프 변수 참조 → distance = 0
TEST_F(OptimizationCheckerFixture, SB_TC_01_GlobalVar_Distance0)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makePrint(std::move(varExprPtr))
    );
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 0);
}

// SB-TC-02 : 블록 1단계 안에서 외부 변수 참조 → distance = 1
TEST_F(OptimizationCheckerFixture, SB_TC_02_OneBlockDeep_Distance1)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makePrint(std::move(varExprPtr))))
    );
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 1);
}

// SB-TC-03 : 블록 2단계 안에서 외부 변수 참조 → distance = 2
TEST_F(OptimizationCheckerFixture, SB_TC_03_TwoBlocksDeep_Distance2)
{
    auto varExprPtr = makeVar("x");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeBlock(S(makePrint(std::move(varExprPtr))))))
    );
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 2);
}

// SB-TC-04 : 내부 블록에서 섀도잉된 변수 참조 → distance = 0 (자기 스코프)
TEST_F(OptimizationCheckerFixture, SB_TC_04_ShadowedVar_Distance0)
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
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 0);
}

// SB-TC-05 : AssignExpr 대상 변수 distance 검증 → distance = 1
TEST_F(OptimizationCheckerFixture, SB_TC_05_AssignExpr_Distance1)
{
    auto assignPtr  = makeAssignExpr("x", makeLit(2.0));
    auto* rawAssign = dynamic_cast<AssignExpr*>(assignPtr.get());

    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeExprStmt(std::move(assignPtr))))
    );
    checker.check(stmts);

    ASSERT_NE(rawAssign, nullptr);
    EXPECT_EQ(rawAssign->distance, 1);
}

// SB-TC-06 : 미선언 변수 참조 → distance = -1 유지
TEST_F(OptimizationCheckerFixture, SB_TC_06_UndeclaredVar_DistanceMinus1)
{
    auto varExprPtr = makeVar("y");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(makePrint(std::move(varExprPtr)));
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, -1);
}

// SB-TC-07 : 같은 이름이 여러 블록에 존재할 때 가장 가까운 스코프 선택 → distance = 1
TEST_F(OptimizationCheckerFixture, SB_TC_07_NearestScope_Selected)
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
    checker.check(stmts);

    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 1);
}

// SB-TC-08 : E2E — 중첩 블록에서 외부 변수 읽기 → "10"
TEST_F(ShellE2EFixture, SB_TC_08_NestedBlock_ReadsOuterVar)
{
    EXPECT_EQ(runLines({"var x = 10;", "{ print x; }"}), "10\n");
}

// SB-TC-09 : E2E — for 루프 조건에서 외부 변수 참조 → 루프 정상 실행
TEST_F(ShellE2EFixture, SB_TC_09_ForLoop_OuterVarInCondition)
{
    EXPECT_EQ(
        runLines({"var limit = 3;",
                  "for (var i = 0; i < limit; i = i + 1) { print i; }"}),
        "0\n1\n2\n"
    );
}

// SB-TC-10 : Mock — distance=0 → getAt(0) 호출, get() 미호출
TEST(StaticBindingMockTest, SB_TC_10_GlobalVar_UsesGetAt_NotGet)
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

// SB-TC-11 : Mock — 블록 내부 외부 변수 참조 → 체인 순회(get) 미호출
TEST(StaticBindingMockTest, SB_TC_11_BlockVar_NoChainTraversal)
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

// ================================================================
// 상수 폴딩 (CF-TC-01~16)
//
// CF-TC-01~05, CF-TC-14 : PatternAFoldTest (TEST_P) — 아래 참조
// CF-TC-06~13, CF-TC-15~16 : OptimizationCheckerFixture (TEST_F)
// ================================================================

// --- CF-TC-01~05, CF-TC-14 : 패턴 A 매개변수화 테스트 ---
// BinaryExpr(Lit, op, Lit) → LiteralExpr 폴딩 검증
struct PatternAParam
{
    const char* testName;
    TokenType   op;
    double      lhs;
    double      rhs;
    Value       expected;
};

class PatternAFoldTest : public ::testing::TestWithParam<PatternAParam> {};

TEST_P(PatternAFoldTest, FoldsToLiteral)
{
    const auto& p = GetParam();
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(p.lhs), p.op, makeLit(p.rhs))));

    Checker checker;
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);

    if (std::holds_alternative<double>(p.expected))
        EXPECT_TRUE(isLiteralDouble(decl->initializer, std::get<double>(p.expected)));
    else if (std::holds_alternative<bool>(p.expected))
        EXPECT_TRUE(isLiteralBool(decl->initializer, std::get<bool>(p.expected)));
}

INSTANTIATE_TEST_SUITE_P(
    PatternA, PatternAFoldTest,
    ::testing::Values(
        PatternAParam{"CF_TC_01_Plus",    TokenType::PLUS,    1.0,  2.0, Value{3.0}},
        PatternAParam{"CF_TC_02_Minus",   TokenType::MINUS,   10.0, 3.0, Value{7.0}},
        PatternAParam{"CF_TC_03_Star",    TokenType::STAR,    4.0,  5.0, Value{20.0}},
        PatternAParam{"CF_TC_04_Slash",   TokenType::SLASH,   10.0, 2.0, Value{5.0}},
        PatternAParam{"CF_TC_05_Less",    TokenType::LESS,    2.0,  5.0, Value{true}},
        PatternAParam{"CF_TC_14_Percent", TokenType::PERCENT, 9.0,  4.0, Value{1.0}}
    ),
    [](const ::testing::TestParamInfo<PatternAParam>& info) {
        return info.param.testName;
    }
);

// --- CF-TC-06~13, CF-TC-15~16 ---

// CF-TC-06 : var x = -7.0;  →  LiteralExpr{-7.0}  (패턴 B: 단항 -)
TEST_F(OptimizationCheckerFixture, CF_TC_06_UnaryMinus_FoldsToLiteral)
{
    auto stmts = S(makeVarDecl("x", makeUnary(TokenType::MINUS, makeLit(7.0))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, -7.0));
}

// CF-TC-07 : var x = (9.0);  →  LiteralExpr{9.0}  (패턴 C: 괄호 껍데기 제거)
TEST_F(OptimizationCheckerFixture, CF_TC_07_Grouping_UnwrapsLiteral)
{
    auto stmts = S(makeVarDecl("x", makeGrouping(makeLit(9.0))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 9.0));
}

// CF-TC-08 : var y = x + 0.0;  →  VariableExpr{"x"}  (패턴 D: x+0)
TEST_F(OptimizationCheckerFixture, CF_TC_08_IdentityAdd0_YieldsVar)
{
    auto stmts = S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::PLUS, makeLit(0.0))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    auto* var = dynamic_cast<VariableExpr*>(decl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
}

// CF-TC-09 : var y = x * 0.0;  →  LiteralExpr{0.0}  (패턴 D: x*0, x는 순수 표현식)
TEST_F(OptimizationCheckerFixture, CF_TC_09_MultiplyBy0_YieldsZero)
{
    auto stmts = S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::STAR, makeLit(0.0))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 0.0));
}

// CF-TC-10 : var y = 1.0 * x;  →  VariableExpr{"x"}  (패턴 D: 1*x)
TEST_F(OptimizationCheckerFixture, CF_TC_10_MultiplyBy1_YieldsVar)
{
    auto stmts = S(makeVarDecl("y", makeBin(makeLit(1.0), TokenType::STAR, makeVar("x"))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    auto* var = dynamic_cast<VariableExpr*>(decl->initializer.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
}

// CF-TC-11 : var x = 5.0 / 0.0;  →  BinaryExpr 유지 (0 나누기 비폴딩)
TEST_F(OptimizationCheckerFixture, CF_TC_11_DivByZero_NotFolded)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(5.0), TokenType::SLASH, makeLit(0.0))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(dynamic_cast<LiteralExpr*>(decl->initializer.get()), nullptr);
    EXPECT_NE(dynamic_cast<BinaryExpr*>(decl->initializer.get()), nullptr);
}

// CF-TC-12 : var x = (1.0 + 2.0) * 3.0;  →  LiteralExpr{9.0}  (중첩 폴딩)
TEST_F(OptimizationCheckerFixture, CF_TC_12_NestedFold_YieldsLiteral)
{
    auto stmts = S(makeVarDecl("x",
        makeBin(
            makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0)),
            TokenType::STAR,
            makeLit(3.0)
        )
    ));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    EXPECT_TRUE(isLiteralDouble(decl->initializer, 9.0));
}

// CF-TC-13 : var y = f() * 0.0;  →  BinaryExpr 유지 (함수 호출 사이드 이펙트 보호)
TEST_F(OptimizationCheckerFixture, CF_TC_13_FuncCallTimesZero_NotFolded)
{
    auto callExpr = std::make_unique<FunctionCallExpr>();
    callExpr->callee = idTok("f");

    auto stmts = S(makeVarDecl("y", makeBin(std::move(callExpr), TokenType::STAR, makeLit(0.0))));
    ASSERT_TRUE(checker.check(stmts));
    auto* decl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(dynamic_cast<LiteralExpr*>(decl->initializer.get()), nullptr);
    EXPECT_NE(dynamic_cast<BinaryExpr*>(decl->initializer.get()), nullptr);
}

// CF-TC-15 : var x = 5.0 % 0.0;  →  BinaryExpr 유지 (0 나머지 비폴딩)
TEST_F(OptimizationCheckerFixture, CF_TC_15_ModuloByZero_NotFolded)
{
    auto stmts = S(makeVarDecl("x", makeBin(makeLit(5.0), TokenType::PERCENT, makeLit(0.0))));
    auto* decl = checkAndGetVarDecl(stmts);
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(dynamic_cast<LiteralExpr*>(decl->initializer.get()), nullptr);
    EXPECT_NE(dynamic_cast<BinaryExpr*>(decl->initializer.get()), nullptr);
}

// CF-TC-16 : 복합 표현식 → BinaryExpr N개에서 0개로 (계산 횟수 검증)
TEST_F(OptimizationCheckerFixture, CF_TC_16_BinaryCount_NToZero)
{
    auto stmts = S(makeVarDecl("x",
        makeBin(
            makeBin(
                makeBin(makeLit(1.0), TokenType::PLUS,  makeLit(2.0)),
                TokenType::STAR,
                makeBin(makeLit(3.0), TokenType::MINUS, makeLit(4.0))),
            TokenType::PLUS,
            makeBin(makeLit(5.0), TokenType::STAR, makeLit(6.0)))
    ));

    EXPECT_EQ(countBinaryExpr(stmts), 5);
    ASSERT_TRUE(checker.check(stmts));
    EXPECT_EQ(countBinaryExpr(stmts), 0);
    EXPECT_TRUE(isLiteralDouble(
        dynamic_cast<VarDeclareStmt*>(stmts[0].get())->initializer, 27.0));
}

// ================================================================
// 교차 검증 (MX-TC-01~14)
// ================================================================

// MX-TC-01 : x+0 폴딩 후 VariableExpr distance 보존
TEST_F(OptimizationCheckerFixture, MX_TC_01_IdentityFold_PreservesDistance)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::PLUS, makeLit(0.0)))))
    );
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
TEST_F(OptimizationCheckerFixture, MX_TC_02_MultiplyOne_PreservesDistance)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(2.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::STAR, makeLit(1.0)))))
    );
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
TEST_F(OptimizationCheckerFixture, MX_TC_03_ZeroPlus_PreservesDistance)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(3.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeLit(0.0), TokenType::PLUS, makeVar("x")))))
    );
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
TEST_F(OptimizationCheckerFixture, MX_TC_04_PureVar_TimesZero_Folds)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(5.0)),
        makeBlock(S(makeVarDecl("y", makeBin(makeVar("x"), TokenType::STAR, makeLit(0.0)))))
    );
    ASSERT_TRUE(checker.check(stmts));

    auto* block = dynamic_cast<BlockStmt*>(stmts[1].get());
    auto* yDecl = dynamic_cast<VarDeclareStmt*>(block->statements[0].get());
    ASSERT_NE(yDecl, nullptr);
    EXPECT_TRUE(isLiteralDouble(yDecl->initializer, 0.0));
}

// MX-TC-05 : (2+3)+x 에서 2+3만 폴딩, x distance 보존
TEST_F(OptimizationCheckerFixture, MX_TC_05_PartialFold_LiteralLeft_VarRight)
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

// MX-TC-06 : 초기화식 폴딩 + 참조 distance 정확성
TEST_F(OptimizationCheckerFixture, MX_TC_06_FoldedInit_ReferenceDistanceCorrect)
{
    auto varExprPtr = makeVar("a");
    auto* rawVar    = dynamic_cast<VariableExpr*>(varExprPtr.get());

    auto stmts = S(
        makeVarDecl("a", makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0))),
        makeBlock(S(makePrint(std::move(varExprPtr))))
    );
    ASSERT_TRUE(checker.check(stmts));

    auto* aDecl = dynamic_cast<VarDeclareStmt*>(stmts[0].get());
    ASSERT_NE(aDecl, nullptr);
    EXPECT_TRUE(isLiteralDouble(aDecl->initializer, 3.0));
    ASSERT_NE(rawVar, nullptr);
    EXPECT_EQ(rawVar->distance, 1);
}

// MX-TC-07 : 2단계 중첩 블록에서 x+0 폴딩 후 distance = 2 보존
TEST_F(OptimizationCheckerFixture, MX_TC_07_TwoLevelNest_FoldPreservesDistance2)
{
    auto stmts = S(
        makeVarDecl("x", makeLit(1.0)),
        makeBlock(S(
            makeBlock(S(
                makeVarDecl("y", makeBin(makeVar("x"), TokenType::PLUS, makeLit(0.0)))
            ))
        ))
    );
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

// MX-TC-08 : E2E — { print x + 0; }  →  "10"
TEST_F(ShellE2EFixture, MX_TC_08_E2E_IdentityFold_OuterVar)
{
    EXPECT_EQ(runLines({"var x = 10;", "{ print x + 0; }"}), "10\n");
}

// MX-TC-09 : E2E — var a = 1 + 2; print a;  →  "3"
TEST_F(ShellE2EFixture, MX_TC_09_E2E_FoldedInit_PrintsCorrect)
{
    EXPECT_EQ(runLines({"var a = 1 + 2;", "print a;"}), "3\n");
}

// MX-TC-10 : E2E — var a = 1 + 2; var b = 3; print a + b;  →  "6"
TEST_F(ShellE2EFixture, MX_TC_10_E2E_TwoFoldedVars_Sum)
{
    EXPECT_EQ(runLines({"var a = 1 + 2;", "var b = 3;", "print a + b;"}), "6\n");
}

// MX-TC-11 : E2E — var x = 4; print x * 1;  →  "4"
TEST_F(ShellE2EFixture, MX_TC_11_E2E_MultiplyOne_PrintsVar)
{
    EXPECT_EQ(runLines({"var x = 4;", "print x * 1;"}), "4\n");
}

// MX-TC-12 : E2E — var limit = 2 + 1; for (...) { print i; }  →  "0\n1\n2\n"
TEST_F(ShellE2EFixture, MX_TC_12_E2E_FoldedLimit_ForLoop)
{
    EXPECT_EQ(
        runLines({"var limit = 2 + 1;",
                  "for (var i = 0; i < limit; i = i + 1) { print i; }"}),
        "0\n1\n2\n"
    );
}

// MX-TC-13 : E2E — var x = 3; { var y = x * 0; print y; }  →  "0"
TEST_F(ShellE2EFixture, MX_TC_13_E2E_PureVarTimesZero_PrintsZero)
{
    EXPECT_EQ(runLines({"var x = 3;", "{ var y = x * 0; print y; }"}), "0\n");
}

// MX-TC-14 : E2E — var x = 5; print (2 + 3) * x;  →  "25"
TEST_F(ShellE2EFixture, MX_TC_14_E2E_PartialFold_RuntimeEval)
{
    EXPECT_EQ(runLines({"var x = 5;", "print (2 + 3) * x;"}), "25\n");
}
