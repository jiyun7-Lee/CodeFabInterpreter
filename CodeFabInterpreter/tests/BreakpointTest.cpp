#include <gtest/gtest.h>
#include "../DebugController.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"

// stdin 블로킹 없이 breakpoint 도달 시 상태 전환만 검증하는 서브클래스
class PauseTrackingController : public DebugController
{
public:
    int pausedAtLine = -1;
    void setRunning() { state_ = ExecutionState::RUNNING; }
    void beforeExecute(Stmt* stmt, Environment* /*env*/, int /*depth*/) override
    {
        if (state_ == ExecutionState::RUNNING && breakpoints_.isBreakpoint(stmt->line))
        {
            state_       = ExecutionState::PAUSED;
            pausedAtLine = stmt->line;
        }
    }
};

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
    testing::internal::CaptureStdout();
    mgr.print();
    std::string out = testing::internal::GetCapturedStdout();
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

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(ctrl.pausedAtLine, 2);
}
