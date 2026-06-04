#include <gtest/gtest.h>
#include "../Executor.h"


// TC0: ExpressionStmt 평가 — 결과 자동 출력, UnaryExpr/GroupingExpr, AssignExpr 사이드 이펙트 확인
TEST(ExecutorTest, ExpressionStatement)
{
	// 순수 계산 케이스: 1.0 + 2.0 → stdout "3\n" 자동 출력
	{
		auto left  = std::make_unique<LiteralExpr>(); left->value  = 1.0;
		auto right = std::make_unique<LiteralExpr>(); right->value = 2.0;
		Token plusOp; plusOp.type = TokenType::PLUS;

		auto binExpr = std::make_unique<BinaryExpr>();
		binExpr->left = std::move(left); binExpr->op = plusOp; binExpr->right = std::move(right);

		auto exprStmt = std::make_unique<ExpressionStmt>();
		exprStmt->expression = std::move(binExpr);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(exprStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		ASSERT_NO_THROW(executor.execute(stmts));
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "3\n");
	}

	// UnaryExpr 케이스: -3.0 → "-3\n", !true → "false\n"
	{
		Token minusOp; minusOp.type = TokenType::MINUS;
		auto lit = std::make_unique<LiteralExpr>(); lit->value = 3.0;
		auto unary = std::make_unique<UnaryExpr>();
		unary->op = minusOp; unary->right = std::move(lit);
		auto exprStmt = std::make_unique<ExpressionStmt>();
		exprStmt->expression = std::move(unary);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(exprStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "-3\n");
	}

	// GroupingExpr 케이스: (1.0 + 2.0) → "3\n"
	{
		auto left  = std::make_unique<LiteralExpr>(); left->value  = 1.0;
		auto right = std::make_unique<LiteralExpr>(); right->value = 2.0;
		Token plusOp; plusOp.type = TokenType::PLUS;
		auto binExpr = std::make_unique<BinaryExpr>();
		binExpr->left = std::move(left); binExpr->op = plusOp; binExpr->right = std::move(right);
		auto group = std::make_unique<GroupingExpr>();
		group->expression = std::move(binExpr);
		auto exprStmt = std::make_unique<ExpressionStmt>();
		exprStmt->expression = std::move(group);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(exprStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "3\n");
	}

	// 사이드 이펙트 케이스: var x = 1.0; x = 5.0 (출력 "5\n"); print x → "5\n"
	{
		Token xToken; xToken.lexeme = "x";

		auto varDecl = std::make_unique<VarDeclareStmt>();
		varDecl->name = xToken;
		auto initVal = std::make_unique<LiteralExpr>(); initVal->value = 1.0;
		varDecl->initializer = std::move(initVal);

		auto assignVal = std::make_unique<LiteralExpr>(); assignVal->value = 5.0;
		auto assignExpr = std::make_unique<AssignExpr>();
		assignExpr->name  = xToken;
		assignExpr->value = std::move(assignVal);
		auto exprStmt = std::make_unique<ExpressionStmt>();
		exprStmt->expression = std::move(assignExpr);

		auto varExpr = std::make_unique<VariableExpr>(); varExpr->name = xToken;
		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = std::move(varExpr);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(varDecl));
		stmts.push_back(std::move(exprStmt));
		stmts.push_back(std::move(printStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "5\n5\n");
	}
}

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
TEST(ExecutorTest, ForStatement)
{
	// for (var i = 0; i < 3; i = i + 1) { print i; }
	// expected: "0\n1\n2\n"
	auto makeLit = [](double v) -> std::unique_ptr<Expr> {
		auto e = std::make_unique<LiteralExpr>(); e->value = v; return e;
	};
	auto makeVar = [](const Token& t) -> std::unique_ptr<Expr> {
		auto e = std::make_unique<VariableExpr>(); e->name = t; return e;
	};
	auto makeBin = [](std::unique_ptr<Expr> l, TokenType op, std::unique_ptr<Expr> r) -> std::unique_ptr<Expr> {
		Token t; t.type = op;
		auto e = std::make_unique<BinaryExpr>();
		e->left = std::move(l); e->op = t; e->right = std::move(r);
		return e;
	};

	Token iToken; iToken.lexeme = "i";

	auto init = std::make_unique<VarDeclareStmt>();
	init->name = iToken; init->initializer = makeLit(0.0);

	auto increment = std::make_unique<AssignExpr>();
	increment->name  = iToken;
	increment->value = makeBin(makeVar(iToken), TokenType::PLUS, makeLit(1.0));

	auto body = std::make_unique<PrintStmt>();
	body->expression = makeVar(iToken);

	auto forStmt = std::make_unique<ForStmt>();
	forStmt->init      = std::move(init);
	forStmt->condition = makeBin(makeVar(iToken), TokenType::LESS, makeLit(3.0));
	forStmt->increment = std::move(increment);
	forStmt->body      = std::move(body);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(forStmt));

	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	ASSERT_EQ(testing::internal::GetCapturedStdout(), "0\n1\n2\n");
}

