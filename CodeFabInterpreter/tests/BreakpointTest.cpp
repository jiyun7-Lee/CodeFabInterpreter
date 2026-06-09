#include <gtest/gtest.h>
#include "../DebugController.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include "TestHelpers.h"

class BreakpointTest : public ::testing::Test
{
protected:
    BreakpointManager mgr;
};

// TC-BP-01: break N → isBreakpoint(N) == true
TEST_F(BreakpointTest, TC_BP_01_AddBreakpoint)
{
    mgr.add(5);
    EXPECT_TRUE(mgr.isBreakpoint(5));
    EXPECT_FALSE(mgr.isBreakpoint(3));
}

// TC-BP-02: remove N → isBreakpoint(N) == false
TEST_F(BreakpointTest, TC_BP_02_RemoveBreakpoint)
{
    mgr.add(5);
    mgr.remove(5);
    EXPECT_FALSE(mgr.isBreakpoint(5));
}

// TC-BP-03: breakpoints → 등록된 줄번호 포함 출력
TEST_F(BreakpointTest, TC_BP_03_PrintBreakpoints)
{
    mgr.add(3);
    mgr.add(7);
    auto out = captureOutput([&]{ mgr.print(); });
    EXPECT_NE(out.find("3"), std::string::npos);
    EXPECT_NE(out.find("7"), std::string::npos);
}

// TC-BP-04: breakpoint 줄 도달 시 PAUSED 상태로 전환
TEST_F(BreakpointTest, TC_BP_04_PausesAtBreakpointLine)
{
    Tokenizer tokenizer;
    Parser parser;
    auto stmts = parser.parse(tokenizer.tokenize("var a = 1;\nprint a;"));

    PauseTrackingController ctrl;
    ctrl.addBreakpoint(2);
    ctrl.setRunning();

    Executor executor;
    executor.setDebugController(&ctrl);

    captureOutput([&]{ executor.execute(stmts); });

    EXPECT_EQ(ctrl.pausedAtLine, 2);
}

// TC-BP-05: breakpoint 도달 후 step → 다음 줄로 이동 (같은 줄 반복 정지 없음)
// [Regression] DebugShell 블록 단위 파싱 시 모든 Stmt가 동일 effectiveLine을 가져
// breakpoint 이후 step해도 같은 줄에 반복 정지되던 버그 방지
TEST_F(BreakpointTest, TC_BP_05_StepAfterBreakpointMovesToNextLine)
{
    Tokenizer tokenizer;
    Parser parser;
    auto stmts = parser.parse(tokenizer.tokenize("var a = 1;\nprint a;\nprint 2;"));

    ScriptedController ctrl;
    using Cmd = ScriptedController::Cmd;
    ctrl.addBreakpoint(2);
    ctrl.setRunning();
    ctrl.script = { Cmd::STEP };  // breakpoint 정지 후 step 한 번

    Executor executor;
    executor.setDebugController(&ctrl);
    captureOutput([&]{ executor.execute(stmts); });

    // 줄 2 breakpoint → step → 줄 3, 같은 줄 반복 없음
    ASSERT_EQ(ctrl.pausedLines.size(), 2u);
    EXPECT_EQ(ctrl.pausedLines[0], 2);
    EXPECT_EQ(ctrl.pausedLines[1], 3);
}
