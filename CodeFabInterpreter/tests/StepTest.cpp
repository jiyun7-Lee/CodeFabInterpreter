#include <gtest/gtest.h>
#include "../DebugController.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include "TestHelpers.h"

class StepTest : public ::testing::Test
{
protected:
    Tokenizer         tokenizer;
    Parser            parser;
    ScriptedController ctrl;
    Executor          executor;

    void SetUp() override
    {
        executor.setDebugController(&ctrl);
    }

    std::vector<std::unique_ptr<Stmt>> parse(const std::string& src)
    {
        return parser.parse(tokenizer.tokenize(src));
    }
};

// TC-STEP-01: step — 블록 내부 Stmt에도 정지
TEST_F(StepTest, TC_STEP_01_StepEntersBlockInternals)
{
    using Cmd = ScriptedController::Cmd;
    ctrl.script = { Cmd::STEP, Cmd::STEP, Cmd::STEP, Cmd::STEP };
    auto stmts  = parse("var a = 1;\nif (a > 0) {\nprint a;\n}");

    captureOutput([&]{ executor.execute(stmts); });

    ASSERT_EQ(ctrl.pausedLines.size(), 4u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 2);
    EXPECT_EQ(ctrl.pausedLines[2], 2);
    EXPECT_EQ(ctrl.pausedLines[3], 3);
}

// TC-STEP-02: next — 블록 내부 진입 없이 다음 Stmt에서 정지
TEST_F(StepTest, TC_STEP_02_NextSkipsBlockInternals)
{
    using Cmd = ScriptedController::Cmd;
    ctrl.script = { Cmd::STEP, Cmd::NEXT };
    auto stmts  = parse("var a = 1;\nif (a > 0) {\nprint a;\n}\nprint 2;");

    captureOutput([&]{ executor.execute(stmts); });

    ASSERT_EQ(ctrl.pausedLines.size(), 3u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 2);
    EXPECT_EQ(ctrl.pausedLines[2], 5);
}

// TC-STEP-03: continue — 다음 breakpoint까지 실행
TEST_F(StepTest, TC_STEP_03_ContinueRunsToBreakpoint)
{
    using Cmd = ScriptedController::Cmd;
    ctrl.addBreakpoint(3);
    ctrl.script = { Cmd::CONT };
    auto stmts  = parse("print 1;\nprint 2;\nprint 3;");

    captureOutput([&]{ executor.execute(stmts); });

    ASSERT_EQ(ctrl.pausedLines.size(), 2u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 3);
}

// TC-STEP-04: next — 함수 호출 body 진입 없이 다음 Stmt에서 정지
// [Regression] next-over-call 버그 수정 검증
TEST_F(StepTest, TC_STEP_04_NextSkipsFunctionBody)
{
    using Cmd = ScriptedController::Cmd;
    ctrl.script = { Cmd::STEP, Cmd::NEXT };
    auto stmts  = parse("func foo() {\nprint 99;\n}\nfoo();\nprint 2;");

    captureOutput([&]{ executor.execute(stmts); });

    ASSERT_EQ(ctrl.pausedLines.size(), 3u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 4);
    EXPECT_EQ(ctrl.pausedLines[2], 5);
}
