// ShellTest.cpp — UTF-8 BOM
#include "gtest/gtest.h"
#include "../Shell.h"

class ShellTest : public ::testing::Test
{
protected:
    Shell shell;
};

// TC-PS-01: print 문 실행 → stdout 출력 확인
TEST_F(ShellTest, PrintStatement)
{
    testing::internal::CaptureStdout();
    shell.runLine("print 1+2;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "3\n");
}

// TC-PS-02: var 선언 → 출력 없음, 예외 없음
TEST_F(ShellTest, VarDeclaration)
{
    testing::internal::CaptureStdout();
    EXPECT_NO_THROW(shell.runLine("var x = 10;"));
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// TC-PS-03: runLine 호출 간 변수 상태 유지
TEST_F(ShellTest, StatePersistence)
{
    testing::internal::CaptureStdout();
    shell.runLine("var x = 42;");
    testing::internal::GetCapturedStdout();

    testing::internal::CaptureStdout();
    shell.runLine("print x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "42\n");
}

// TC-PS-04: Checker 에러(자기 참조 var a = a;) 발생 후 Shell 계속 동작
TEST_F(ShellTest, CheckerError)
{
    shell.runLine("var a = a;");

    testing::internal::CaptureStdout();
    shell.runLine("print 1;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// TC-PS-05: 런타임 에러(0 나누기) 발생 후 Shell 계속 동작
TEST_F(ShellTest, RuntimeError)
{
    shell.runLine("print 1/0;");

    testing::internal::CaptureStdout();
    shell.runLine("print 2;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "2\n");
}

// TC-PS-06: if 문 정상 실행 — if (x > 0) { print x; } → stdout "5\n"
TEST_F(ShellTest, IfStatementInShell)
{
    shell.runLine("var x = 5;");

    testing::internal::CaptureStdout();
    shell.runLine("if (x > 0) { print x; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n");
}

// TC-PS-07: 불완전한 if 문 → 파싱 에러 후 Shell 계속 동작
TEST_F(ShellTest, IncompleteIfShellContinues)
{
    shell.runLine("var x = 5;");
    shell.runLine("if (x > 0)");

    testing::internal::CaptureStdout();
    shell.runLine("print x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n");
}

// TC-PS-08: exit / quit 입력 시 에러 없이 조용히 처리 (대소문자 무관)
TEST_F(ShellTest, ExitQuitCommand)
{
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
TEST_F(ShellTest, CheckerErrorDoesNotCorruptState)
{
    shell.runLine("var a = 5;");
    shell.runLine("var b = b;");

    testing::internal::CaptureStdout();
    shell.runLine("print a;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n");
}

// TC-PS-11: 별도 runLine 호출에서 동일 변수명을 재선언하면 Checker 에러가 발생하는지 확인
TEST_F(ShellTest, DuplicateVarAcrossRunLine)
{
    shell.runLine("var x = 1;");

    testing::internal::CaptureStdout();
    shell.runLine("var x = 2;");
    std::string errOut = testing::internal::GetCapturedStdout();
    EXPECT_NE(errOut.find("[Checker]"), std::string::npos);

    testing::internal::CaptureStdout();
    shell.runLine("print x;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// TC-PS-12: 파서 에러 발생 시 [Error] prefix가 출력된다
TEST_F(ShellTest, ParserErrorOutputsErrorPrefix)
{
    testing::internal::CaptureStdout();
    shell.runLine("if (true)");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("[Error]"), std::string::npos);
}

// TC-PS-09: 함수 선언과 호출이 별도 runLine() 호출에 걸쳐 동작해야 함
// [Regression] FunctionValue::body 가 raw Stmt* 일 때 dangling pointer로 crash 발생
TEST_F(ShellTest, FunctionPersistsAcrossRunLine)
{
    shell.runLine("func add(a, b) { return a + b; }");

    testing::internal::CaptureStdout();
    shell.runLine("print add(3, 4);");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "7\n");
}
