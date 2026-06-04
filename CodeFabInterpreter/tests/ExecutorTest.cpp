#include <gtest/gtest.h>
#include "../Executor.h"


// TC1: execute()에 Stmt* 벡터가 정상적으로 전달되는지 확인
TEST(ExecutorTest, StmtReceivedCorrectly)
{
	auto expr = std::make_unique<LiteralExpr>();
	expr->value = 42.0;
	LiteralExpr * exprPtr = expr.get();

	auto stmt = std::make_unique<PrintStmt>();
	stmt->expression = std::move(expr);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	
	Executor executor;
	ASSERT_NO_THROW(executor.execute(stmts));
	ASSERT_EQ(stmts.size(), 1u);
	
	ASSERT_NE(dynamic_cast<PrintStmt*>(stmts[0].get()), nullptr);
	ASSERT_EQ(dynamic_cast<PrintStmt*>(stmts[0].get())->expression.get(), exprPtr);
}

// TC2: PrintStmt + LiteralExpr → 숫자/문자열 값이 stdout에 출력되는지 확인
TEST(ExecutorTest, PrintLiteral)
{
	// 숫자 케이스: 3.14 → "3.14\n"
	{
		auto expr = std::make_unique<LiteralExpr>();
		expr->value = 3.14;

		auto stmt = std::make_unique<PrintStmt>();
		stmt->expression = std::move(expr);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(stmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		std::string output = testing::internal::GetCapturedStdout();

		ASSERT_EQ(output, "3.14\n");
	}

	// 문자열 케이스: "hello" → "hello\n"
	{
		auto expr = std::make_unique<LiteralExpr>();
		expr->value = std::string("hello");

		auto stmt = std::make_unique<PrintStmt>();
		stmt->expression = std::move(expr);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(stmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		std::string output = testing::internal::GetCapturedStdout();

		ASSERT_EQ(output, "hello\n");
	}
}

// TC3: BinaryExpr 사칙연산 결과가 올바르게 계산되는지 확인
TEST(ExecutorTest, ArithmeticExpr)
{
	struct Case { double l; TokenType op; double r; std::string expected; };
	std::vector<Case> cases = {
		{ 3.0,  TokenType::PLUS,  4.0, "7\n"  },
		{ 10.0, TokenType::MINUS, 3.0, "7\n"  },
		{ 2.0,  TokenType::STAR,  5.0, "10\n" },
		{ 9.0,  TokenType::SLASH, 3.0, "3\n"  },
	};

	for (const auto& c : cases)
	{
		auto left = std::make_unique<LiteralExpr>();
		left->value = c.l;

		auto right = std::make_unique<LiteralExpr>();
		right->value = c.r;

		Token op;
		op.type = c.op;

		auto binExpr = std::make_unique<BinaryExpr>();
		binExpr->left  = std::move(left);
		binExpr->op    = op;
		binExpr->right = std::move(right);

		auto stmt = std::make_unique<PrintStmt>();
		stmt->expression = std::move(binExpr);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(stmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		std::string output = testing::internal::GetCapturedStdout();

		ASSERT_EQ(output, c.expected);
	}
}

// TC4: VarDeclareStmt로 선언한 변수를 VariableExpr로 참조할 수 있는지 확인
TEST(ExecutorTest, VarDeclareAndUse)
{
	// var x = 10.0;
	Token nameToken;
	nameToken.lexeme = "x";

	auto init = std::make_unique<LiteralExpr>();
	init->value = 10.0;

	auto varDecl = std::make_unique<VarDeclareStmt>();
	varDecl->name        = nameToken;
	varDecl->initializer = std::move(init);

	// print x;
	auto varExpr = std::make_unique<VariableExpr>();
	varExpr->name = nameToken;

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(varExpr);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(varDecl));
	stmts.push_back(std::move(printStmt));

	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	std::string output = testing::internal::GetCapturedStdout();

	ASSERT_EQ(output, "10\n");
}

// TC5: IfStmt의 condition 결과에 따라 thenBranch/elseBranch가 선택 실행되는지 확인
TEST(ExecutorTest, IfStatement)
{
	auto makePrintStr = [](const std::string& s) {
		auto expr = std::make_unique<LiteralExpr>();
		expr->value = s;
		auto stmt = std::make_unique<PrintStmt>();
		stmt->expression = std::move(expr);
		return stmt;
	};

	// true 케이스: condition=true → thenBranch 실행 → "then\n"
	{
		auto cond = std::make_unique<LiteralExpr>();
		cond->value = true;

		auto ifStmt = std::make_unique<IfStmt>();
		ifStmt->condition  = std::move(cond);
		ifStmt->thenBranch = makePrintStr("then");
		ifStmt->elseBranch = makePrintStr("else");

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(ifStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "then\n");
	}

	// false 케이스: condition=false → elseBranch 실행 → "else\n"
	{
		auto cond = std::make_unique<LiteralExpr>();
		cond->value = false;

		auto ifStmt = std::make_unique<IfStmt>();
		ifStmt->condition  = std::move(cond);
		ifStmt->thenBranch = makePrintStr("then");
		ifStmt->elseBranch = makePrintStr("else");

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(ifStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "else\n");
	}
}

// TC6: ForStmt의 init/condition/increment/body가 순서대로 반복 실행되는지 확인
TEST(ExecutorTest, ForStatement) { ASSERT_TRUE(false); }

// TC7: LEFT_BRACE → 새 변수 저장소 생성, RIGHT_BRACE → 변수 저장소 소멸 (Shadowing으로 검증)
TEST(ExecutorTest, BlockScope_ScopeLifecycle){ ASSERT_TRUE(false); }

// TC7-1: 중첩 블록마다 독립적인 로컬 저장소가 생성/소멸되는지 확인
TEST(ExecutorTest, BlockScope_NestedScopes) { ASSERT_TRUE(false); }

// TC8: 선언되지 않은 변수 참조 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, UndefinedVariable) { ASSERT_TRUE(false); }

// TC9: 타입 불일치 연산(number + string 등) 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, TypeError) { ASSERT_TRUE(false); }

// TC10: 0으로 나누기 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, DivideByZero) { ASSERT_TRUE(false); }
