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
TEST(ExecutorTest, PrintLiteral) { ASSERT_TRUE(false); }

// TC3: BinaryExpr 사칙연산 결과가 올바르게 계산되는지 확인
TEST(ExecutorTest, ArithmeticExpr) { ASSERT_TRUE(false); }

// TC4: VarDeclareStmt로 선언한 변수를 VariableExpr로 참조할 수 있는지 확인
TEST(ExecutorTest, VarDeclareAndUse) { ASSERT_TRUE(false); }

// TC5: IfStmt의 condition 결과에 따라 thenBranch/elseBranch가 선택 실행되는지 확인
TEST(ExecutorTest, IfStatement) { ASSERT_TRUE(false); }

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
