#include <gtest/gtest.h>
#include "../Executor.h"
#include "../Tokenizer.h"
#include "../Parser.h"


// TC0: ExpressionStmt 평가 — 순수 계산은 예외 없이 실행, AssignExpr 사이드 이펙트 반영 확인
TEST(ExecutorTest, ExpressionStatement)
{
	// 순수 계산 케이스: 1.0 + 2.0 → 예외 없이 실행, stdout 출력 없음
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
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "");
	}

	// 사이드 이펙트 케이스: var x = 1.0; x = 5.0; print x; → "5\n"
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
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "5\n");
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
		{ 3.0,  TokenType::PLUS,    4.0, "7\n"  },
		{ 10.0, TokenType::MINUS,   3.0, "7\n"  },
		{ 2.0,  TokenType::STAR,    5.0, "10\n" },
		{ 9.0,  TokenType::SLASH,   3.0, "3\n"  },
		{ 10.0, TokenType::PERCENT, 3.0, "1\n"  },
		{ 0.0,  TokenType::PERCENT, 5.0, "0\n"  },
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

// TC11-1: UnaryExpr MINUS — 숫자 부호 반전 확인
TEST(ExecutorTest, UnaryExpr_Minus)
{
	Token minusOp; minusOp.type = TokenType::MINUS;
	auto lit = std::make_unique<LiteralExpr>(); lit->value = 3.0;
	auto unary = std::make_unique<UnaryExpr>();
	unary->op = minusOp; unary->right = std::move(lit);

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(unary);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(printStmt));

	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	ASSERT_EQ(testing::internal::GetCapturedStdout(), "-3\n");
}

// TC11-2: UnaryExpr BANG — 논리 반전 확인
TEST(ExecutorTest, UnaryExpr_Bang)
{
	Token bangOp; bangOp.type = TokenType::BANG;
	auto lit = std::make_unique<LiteralExpr>(); lit->value = true;
	auto unary = std::make_unique<UnaryExpr>();
	unary->op = bangOp; unary->right = std::move(lit);

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(unary);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(printStmt));

	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	ASSERT_EQ(testing::internal::GetCapturedStdout(), "false\n");
}

// TC11-3: UnaryExpr MINUS 타입 불일치 — RuntimeError 확인
TEST(ExecutorTest, UnaryExpr_TypeMismatch)
{
	Token minusOp; minusOp.type = TokenType::MINUS;
	auto lit = std::make_unique<LiteralExpr>(); lit->value = std::string("hello");
	auto unary = std::make_unique<UnaryExpr>();
	unary->op = minusOp; unary->right = std::move(lit);

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(unary);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(printStmt));

	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC12: GroupingExpr — 괄호 내부 expression이 올바르게 평가되는지 확인
TEST(ExecutorTest, GroupingExpr)
{
	// (1.0 + 2.0) → "3\n"
	{
		auto left  = std::make_unique<LiteralExpr>(); left->value  = 1.0;
		auto right = std::make_unique<LiteralExpr>(); right->value = 2.0;
		Token plusOp; plusOp.type = TokenType::PLUS;

		auto binExpr = std::make_unique<BinaryExpr>();
		binExpr->left = std::move(left); binExpr->op = plusOp; binExpr->right = std::move(right);

		auto group = std::make_unique<GroupingExpr>();
		group->expression = std::move(binExpr);

		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = std::move(group);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(printStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "3\n");
	}

	// (-(3.0)) → 중첩 GroupingExpr + UnaryExpr → "-3\n"
	{
		Token minusOp; minusOp.type = TokenType::MINUS;
		auto lit = std::make_unique<LiteralExpr>(); lit->value = 3.0;

		auto unary = std::make_unique<UnaryExpr>();
		unary->op = minusOp; unary->right = std::move(lit);

		auto group = std::make_unique<GroupingExpr>();
		group->expression = std::move(unary);

		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = std::move(group);

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(printStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), "-3\n");
	}
}

// TC13: BinaryExpr AND — 단락 평가, true/false 조합 결과 확인
TEST(ExecutorTest, LogicalAnd)
{
	auto makeLitBool = [](bool v) -> std::unique_ptr<Expr> {
		auto e = std::make_unique<LiteralExpr>(); e->value = v; return e;
	};
	auto makeAnd = [](std::unique_ptr<Expr> l, std::unique_ptr<Expr> r) -> std::unique_ptr<Expr> {
		Token t; t.type = TokenType::AND;
		auto e = std::make_unique<BinaryExpr>();
		e->left = std::move(l); e->op = t; e->right = std::move(r);
		return e;
	};

	struct Case { bool l; bool r; std::string expected; };
	std::vector<Case> cases = {
		{ true,  true,  "true\n"  },
		{ true,  false, "false\n" },
		{ false, true,  "false\n" },
		{ false, false, "false\n" },
	};

	for (const auto& c : cases)
	{
		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = makeAnd(makeLitBool(c.l), makeLitBool(c.r));

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(printStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), c.expected);
	}

	// 단락 평가: 왼쪽이 false면 오른쪽 평가 안 함 (RuntimeError 미발생)
	{
		Token divOp; divOp.type = TokenType::SLASH;
		auto zero  = std::make_unique<LiteralExpr>(); zero->value  = 0.0;
		auto one   = std::make_unique<LiteralExpr>(); one->value   = 1.0;
		auto divByZero = std::make_unique<BinaryExpr>();
		divByZero->left = std::move(one); divByZero->op = divOp; divByZero->right = std::move(zero);

		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = makeAnd(makeLitBool(false), std::move(divByZero));

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(printStmt));

		Executor executor;
		ASSERT_NO_THROW(executor.execute(stmts));
	}
}

