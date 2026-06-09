#include <gtest/gtest.h>
#include "../Shell.h"
#include "../DebugController.h"
#include "TestHelpers.h"

// ================================================================
// NegativeFixture — REPL / File / Debug 각 모드 에러 검증 공통 픽스처
// ================================================================

class NegativeFixture : public ::testing::Test
{
protected:
    Shell     shell;
    DebugShell debugShell;

    std::string runShell(const std::string& source)
    {
        testing::internal::CaptureStdout();
        shell.runLine(source);
        return testing::internal::GetCapturedStdout();
    }

    std::string runFile(const std::vector<std::string>& lines)
    {
        FileRunner runner;
        testing::internal::CaptureStdout();
        runner.runSource(lines);
        return testing::internal::GetCapturedStdout();
    }

    std::string runDebug(const std::vector<std::string>& lines)
    {
        NonBlockingController ctrl;
        return captureOutput([&]{ debugShell.runSource(lines, ctrl); });
    }

    void expectContains(const std::string& output, const std::string& keyword)
    {
        EXPECT_NE(output.find(keyword), std::string::npos)
            << "Expected to find [" << keyword << "] in output:\n" << output;
    }
};

// ================================================================
// NT_01 — 정의되지 않은 변수 참조 (런타임 에러)
// ================================================================

TEST_F(NegativeFixture, NT_01_Shell_UndefinedVariable)
{
    expectContains(runShell("print notDefined;"), "[Error]");
}

TEST_F(NegativeFixture, NT_01_File_UndefinedVariable)
{
    expectContains(runFile({"print notDefined;"}), "[Error]");
}

TEST_F(NegativeFixture, NT_01_Debug_UndefinedVariable)
{
    expectContains(runDebug({"print notDefined;"}), "[Error]");
}

// ================================================================
// NT_02 — + 연산자 숫자/문자열 혼용 타입 오류
// ================================================================

TEST_F(NegativeFixture, NT_02_Shell_TypeMismatchPlus)
{
    expectContains(runShell("print 1 + \"HI\";"), "[Error]");
}

TEST_F(NegativeFixture, NT_02_File_TypeMismatchPlus)
{
    expectContains(runFile({"print 1 + \"HI\";"}), "[Error]");
}

TEST_F(NegativeFixture, NT_02_Debug_TypeMismatchPlus)
{
    expectContains(runDebug({"print 1 + \"HI\";"}), "[Error]");
}

// ================================================================
// NT_03 — 단항 마이너스에 문자열 피연산자 타입 오류
// ================================================================

TEST_F(NegativeFixture, NT_03_Shell_UnaryMinusTypeMismatch)
{
    expectContains(runShell("print -\"FabCoding\";"), "[Error]");
}

TEST_F(NegativeFixture, NT_03_File_UnaryMinusTypeMismatch)
{
    expectContains(runFile({"print -\"FabCoding\";"}), "[Error]");
}

TEST_F(NegativeFixture, NT_03_Debug_UnaryMinusTypeMismatch)
{
    expectContains(runDebug({"print -\"FabCoding\";"}), "[Error]");
}

// ================================================================
// NT_04 — 세미콜론 누락 (파서 에러)
// ================================================================

TEST_F(NegativeFixture, NT_04_Shell_MissingSemicolon)
{
    expectContains(runShell("print 1 + 2"), "[Error]");
}

TEST_F(NegativeFixture, NT_04_File_MissingSemicolon)
{
    expectContains(runFile({"print 1 + 2"}), "[Error]");
}

TEST_F(NegativeFixture, NT_04_Debug_MissingSemicolon)
{
    expectContains(runDebug({"print 1 + 2"}), "[Error]");
}

// ================================================================
// NT_05 — 닫는 괄호 누락 (파서 에러)
// ================================================================

TEST_F(NegativeFixture, NT_05_Shell_MissingClosingParen)
{
    expectContains(runShell("print (1 + 2;"), "[Error]");
}

TEST_F(NegativeFixture, NT_05_File_MissingClosingParen)
{
    expectContains(runFile({"print (1 + 2;"}), "[Error]");
}

TEST_F(NegativeFixture, NT_05_Debug_MissingClosingParen)
{
    expectContains(runDebug({"print (1 + 2;"}), "[Error]");
}

// ================================================================
// NT_06 — 잘못된 할당 대상 (파서 에러)
// ================================================================

TEST_F(NegativeFixture, NT_06_Shell_InvalidAssignmentTarget)
{
    expectContains(runShell("var a = 1;\nvar b = 2;\na + b = 3;"), "[Error]");
}

TEST_F(NegativeFixture, NT_06_File_InvalidAssignmentTarget)
{
    expectContains(runFile({"var a = 1;", "var b = 2;", "a + b = 3;"}), "[Error]");
}

TEST_F(NegativeFixture, NT_06_Debug_InvalidAssignmentTarget)
{
    expectContains(runDebug({"var a = 1;", "var b = 2;", "a + b = 3;"}), "[Error]");
}

// ================================================================
// NT_07 — 예상치 못한 토큰 (파서 에러)
// ================================================================

TEST_F(NegativeFixture, NT_07_Shell_UnexpectedToken)
{
    expectContains(runShell("print * 5;"), "[Error]");
}

TEST_F(NegativeFixture, NT_07_File_UnexpectedToken)
{
    expectContains(runFile({"print * 5;"}), "[Error]");
}

TEST_F(NegativeFixture, NT_07_Debug_UnexpectedToken)
{
    expectContains(runDebug({"print * 5;"}), "[Error]");
}

// ================================================================
// NT_08 — 지역 변수 자기 초기화 (Checker 정적 에러)
// ================================================================

TEST_F(NegativeFixture, NT_08_Shell_SelfInitReference)
{
    expectContains(runShell("{ var a = a; }"), "[Checker]");
}

TEST_F(NegativeFixture, NT_08_File_SelfInitReference)
{
    expectContains(runFile({"{ var a = a; }"}), "[Checker]");
}

TEST_F(NegativeFixture, NT_08_Debug_SelfInitReference)
{
    expectContains(runDebug({"{ var a = a; }"}), "[Checker]");
}

// ================================================================
// NT_09 — 같은 스코프 중복 변수 선언 (Checker 정적 에러)
// ================================================================

TEST_F(NegativeFixture, NT_09_Shell_DuplicateDeclaration)
{
    expectContains(runShell("{ var a = \"hi\"; var a = 3; }"), "[Checker]");
}

TEST_F(NegativeFixture, NT_09_File_DuplicateDeclaration)
{
    expectContains(runFile({"{ var a = \"hi\"; var a = 3; }"}), "[Checker]");
}

TEST_F(NegativeFixture, NT_09_Debug_DuplicateDeclaration)
{
    expectContains(runDebug({"{ var a = \"hi\"; var a = 3; }"}), "[Checker]");
}
