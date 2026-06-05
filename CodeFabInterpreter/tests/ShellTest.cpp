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

// TC-PS-06: if 문 정상 실행 — if (x > 0) { print x; } → stdout "5\n"
// Arrange: var x = 5; 선언 후 if 문 입력
// Act    : runLine() 두 번 순차 호출
// Assert : if 조건 참일 때 본문 실행
TEST(ShellTest, IfStatementInShell)
{
    Shell shell;
    shell.runLine("var x = 5;");

    testing::internal::CaptureStdout();
    shell.runLine("if (x > 0) { print x; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n");
}

// TC-PS-07: 불완전한 if 문 → 파싱 에러 후 Shell 계속 동작
// Arrange: if (x > 0) 만 입력 → EOF 에서 파싱 에러
// Act    : runLine() 두 번 순차 호출
// Assert : 파싱 에러 후에도 Shell 이 살아있어 다음 명령이 정상 동작
TEST(ShellTest, IncompleteIfShellContinues)
{
    Shell shell;
    shell.runLine("var x = 5;");
    shell.runLine("if (x > 0)"); // 불완전 — 파싱 에러 발생, Shell 은 계속 동작해야 함

    testing::internal::CaptureStdout();
    shell.runLine("print x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n");
}

// TC-PS-08: exit / quit 입력 시 에러 없이 조용히 처리 (대소문자 무관)
// Arrange: exit / EXIT / quit / QUIT 입력
// Act    : runLine() 호출
// Assert : stdout 출력 없음 (에러 메시지 출력 안 됨)
TEST(ShellTest, ExitQuitCommand)
{
    Shell shell;
    for (const auto& cmd : {"exit", "EXIT", "Exit", "quit", "QUIT", "Quit"})
    {
        testing::internal::CaptureStdout();
        shell.runLine(cmd);
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "") << "input: " << cmd;
    }
}

// ================================================================
// Negative UT
// ================================================================

// TC-PS-10: Checker 에러 후 이전 runLine에서 선언된 변수 상태가 유지되는지 확인
// Arrange: var a = 5; 선언 후 자기 참조 에러 발생 (var b = b;)
// Act    : runLine() 세 번 순차 호출
// Assert : Checker 에러 후에도 a == 5 상태 보존
TEST(ShellTest, CheckerErrorDoesNotCorruptState)
{
    Shell shell;
    shell.runLine("var a = 5;");
    shell.runLine("var b = b;"); // Checker 에러 — b는 executor에 등록되지 않음

    testing::internal::CaptureStdout();
    shell.runLine("print a;");   // a는 여전히 5
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n");
}

// TC-PS-11: 별도 runLine 호출에서 동일 변수명을 재선언하면 Checker 에러가 발생하는지 확인
// Arrange: var x = 1; 선언 후 동일 이름 재선언
// Act    : runLine() 세 번 순차 호출
// Assert : 두 번째 선언에서 [Checker] 에러 출력, x 값은 1 유지
TEST(ShellTest, DuplicateVarAcrossRunLine)
{
    Shell shell;
    shell.runLine("var x = 1;");

    testing::internal::CaptureStdout();
    shell.runLine("var x = 2;"); // 중복 선언 → Checker 에러
    std::string errOut = testing::internal::GetCapturedStdout();
    EXPECT_NE(errOut.find("[Checker]"), std::string::npos); // 에러 메시지 출력 확인

    // x는 여전히 1이어야 한다 (Executor가 호출되지 않았으므로)
    testing::internal::CaptureStdout();
    shell.runLine("print x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// TC-PS-09: 함수 선언과 호출이 별도 runLine() 호출에 걸쳐 동작해야 함
// Arrange: 첫 번째 runLine()에서 func 선언 (파싱된 AST는 runLine 종료 시 소멸)
// Act    : 두 번째 runLine()에서 함수 호출
// Assert : stdout == "7\n" — AST 소멸 후에도 함수 body가 유효해야 함
// [Regression] FunctionValue::body 가 raw Stmt* 일 때 dangling pointer로 crash 발생
TEST(ShellTest, FunctionPersistsAcrossRunLine)
{
    Shell shell;
    shell.runLine("func add(a, b) { return a + b; }");

    testing::internal::CaptureStdout();
    shell.runLine("print add(3, 4);");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "7\n");
}