// TC7: LEFT_BRACE → 새 변수 저장소 생성, RIGHT_BRACE → 변수 저장소 소멸 (Shadowing으로 검증)
TEST(ExecutorTest, BlockScope_ScopeLifecycle)
{
	// var x = 1.0;
	// { var x = 2.0; print x; }  → "2"
	// print x;                    → "1"
	Token xToken; xToken.lexeme = "x";

	auto makeVarDecl = [](const Token& t, double v) {
		auto init = std::make_unique<LiteralExpr>(); init->value = v;
		auto decl = std::make_unique<VarDeclareStmt>();
		decl->name = t; decl->initializer = std::move(init);
		return decl;
	};
	auto makePrintVar = [](const Token& t) {
		auto e = std::make_unique<VariableExpr>(); e->name = t;
		auto s = std::make_unique<PrintStmt>(); s->expression = std::move(e);
		return s;
	};

	auto block = std::make_unique<BlockStmt>();
	block->statements.push_back(makeVarDecl(xToken, 2.0));
	block->statements.push_back(makePrintVar(xToken));

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(makeVarDecl(xToken, 1.0));
	stmts.push_back(std::move(block));
	stmts.push_back(makePrintVar(xToken));

	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	ASSERT_EQ(testing::internal::GetCapturedStdout(), "2\n1\n");
}

// TC7-1: 중첩 블록마다 독립적인 로컬 저장소가 생성/소멸되는지 확인
TEST(ExecutorTest, BlockScope_NestedScopes)
{
	// var x = 1.0;
	// { var x = 2.0; { var x = 3.0; print x; }  → "3"
	//                  print x; }                → "2"
	// print x;                                   → "1"
	Token xToken; xToken.lexeme = "x";

	auto makeVarDecl = [](const Token& t, double v) {
		auto init = std::make_unique<LiteralExpr>(); init->value = v;
		auto decl = std::make_unique<VarDeclareStmt>();
		decl->name = t; decl->initializer = std::move(init);
		return decl;
	};
	auto makePrintVar = [](const Token& t) {
		auto e = std::make_unique<VariableExpr>(); e->name = t;
		auto s = std::make_unique<PrintStmt>(); s->expression = std::move(e);
		return s;
	};

	// inner block: { var x = 3.0; print x; }
	auto innerBlock = std::make_unique<BlockStmt>();
	innerBlock->statements.push_back(makeVarDecl(xToken, 3.0));
	innerBlock->statements.push_back(makePrintVar(xToken));

	// outer block: { var x = 2.0; innerBlock; print x; }
	auto outerBlock = std::make_unique<BlockStmt>();
	outerBlock->statements.push_back(makeVarDecl(xToken, 2.0));
	outerBlock->statements.push_back(std::move(innerBlock));
	outerBlock->statements.push_back(makePrintVar(xToken));

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(makeVarDecl(xToken, 1.0));
	stmts.push_back(std::move(outerBlock));
	stmts.push_back(makePrintVar(xToken));

	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	ASSERT_EQ(testing::internal::GetCapturedStdout(), "3\n2\n1\n");
}

// TC8: 선언되지 않은 변수 참조 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, UndefinedVariable)
{
	// print abc;  // 'abc' 미선언
	Token abcToken; abcToken.lexeme = "abc";

	auto varExpr = std::make_unique<VariableExpr>();
	varExpr->name = abcToken;

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(varExpr);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(printStmt));

	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC9: 타입 불일치 연산(number + string 등) 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, TypeError)
{
	// print 1.0 + "HI";
	auto left = std::make_unique<LiteralExpr>();
	left->value = 1.0;

	auto right = std::make_unique<LiteralExpr>();
	right->value = std::string("HI");

	Token plusOp; plusOp.type = TokenType::PLUS;

	auto binExpr = std::make_unique<BinaryExpr>();
	binExpr->left  = std::move(left);
	binExpr->op    = plusOp;
	binExpr->right = std::move(right);

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(binExpr);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(printStmt));

	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC10: 0으로 나누기 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, DivideByZero)
{
	// print 10.0 / 0.0;
	auto left = std::make_unique<LiteralExpr>();
	left->value = 10.0;

	auto right = std::make_unique<LiteralExpr>();
	right->value = 0.0;

	Token slashOp; slashOp.type = TokenType::SLASH;

	auto binExpr = std::make_unique<BinaryExpr>();
	binExpr->left  = std::move(left);
	binExpr->op    = slashOp;
	binExpr->right = std::move(right);

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(binExpr);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(printStmt));

	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}
