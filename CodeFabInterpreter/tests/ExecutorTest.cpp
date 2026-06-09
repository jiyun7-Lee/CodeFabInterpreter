#include <gtest/gtest.h>
#include "../Executor.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "TestHelpers.h"

// -----------------------------------------------------------------------
// Fixture
// -----------------------------------------------------------------------

class ExecutorTest : public ::testing::Test
{
protected:
    Executor  executor;
    Tokenizer tokenizer;
    Parser    parser;

    std::vector<std::unique_ptr<Stmt>> parse(const std::string& src)
    {
        return parser.parse(tokenizer.tokenize(src));
    }

    // 공통 람다 — BlockScope 테스트용
    static std::unique_ptr<PrintStmt> printVar(const Token& t)  { return makePrintVar(t); }
    static std::unique_ptr<VarDeclareStmt> varDecl(const Token& t, double v) { return makeVarDeclLit(t, v); }
};

// TC0: ExpressionStmt 평가 — 순수 계산은 예외 없이 실행, AssignExpr 사이드 이펙트 반영 확인
TEST_F(ExecutorTest, ExpressionStatement)
{
	// 순수 계산: 1.0 + 2.0 → stdout 없음
	{
		auto exprStmt = std::make_unique<ExpressionStmt>();
		exprStmt->expression = makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0));
		auto stmts = makeStmts(std::move(exprStmt));

		ASSERT_EQ(captureOutput([&]{ ASSERT_NO_THROW(executor.execute(stmts)); }), "");
	}

	// 사이드 이펙트: var x = 1.0; x = 5.0; print x; → "5\n"
	{
		Token xToken; xToken.lexeme = "x";

		auto assignExpr = std::make_unique<AssignExpr>();
		assignExpr->name  = xToken;
		assignExpr->value = makeLit(5.0);

		auto exprStmt = std::make_unique<ExpressionStmt>();
		exprStmt->expression = std::move(assignExpr);

		auto stmts = makeStmts(
			makeVarDeclLit(xToken, 1.0),
			std::move(exprStmt),
			makePrintVar(xToken));

		ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "5\n");
	}
}

// TC1: execute()에 Stmt* 벡터가 정상적으로 전달되는지 확인
TEST_F(ExecutorTest, StmtReceivedCorrectly)
{
	auto expr    = std::make_unique<LiteralExpr>(); expr->value = 42.0;
	auto exprPtr = expr.get();
	auto stmts   = makeStmts(makePrint(std::move(expr)));

	ASSERT_NO_THROW(executor.execute(stmts));
	ASSERT_NE(dynamic_cast<PrintStmt*>(stmts[0].get()), nullptr);
	ASSERT_EQ(dynamic_cast<PrintStmt*>(stmts[0].get())->expression.get(), exprPtr);
}

// TC2: PrintStmt + LiteralExpr → 숫자/문자열 값이 stdout에 출력되는지 확인
TEST_F(ExecutorTest, PrintLiteral)
{
	ASSERT_EQ(captureOutput([&]{ executor.execute(makeStmts(makePrintLit(3.14))); }), "3.14\n");
	ASSERT_EQ(captureOutput([&]{ executor.execute(makeStmts(makePrintStr("hello"))); }), "hello\n");
}

// TC3: BinaryExpr 사칙연산 결과가 올바르게 계산되는지 확인
TEST_F(ExecutorTest, ArithmeticExpr)
{
	struct Case { double l; TokenType op; double r; std::string expected; };
	std::vector<Case> cases = {
		{ 3.0,  TokenType::PLUS,    4.0, "7\n"  },
		{ 10.0, TokenType::MINUS,   3.0, "7\n"  },
		{ 2.0,  TokenType::STAR,    5.0, "10\n" },
		{ 9.0,  TokenType::SLASH,   3.0, "3\n"  },
		{ 10.0, TokenType::PERCENT, 3.0, "1\n"  },
		{ 0.0,  TokenType::PERCENT, 5.0, "0\n"  },
	};

	for (const auto& c : cases)
	{
		auto stmts = makeStmts(makePrint(makeBin(makeLit(c.l), c.op, makeLit(c.r))));
		ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), c.expected);
	}
}

