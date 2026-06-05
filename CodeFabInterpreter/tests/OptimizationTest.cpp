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
    // (1.0 + 2.0) 는 GroupingExpr 없이 BinaryExpr 로 직접 구성
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
