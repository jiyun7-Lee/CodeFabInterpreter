// ShellTest.cpp — UTF-8 BOM
#include "gtest/gtest.h"
#include "../Shell.h"

// TC-PS-01: print 문 실행 → stdout 출력 확인
// Arrange: print 1+2; 입력
// Act    : runLine() 호출
// Assert : stdout == "3\n"
TEST(ShellTest, PrintStatement)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print 1+2;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "3\n");
}

// TC-PS-02: var 선언 → 출력 없음, 예외 없음
// Arrange: var x = 10; 입력
// Act    : runLine() 호출
// Assert : stdout == "", 예외 없음
TEST(ShellTest, VarDeclaration)
{
    Shell shell;
    testing::internal::CaptureStdout();
    EXPECT_NO_THROW(shell.runLine("var x = 10;"));
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// TC-PS-03: runLine 호출 간 변수 상태 유지
// Arrange: var x = 42; 선언 후 print x; 호출
// Act    : runLine() 두 번 순차 호출
// Assert : 두 번째 호출 stdout == "42\n"
TEST(ShellTest, StatePersistence)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var x = 42;");
    testing::internal::GetCapturedStdout(); // discard

    testing::internal::CaptureStdout();
    shell.runLine("print x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "42\n");
}

// TC-PS-04: Checker 에러(자기 참조 var a = a;) 발생 후 Shell 계속 동작
// Arrange: 자기 참조 선언 후 정상 print 입력
// Act    : runLine() 두 번 순차 호출
// Assert : Checker 에러 후 Shell 이 살아있어 두 번째 출력이 정상 동작
TEST(ShellTest, CheckerError)
{
    Shell shell;
    shell.runLine("var a = a;"); // Checker 에러 — 예외 없이 처리되어야 함

    testing::internal::CaptureStdout();
    shell.runLine("print 1;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// TC-PS-05: 런타임 에러(0 나누기) 발생 후 Shell 계속 동작
// Arrange: 0 나누기 식 입력 후 정상 print 입력
// Act    : runLine() 두 번 순차 호출
// Assert : 런타임 에러 후 Shell 이 살아있어 두 번째 출력이 정상 동작
TEST(ShellTest, RuntimeError)
{
    Shell shell;
    shell.runLine("print 1/0;"); // RuntimeError — 예외 없이 처리되어야 함

    testing::internal::CaptureStdout();
    shell.runLine("print 2;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "2\n");
}
