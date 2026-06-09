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
