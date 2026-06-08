#include <gtest/gtest.h>
#include "../DebugController.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"

// stdin 블로킹 없이 step/next/continue를 스크립트로 실행하는 테스트용 컨트롤러
class ScriptedController : public DebugController
{
public:
    std::vector<int> pausedLines;

    enum class Cmd { STEP, NEXT, CONT };
    std::vector<Cmd> script; // 각 정지 시점에 실행할 명령

    void beforeExecute(Stmt* stmt, Environment* /*env*/, int depth) override
    {
        // RUNNING: breakpoint 도달 시에만 정지
        if (state_ == ExecutionState::RUNNING)
        {
            if (!breakpoints_.isBreakpoint(stmt->line)) return;
            state_ = ExecutionState::PAUSED;
        }
        // NEXT: 발행 depth보다 깊은 Stmt는 통과
        if (state_ == ExecutionState::NEXT)
        {
            if (depth > nextDepth_) return;
            state_ = ExecutionState::STEP;
        }

        pausedLines.push_back(stmt->line);

        size_t i    = pausedLines.size() - 1;
        Cmd    cmd  = (i < script.size()) ? script[i] : Cmd::CONT;
        switch (cmd)
        {
            case Cmd::STEP: state_ = ExecutionState::STEP; break;
            case Cmd::NEXT: nextDepth_ = depth; state_ = ExecutionState::NEXT; break;
            case Cmd::CONT: state_ = ExecutionState::RUNNING; break;
        }
    }
};

// TC-STEP-01: step — 블록 내부 Stmt에도 정지
// "var a = 1;\nif (a > 0) {\nprint a;\n}"
// 정지 순서: 1(var) → 2(if) → 2(block) → 3(print)
TEST(StepTest, TC_STEP_01_StepEntersBlockInternals)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("var a = 1;\nif (a > 0) {\nprint a;\n}");
    Parser parser;
    auto stmts = parser.parse(tokens);

    ScriptedController ctrl;
    using Cmd = ScriptedController::Cmd;
    ctrl.script = { Cmd::STEP, Cmd::STEP, Cmd::STEP, Cmd::STEP };

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    // var(1), if(2), block(2), print(3) — 총 4회 정지
    ASSERT_EQ(ctrl.pausedLines.size(), 4u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 2);
    EXPECT_EQ(ctrl.pausedLines[2], 2);
    EXPECT_EQ(ctrl.pausedLines[3], 3);
}

// TC-STEP-02: next — 블록 내부 진입 없이 다음 Stmt에서 정지
// 줄 2(if)에서 next → 블록 내부 skip → 줄 5(print 2)에서 정지
TEST(StepTest, TC_STEP_02_NextSkipsBlockInternals)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("var a = 1;\nif (a > 0) {\nprint a;\n}\nprint 2;");
    Parser parser;
    auto stmts = parser.parse(tokens);

    ScriptedController ctrl;
    using Cmd = ScriptedController::Cmd;
    ctrl.script = { Cmd::STEP, Cmd::NEXT }; // 줄1: step, 줄2(if): next

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    // var(1), if(2) → next → print 2(5)
    ASSERT_EQ(ctrl.pausedLines.size(), 3u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 2);
    EXPECT_EQ(ctrl.pausedLines[2], 5);
}

// TC-STEP-04: next — 함수 호출 body 진입 없이 다음 Stmt에서 정지 ("next-over-call 버그" 수정 검증)
// func foo() {\nprint 99;\n}\nfoo();\nprint 2;
// 줄 1(func): step → 줄 4(foo()): next → 줄 5(print 2): cont
// 버그 시: foo() body 안 줄 2(print 99)에서 잘못 정지됨
TEST(StepTest, TC_STEP_04_NextSkipsFunctionBody)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("func foo() {\nprint 99;\n}\nfoo();\nprint 2;");
    Parser parser;
    auto stmts = parser.parse(tokens);

    ScriptedController ctrl;
    using Cmd = ScriptedController::Cmd;
    ctrl.script = { Cmd::STEP, Cmd::NEXT };  // 줄1(func): step, 줄4(foo()): next

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    // func(1), foo()(4) → next → print 2(5)  — 총 3회 정지
    ASSERT_EQ(ctrl.pausedLines.size(), 3u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 4);
    EXPECT_EQ(ctrl.pausedLines[2], 5);
}

// TC-STEP-03: continue — 다음 breakpoint까지 실행
// breakpoint at 줄 3, 줄 1에서 continue → 줄 3에서 정지
TEST(StepTest, TC_STEP_03_ContinueRunsToBreakpoint)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("print 1;\nprint 2;\nprint 3;");
    Parser parser;
    auto stmts = parser.parse(tokens);

    ScriptedController ctrl;
    using Cmd = ScriptedController::Cmd;
    ctrl.addBreakpoint(3);
    ctrl.script = { Cmd::CONT }; // 줄 1(STEP 초기 정지): continue

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    // 줄 1(step 초기) → continue → breakpoint 줄 3
    ASSERT_EQ(ctrl.pausedLines.size(), 2u);
    EXPECT_EQ(ctrl.pausedLines[0], 1);
    EXPECT_EQ(ctrl.pausedLines[1], 3);
}