// TC4: VarDeclareStmt로 선언한 변수를 VariableExpr로 참조할 수 있는지 확인
TEST_F(ExecutorTest, VarDeclareAndUse)
{
	Token xToken; xToken.lexeme = "x";
	auto stmts = makeStmts(makeVarDeclLit(xToken, 10.0), makePrintVar(xToken));
	ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "10\n");
}

// TC5: IfStmt의 condition 결과에 따라 thenBranch/elseBranch가 선택 실행되는지 확인
TEST_F(ExecutorTest, IfStatement)
{
	auto makeIf = [](bool cond) {
		auto ifStmt = std::make_unique<IfStmt>();
		ifStmt->condition  = makeLitBool(cond);
		ifStmt->thenBranch = makePrintStr("then");
		ifStmt->elseBranch = makePrintStr("else");
		return ifStmt;
	};

	ASSERT_EQ(captureOutput([&]{ executor.execute(makeStmts(makeIf(true))); }),  "then\n");
	ASSERT_EQ(captureOutput([&]{ executor.execute(makeStmts(makeIf(false))); }), "else\n");
}

// TC6: ForStmt의 init/condition/increment/body가 순서대로 반복 실행되는지 확인
TEST_F(ExecutorTest, ForStatement)
{
	// for (var i = 0; i < 3; i = i + 1) { print i; } → "0\n1\n2\n"
	Token iToken; iToken.lexeme = "i";

	auto increment = std::make_unique<AssignExpr>();
	increment->name  = iToken;
	increment->value = makeBin(makeVar(iToken), TokenType::PLUS, makeLit(1.0));

	auto forStmt = std::make_unique<ForStmt>();
	forStmt->init      = makeVarDeclLit(iToken, 0.0);
	forStmt->condition = makeBin(makeVar(iToken), TokenType::LESS, makeLit(3.0));
	forStmt->increment = std::move(increment);
	forStmt->body      = makePrintVar(iToken);

	ASSERT_EQ(captureOutput([&]{ executor.execute(makeStmts(std::move(forStmt))); }), "0\n1\n2\n");
}

// TC7: 블록 스코프 — 안쪽 var은 바깥과 독립, 바깥 var은 블록 후에도 유지
TEST_F(ExecutorTest, BlockScope_ScopeLifecycle)
{
	Token xToken; xToken.lexeme = "x";

	auto block = std::make_unique<BlockStmt>();
	block->statements.push_back(varDecl(xToken, 2.0));
	block->statements.push_back(printVar(xToken));

	auto stmts = makeStmts(varDecl(xToken, 1.0), std::move(block), printVar(xToken));
	ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "2\n1\n");
}

// TC7-1: 중첩 블록마다 독립적인 로컬 저장소가 생성/소멸되는지 확인
TEST_F(ExecutorTest, BlockScope_NestedScopes)
{
	Token xToken; xToken.lexeme = "x";

	auto innerBlock = std::make_unique<BlockStmt>();
	innerBlock->statements.push_back(varDecl(xToken, 3.0));
	innerBlock->statements.push_back(printVar(xToken));

	auto outerBlock = std::make_unique<BlockStmt>();
	outerBlock->statements.push_back(varDecl(xToken, 2.0));
	outerBlock->statements.push_back(std::move(innerBlock));
	outerBlock->statements.push_back(printVar(xToken));

	auto stmts = makeStmts(varDecl(xToken, 1.0), std::move(outerBlock), printVar(xToken));
	ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "3\n2\n1\n");
}

// TC8: 선언되지 않은 변수 참조 시 RuntimeError
TEST_F(ExecutorTest, UndefinedVariable)
{
	Token abcToken; abcToken.lexeme = "abc";
	auto stmts = makeStmts(makePrintVar(abcToken));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC8-2: string + string → 문자열 연결 출력
TEST_F(ExecutorTest, StringConcatenation)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLitStr("Hello, "), TokenType::PLUS, makeLitStr("CodeFab!"))));
	EXPECT_EQ(captureOutput([&]{ executor.execute(stmts); }), "Hello, CodeFab!\n");
}

