#include <gtest/gtest.h>
#include "../Shell.h"
#include "../DebugController.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include <sstream>
#include <fstream>
#include <cstdio>

// -----------------------------------------------------------------------
// 테스트용 컨트롤러
// -----------------------------------------------------------------------

// stdin 블로킹 없이 자동 step 진행
class NonBlockingController : public DebugController
{
public:
    void beforeExecute(Stmt*, Environment*, int) override
    {
        state_ = ExecutionState::STEP;
    }
};

// 정지한 줄번호를 기록하며 자동 step
class RecordingController : public DebugController
{
public:
    std::vector<int> stoppedAtLines;
    void beforeExecute(Stmt*, Environment*, int) override
    {
        stoppedAtLines.push_back(currentLineNo_);
        state_ = ExecutionState::STEP;
    }
};

// RUNNING 모드에서 effectiveLine 기반 breakpoint 체크 (stdin 블로킹 없음)
class EffectiveLineController : public DebugController
{
public:
    int capturedLine = -1;
    void setRunning() { state_ = ExecutionState::RUNNING; }
    void beforeExecute(Stmt* stmt, Environment*, int) override
    {
        int effectiveLine = currentLineNo_ > 0 ? currentLineNo_ : stmt->line;
        if (state_ == ExecutionState::RUNNING && breakpoints_.isBreakpoint(effectiveLine))
        {
            state_        = ExecutionState::PAUSED;
            capturedLine  = effectiveLine;
        }
    }
};

// -----------------------------------------------------------------------
// TC-DSHELL-01: setLineContext 후 effectiveLine 기반으로 breakpoint 체크
// stmt->line = 1이지만 setLineContext(3)이면 breakpoint(3)에서 정지해야 함
// -----------------------------------------------------------------------
TEST(DebugShellTest, TC_DSHELL_01_SetLineContext_EffectiveLineForBreakpoint)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("print 1;");  // stmt->line = 1
    Parser parser;
    auto stmts = parser.parse(tokens);

    EffectiveLineController ctrl;
    ctrl.addBreakpoint(3);
    ctrl.setRunning();
    ctrl.setLineContext(3, "print 1;");

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(ctrl.capturedLine, 3);
}

// TC-DSHELL-02: setLineContext 없을 때 stmt->line으로 fallback
TEST(DebugShellTest, TC_DSHELL_02_EffectiveLine_FallbackToStmtLine)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("print 1;\nprint 2;");
    Parser parser;
    auto stmts = parser.parse(tokens);

    EffectiveLineController ctrl;
    ctrl.addBreakpoint(2);
    ctrl.setRunning();
    // setLineContext 미호출 → effectiveLine = stmt->line

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(ctrl.capturedLine, 2);
}

// TC-DSHELL-03: [DEBUG] 출력 형식 — "N번째 줄에서 정지 -> 'srcLine'"
// fake stdin으로 "step" 주입하여 stdin 블로킹 해소
TEST(DebugShellTest, TC_DSHELL_03_DebugOutputFormat)
{
    std::istringstream fakeInput("step\nstep\n");
    auto* origCin = std::cin.rdbuf(fakeInput.rdbuf());

    DebugShell shell;
    testing::internal::CaptureStdout();
    shell.runSource({"var a = 1;", "print a;"});
    std::string out = testing::internal::GetCapturedStdout();

    std::cin.rdbuf(origCin);

    EXPECT_NE(out.find("[DEBUG] 1번째 줄에서 정지 -> 'var a = 1;'"),  std::string::npos);
    EXPECT_NE(out.find("[DEBUG] 2번째 줄에서 정지 -> 'print a;'"),    std::string::npos);
}

// TC-DSHELL-04: runSource — 줄 간 변수 공유 (DebugShell도 FileRunner와 동일)
TEST(DebugShellTest, TC_DSHELL_04_VariableSharingAcrossLines)
{
    NonBlockingController ctrl;
    DebugShell shell;

    testing::internal::CaptureStdout();
    shell.runSource({"var a = 10;", "print a;"}, &ctrl);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// TC-DSHELL-05: runSource — 빈 줄은 건너뜀 (정지 및 실행 대상 제외)
TEST(DebugShellTest, TC_DSHELL_05_EmptyLinesSkipped)
{
    RecordingController ctrl;
    DebugShell shell;

    testing::internal::CaptureStdout();
    shell.runSource({"var a = 1;", "", "print a;"}, &ctrl);
    testing::internal::GetCapturedStdout();

    // 줄 1, 줄 3에서만 정지 — 빈 줄 2는 건너뜀
    ASSERT_EQ(ctrl.stoppedAtLines.size(), 2u);
    EXPECT_EQ(ctrl.stoppedAtLines[0], 1);
    EXPECT_EQ(ctrl.stoppedAtLines[1], 3);
}

// TC-DSHELL-06: run() — "[DEBUG] 소스코드 로딩: <filepath>" 출력
// 빈 파일로 테스트하여 runSource 내 stdin 블로킹 없음
TEST(DebugShellTest, TC_DSHELL_06_LoadingMessage)
{
    const std::string tmpPath = "test_debug_loading.tmp";
    { std::ofstream f(tmpPath); }  // 빈 파일 생성

    DebugShell shell;
    testing::internal::CaptureStdout();
    shell.run(tmpPath);
    std::string out = testing::internal::GetCapturedStdout();

    std::remove(tmpPath.c_str());

    EXPECT_NE(out.find("[DEBUG] 소스코드 로딩:"), std::string::npos);
    EXPECT_NE(out.find(tmpPath),                  std::string::npos);
}