// [Regression #4] for 루프 init 변수는 루프 종료 후 소멸해야 함
// for (var i = 0; ...) {} 이후 print i; → RuntimeError (i는 for 스코프 내에서만 유효)
TEST(ExecutorTest, ForScope_InitVarNotLeaking)
{
    Tokenizer tz;
    Parser parser;
    auto stmts = parser.parse(tz.tokenize(
        "for (var i = 0; i < 3; i = i + 1) {}"
        "print i;"));
    Executor executor;
    ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// [Regression #11] 런타임 에러 메시지에 줄 번호 + 한국어 형식 포함 확인
// TC-ErrMsg-1: 미정의 변수 → "[N번째 줄] 미정의된 변수 'x'"
TEST(ExecutorTest, ErrorMessage_UndefinedVariable)
{
    Tokenizer tz;
    Parser parser;
    auto stmts = parser.parse(tz.tokenize("print abc;"));
    Executor executor;
    try {
        executor.execute(stmts);
        FAIL() << "RuntimeError expected";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("번째 줄"), std::string::npos) << "줄 번호 없음: " << msg;
        EXPECT_NE(msg.find("미정의된 변수"), std::string::npos) << "메시지 형식 불일치: " << msg;
    }
}

// TC-ErrMsg-2: 타입 불일치 → "[N번째 줄] 피연산자는 반드시 숫자여야 한다."
TEST(ExecutorTest, ErrorMessage_TypeError)
{
    Tokenizer tz;
    Parser parser;
    auto stmts = parser.parse(tz.tokenize("print 1 + \"hi\";"));
    Executor executor;
    try {
        executor.execute(stmts);
        FAIL() << "RuntimeError expected";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("번째 줄"), std::string::npos) << "줄 번호 없음: " << msg;
        EXPECT_NE(msg.find("피연산자"), std::string::npos) << "메시지 형식 불일치: " << msg;
    }
}

// TC-ErrMsg-3: 0 나누기 → "[N번째 줄] 0으로 나눈 오류"
TEST(ExecutorTest, ErrorMessage_DivideByZero)
{
    Tokenizer tz;
    Parser parser;
    auto stmts = parser.parse(tz.tokenize("print 1 / 0;"));
    Executor executor;
    try {
        executor.execute(stmts);
        FAIL() << "RuntimeError expected";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("번째 줄"), std::string::npos) << "줄 번호 없음: " << msg;
        EXPECT_NE(msg.find("0으로 나눈"), std::string::npos) << "메시지 형식 불일치: " << msg;
    }
}

// ================================================================
// Negative UT
// ================================================================

// TC15: 문자열(lhs) + 숫자 연산 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, StringPlusNumberThrows)
{
	auto left  = std::make_unique<LiteralExpr>(); left->value  = std::string("hello");
	auto right = std::make_unique<LiteralExpr>(); right->value = 1.0;
	Token op; op.type = TokenType::PLUS;
	auto bin = std::make_unique<BinaryExpr>();
	bin->left = std::move(left); bin->op = op; bin->right = std::move(right);
	auto stmt = std::make_unique<PrintStmt>();
	stmt->expression = std::move(bin);
	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC16: 문자열끼리 대소 비교(>) 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, StringComparisonThrows)
{
	auto left  = std::make_unique<LiteralExpr>(); left->value  = std::string("a");
	auto right = std::make_unique<LiteralExpr>(); right->value = std::string("b");
	Token op; op.type = TokenType::GREATER;
	auto bin = std::make_unique<BinaryExpr>();
	bin->left = std::move(left); bin->op = op; bin->right = std::move(right);
	auto stmt = std::make_unique<PrintStmt>();
	stmt->expression = std::move(bin);
	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC17: bool + 숫자 연산 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, BoolArithmeticThrows)
{
	auto left  = std::make_unique<LiteralExpr>(); left->value  = true;
	auto right = std::make_unique<LiteralExpr>(); right->value = 1.0;
	Token op; op.type = TokenType::PLUS;
	auto bin = std::make_unique<BinaryExpr>();
	bin->left = std::move(left); bin->op = op; bin->right = std::move(right);
	auto stmt = std::make_unique<PrintStmt>();
	stmt->expression = std::move(bin);
	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC18: null + 숫자 연산 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, NullArithmeticThrows)
{
	auto left  = std::make_unique<LiteralExpr>(); left->value  = std::monostate{};
	auto right = std::make_unique<LiteralExpr>(); right->value = 1.0;
	Token op; op.type = TokenType::PLUS;
	auto bin = std::make_unique<BinaryExpr>();
	bin->left = std::move(left); bin->op = op; bin->right = std::move(right);
	auto stmt = std::make_unique<PrintStmt>();
	stmt->expression = std::move(bin);
	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC19: 선언되지 않은 변수에 대입 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, AssignUndeclaredVarThrows)
{
	Token xToken; xToken.lexeme = "x";
	auto assignVal = std::make_unique<LiteralExpr>(); assignVal->value = 5.0;
	auto assignExpr = std::make_unique<AssignExpr>();
	assignExpr->name  = xToken;
	assignExpr->value = std::move(assignVal);
	auto stmt = std::make_unique<ExpressionStmt>();
	stmt->expression = std::move(assignExpr);
	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC20: null 반환 함수 결과를 산술 연산에 사용 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, NullReturnArithmeticThrows)
{
	Token fTok; fTok.lexeme = "f";

	// func f() {}
	auto fnBody = std::make_unique<BlockStmt>();
	auto fnDecl = std::make_unique<FunctionDeclareStmt>();
	fnDecl->name = fTok;
	fnDecl->body = std::move(fnBody);

	// f() + 1.0
	auto callExpr = std::make_unique<FunctionCallExpr>();
	callExpr->callee = fTok;

	auto one = std::make_unique<LiteralExpr>(); one->value = 1.0;
	Token plusOp; plusOp.type = TokenType::PLUS;
	auto bin = std::make_unique<BinaryExpr>();
	bin->left = std::move(callExpr); bin->op = plusOp; bin->right = std::move(one);

	auto printStmt = std::make_unique<PrintStmt>();
	printStmt->expression = std::move(bin);

	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(fnDecl));
	stmts.push_back(std::move(printStmt));

	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC14: BinaryExpr OR — 단락 평가, true/false 조합 결과 확인