// TC8-3: boolean * boolean → RuntimeError
TEST_F(ExecutorTest, BooleanArithmeticError)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLitBool(true), TokenType::STAR, makeLitBool(true))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC9: 타입 불일치 연산(number + string) 시 RuntimeError
TEST_F(ExecutorTest, TypeError)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLit(1.0), TokenType::PLUS, makeLitStr("HI"))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC10: 0으로 나누기 시 RuntimeError
TEST_F(ExecutorTest, DivideByZero)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLit(10.0), TokenType::SLASH, makeLit(0.0))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC11-1: UnaryExpr MINUS — 숫자 부호 반전
TEST_F(ExecutorTest, UnaryExpr_Minus)
{
	auto stmts = makeStmts(makePrint(makeUnary(TokenType::MINUS, makeLit(3.0))));
	ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "-3\n");
}

// TC11-2: UnaryExpr BANG — 논리 반전
TEST_F(ExecutorTest, UnaryExpr_Bang)
{
	auto stmts = makeStmts(makePrint(makeUnary(TokenType::BANG, makeLitBool(true))));
	ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "false\n");
}

// TC11-3: UnaryExpr MINUS 타입 불일치 — RuntimeError
TEST_F(ExecutorTest, UnaryExpr_TypeMismatch)
{
	auto stmts = makeStmts(makePrint(makeUnary(TokenType::MINUS, makeLitStr("hello"))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC12: GroupingExpr — 괄호 내부 expression이 올바르게 평가되는지 확인
TEST_F(ExecutorTest, GroupingExpr)
{
	auto makeGroup = [](std::unique_ptr<Expr> e) {
		auto g = std::make_unique<GroupingExpr>(); g->expression = std::move(e); return g;
	};

	// (1.0 + 2.0) → "3\n"
	{
		auto stmts = makeStmts(makePrint(makeGroup(makeBin(makeLit(1.0), TokenType::PLUS, makeLit(2.0)))));
		ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "3\n");
	}
	// (-(3.0)) → "-3\n"
	{
		auto stmts = makeStmts(makePrint(makeGroup(makeUnary(TokenType::MINUS, makeLit(3.0)))));
		ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), "-3\n");
	}
}

// TC13: BinaryExpr AND — 단락 평가, true/false 조합 결과 확인
TEST_F(ExecutorTest, LogicalAnd)
{
	auto makeAnd = [](std::unique_ptr<Expr> l, std::unique_ptr<Expr> r) {
		return makeBin(std::move(l), TokenType::AND, std::move(r));
	};

	struct Case { bool l; bool r; std::string expected; };
	for (const auto& c : std::vector<Case>{
		{true, true, "true\n"}, {true, false, "false\n"},
		{false, true, "false\n"}, {false, false, "false\n"},
	})
	{
		auto stmts = makeStmts(makePrint(makeAnd(makeLitBool(c.l), makeLitBool(c.r))));
		ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), c.expected);
	}

	// 단락 평가: 왼쪽이 false면 오른쪽 평가 안 함
	auto stmts = makeStmts(makePrint(
		makeAnd(makeLitBool(false), makeBin(makeLit(1.0), TokenType::SLASH, makeLit(0.0)))));
	ASSERT_NO_THROW(executor.execute(stmts));
}

// TC14: BinaryExpr OR — 단락 평가, true/false 조합 결과 확인
TEST_F(ExecutorTest, LogicalOr)
{
	auto makeOr = [](std::unique_ptr<Expr> l, std::unique_ptr<Expr> r) {
		return makeBin(std::move(l), TokenType::OR, std::move(r));
	};

	struct Case { bool l; bool r; std::string expected; };
	for (const auto& c : std::vector<Case>{
		{true, true, "true\n"}, {true, false, "true\n"},
		{false, true, "true\n"}, {false, false, "false\n"},
	})
	{
		auto stmts = makeStmts(makePrint(makeOr(makeLitBool(c.l), makeLitBool(c.r))));
		ASSERT_EQ(captureOutput([&]{ executor.execute(stmts); }), c.expected);
	}

	// 단락 평가: 왼쪽이 true면 오른쪽 평가 안 함
	auto stmts = makeStmts(makePrint(
		makeOr(makeLitBool(true), makeBin(makeLit(1.0), TokenType::SLASH, makeLit(0.0)))));
	ASSERT_NO_THROW(executor.execute(stmts));
}

