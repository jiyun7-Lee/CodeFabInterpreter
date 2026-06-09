#include <gtest/gtest.h>
#include "../Shell.h"
#include "../DebugController.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include "TestHelpers.h"

class DebugShellTest : public ::testing::Test
{
protected:
    DebugShell shell;
    Tokenizer  tokenizer;
    Parser     parser;

    std::vector<std::unique_ptr<Stmt>> parse(const std::string& src)
    {
        return parser.parse(tokenizer.tokenize(src));
    }
};

// TC-DSHELL-01: setLineContext 후 effectiveLine 기반으로 breakpoint 체크
TEST_F(DebugShellTest, TC_DSHELL_01_SetLineContext_EffectiveLineForBreakpoint)
{
    auto stmts = parse("print 1;");

    EffectiveLineController ctrl;
    ctrl.addBreakpoint(3);
    ctrl.setRunning();
    ctrl.setLineContext(3, "print 1;");

    Executor executor;
    executor.setDebugController(&ctrl);

    captureOutput([&]{ executor.execute(stmts); });

    EXPECT_EQ(ctrl.capturedLine, 3);
}

// TC-DSHELL-02: setLineContext 없을 때 stmt->line으로 fallback
TEST_F(DebugShellTest, TC_DSHELL_02_EffectiveLine_FallbackToStmtLine)
{
    auto stmts = parse("print 1;\nprint 2;");

    EffectiveLineController ctrl;
    ctrl.addBreakpoint(2);
    ctrl.setRunning();

    Executor executor;
    executor.setDebugController(&ctrl);

    captureOutput([&]{ executor.execute(stmts); });

    EXPECT_EQ(ctrl.capturedLine, 2);
}

// TC-DSHELL-03: [DEBUG] 출력 형식 — "N번째 줄에서 정지 -> 'srcLine'"
TEST_F(DebugShellTest, TC_DSHELL_03_DebugOutputFormat)
{
    std::istringstream fakeInput("step\nstep\n");
    auto* origCin = std::cin.rdbuf(fakeInput.rdbuf());

    DebugController ctrl;
    auto out = captureOutput([&]{ shell.runSource({"var a = 1;", "print a;"}, ctrl); });

    std::cin.rdbuf(origCin);

    EXPECT_NE(out.find("[DEBUG] 1번째 줄에서 정지 -> 'var a = 1;'"),  std::string::npos);
    EXPECT_NE(out.find("[DEBUG] 2번째 줄에서 정지 -> 'print a;'"),    std::string::npos);
}

// TC-DSHELL-04: runSource — 줄 간 변수 공유
TEST_F(DebugShellTest, TC_DSHELL_04_VariableSharingAcrossLines)
{
    NonBlockingController ctrl;
    EXPECT_EQ(captureOutput([&]{ shell.runSource({"var a = 10;", "print a;"}, ctrl); }), "10\n");
}

// TC-DSHELL-05: runSource — 빈 줄은 건너뜀
TEST_F(DebugShellTest, TC_DSHELL_05_EmptyLinesSkipped)
{
    RecordingController ctrl;
    captureOutput([&]{ shell.runSource({"var a = 1;", "", "print a;"}, ctrl); });

    ASSERT_EQ(ctrl.stoppedAtLines.size(), 2u);
    EXPECT_EQ(ctrl.stoppedAtLines[0], 1);
    EXPECT_EQ(ctrl.stoppedAtLines[1], 3);
}

// TC-DSHELL-06: run() — "[DEBUG] 소스코드 로딩: <filepath>" 출력
TEST_F(DebugShellTest, TC_DSHELL_06_LoadingMessage)
{
    const std::string tmpPath = "test_debug_loading.tmp";
    { std::ofstream f(tmpPath); }

    auto out = captureOutput([&]{ shell.run(tmpPath); });
    std::remove(tmpPath.c_str());

    EXPECT_NE(out.find("[DEBUG] 소스코드 로딩:"), std::string::npos);
    EXPECT_NE(out.find(tmpPath),                  std::string::npos);
}