TEST(ExecutorTest, LogicalOr)
{
	auto makeLitBool = [](bool v) -> std::unique_ptr<Expr> {
		auto e = std::make_unique<LiteralExpr>(); e->value = v; return e;
	};
	auto makeOr = [](std::unique_ptr<Expr> l, std::unique_ptr<Expr> r) -> std::unique_ptr<Expr> {
		Token t; t.type = TokenType::OR;
		auto e = std::make_unique<BinaryExpr>();
		e->left = std::move(l); e->op = t; e->right = std::move(r);
		return e;
	};

	struct Case { bool l; bool r; std::string expected; };
	std::vector<Case> cases = {
		{ true,  true,  "true\n"  },
		{ true,  false, "true\n"  },
		{ false, true,  "true\n"  },
		{ false, false, "false\n" },
	};

	for (const auto& c : cases)
	{
		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = makeOr(makeLitBool(c.l), makeLitBool(c.r));

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(printStmt));

		Executor executor;
		testing::internal::CaptureStdout();
		executor.execute(stmts);
		ASSERT_EQ(testing::internal::GetCapturedStdout(), c.expected);
	}

	// 단락 평가: 왼쪽이 true면 오른쪽 평가 안 함 (RuntimeError 미발생)
	{
		Token divOp; divOp.type = TokenType::SLASH;
		auto zero = std::make_unique<LiteralExpr>(); zero->value = 0.0;
		auto one  = std::make_unique<LiteralExpr>(); one->value  = 1.0;
		auto divByZero = std::make_unique<BinaryExpr>();
		divByZero->left = std::move(one); divByZero->op = divOp; divByZero->right = std::move(zero);

		auto printStmt = std::make_unique<PrintStmt>();
		printStmt->expression = makeOr(makeLitBool(true), std::move(divByZero));

		std::vector<std::unique_ptr<Stmt>> stmts;
		stmts.push_back(std::move(printStmt));

		Executor executor;
		ASSERT_NO_THROW(executor.execute(stmts));
	}
}

// TC22: 0으로 나누기 모듈러 연산 시 RuntimeError가 발생하는지 확인
TEST(ExecutorTest, ModuloByZero)
{
	auto left  = std::make_unique<LiteralExpr>(); left->value  = 10.0;
	auto right = std::make_unique<LiteralExpr>(); right->value = 0.0;
	Token op; op.type = TokenType::PERCENT;
	auto bin = std::make_unique<BinaryExpr>();
	bin->left = std::move(left); bin->op = op; bin->right = std::move(right);
	auto stmt = std::make_unique<PrintStmt>();
	stmt->expression = std::move(bin);
	std::vector<std::unique_ptr<Stmt>> stmts;
	stmts.push_back(std::move(stmt));
	Executor executor;
	ASSERT_THROW(executor.execute(stmts), std::runtime_error);
}

// TC21: 초기화 없는 var 선언 후 print → "null\n"
TEST(ExecutorTest, VarNoInitializerPrintsNull)
{
	Tokenizer tz;
	auto tokens = tz.tokenize("var x; print x;");
	Parser parser;
	auto stmts = parser.parse(tokens);
	Executor executor;
	testing::internal::CaptureStdout();
	executor.execute(stmts);
	EXPECT_EQ(testing::internal::GetCapturedStdout(), "null\n");
}