// [Regression #4] for 루프 init 변수는 루프 종료 후 소멸해야 함
TEST_F(ExecutorTest, ForScope_InitVarNotLeaking)
{
    auto stmts = parse("for (var i = 0; i < 3; i = i + 1) {} print i;");
    ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// [Regression #11] TC-ErrMsg-1: 미정의 변수 → "[N번째 줄] 미정의된 변수 'x'"
TEST_F(ExecutorTest, ErrorMessage_UndefinedVariable)
{
    try {
        executor.execute(parse("print abc;"));
        FAIL() << "RuntimeError expected";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("번째 줄"),     std::string::npos) << msg;
        EXPECT_NE(msg.find("미정의된 변수"), std::string::npos) << msg;
    }
}

// TC-ErrMsg-2: 타입 불일치(number + string) → "[N번째 줄] '+' 연산자는 ..."
TEST_F(ExecutorTest, ErrorMessage_TypeError)
{
    try {
        executor.execute(parse("print 1 + \"hi\";"));
        FAIL() << "RuntimeError expected";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("번째 줄"),      std::string::npos) << msg;
        EXPECT_NE(msg.find("'+' 연산자는"), std::string::npos) << msg;
    }
}

// TC-ErrMsg-3: 0 나누기 → "[N번째 줄] 0으로 나눈 오류"
TEST_F(ExecutorTest, ErrorMessage_DivideByZero)
{
    try {
        executor.execute(parse("print 1 / 0;"));
        FAIL() << "RuntimeError expected";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("번째 줄"),   std::string::npos) << msg;
        EXPECT_NE(msg.find("0으로 나눈"), std::string::npos) << msg;
    }
}

// ================================================================
// Negative UT
// ================================================================

// TC15: 문자열(lhs) + 숫자 연산 시 RuntimeError
TEST_F(ExecutorTest, StringPlusNumberThrows)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLitStr("hello"), TokenType::PLUS, makeLit(1.0))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC16: 문자열끼리 대소 비교(>) 시 RuntimeError
TEST_F(ExecutorTest, StringComparisonThrows)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLitStr("a"), TokenType::GREATER, makeLitStr("b"))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC17: bool + 숫자 연산 시 RuntimeError
TEST_F(ExecutorTest, BoolArithmeticThrows)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLitBool(true), TokenType::PLUS, makeLit(1.0))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC18: null + 숫자 연산 시 RuntimeError
TEST_F(ExecutorTest, NullArithmeticThrows)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLitNull(), TokenType::PLUS, makeLit(1.0))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC19: 선언되지 않은 변수에 대입 시 RuntimeError
TEST_F(ExecutorTest, AssignUndeclaredVarThrows)
{
	Token xToken; xToken.lexeme = "x";
	auto assignExpr = std::make_unique<AssignExpr>();
	assignExpr->name = xToken; assignExpr->value = makeLit(5.0);
	auto stmt = std::make_unique<ExpressionStmt>(); stmt->expression = std::move(assignExpr);
	auto stmts = makeStmts(std::move(stmt));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC20: null 반환 함수 결과를 산술 연산에 사용 시 RuntimeError
TEST_F(ExecutorTest, NullReturnArithmeticThrows)
{
	Token fTok; fTok.lexeme = "f";

	auto fnDecl = std::make_unique<FunctionDeclareStmt>();
	fnDecl->name = fTok; fnDecl->body = std::make_unique<BlockStmt>();

	auto callExpr = std::make_unique<FunctionCallExpr>(); callExpr->callee = fTok;
	auto stmts = makeStmts(
		std::move(fnDecl),
		makePrint(makeBin(std::move(callExpr), TokenType::PLUS, makeLit(1.0))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC22: 0으로 나누기 모듈러 연산 시 RuntimeError
TEST_F(ExecutorTest, ModuloByZero)
{
	auto stmts = makeStmts(makePrint(makeBin(makeLit(10.0), TokenType::PERCENT, makeLit(0.0))));
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC21: 초기화 없는 var 선언 후 print → "null\n"
TEST_F(ExecutorTest, VarNoInitializerPrintsNull)
{
	EXPECT_EQ(captureOutput([&]{ executor.execute(parse("var x; print x;")); }), "null\n");
}
