// DebugControllerTest.cpp — DebugController 브랜치 커버리지 향상 TC
#include <gtest/gtest.h>
#include <sstream>
#include "../DebugController.h"
#include "../Environment.h"
#include "../Executor.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Value.h"
#include "TestHelpers.h"

// ================================================================
// 테스트용 헬퍼 — state_ 노출 최소 서브클래스
// ================================================================
class ExposedController : public DebugController
{
public:
    void setRunning()            { state_ = ExecutionState::RUNNING; }
    void setNext(int depth = 0)  { nextDepth_ = depth; state_ = ExecutionState::NEXT; }
    ExecutionState getState() const { return state_; }
};

// stdin 리다이렉트 RAII 가드
struct CinRedirect
{
    std::istringstream  iss;
    std::streambuf*     orig;
    explicit CinRedirect(const std::string& input)
        : iss(input), orig(std::cin.rdbuf(iss.rdbuf())) {}
    ~CinRedirect() { std::cin.rdbuf(orig); }
};

static std::vector<std::unique_ptr<Stmt>> parseSource(const std::string& src)
{
    Tokenizer tz; Parser p;
    return p.parse(tz.tokenize(src));
}

// ================================================================
// 1. WatchManager — valueToString 타입별 커버
// ================================================================

class WatchValueTypeTest : public ::testing::Test
{
protected:
    WatchManager mgr;
    Environment  env;
};

// TC-DBG-VAL-01: string 값 감시 → 따옴표 포함 출력
TEST_F(WatchValueTypeTest, TC_DBG_VAL_01_StringValueInWatch)
{
    mgr.add("s");
    env.define("s", std::string("hello"));
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("hello"), std::string::npos);
}

// TC-DBG-VAL-02: bool true 감시 → "true" 출력
TEST_F(WatchValueTypeTest, TC_DBG_VAL_02_BoolTrueInWatch)
{
    mgr.add("b");
    env.define("b", true);
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("true"), std::string::npos);
}

// TC-DBG-VAL-03: bool false 감시 → "false" 출력
TEST_F(WatchValueTypeTest, TC_DBG_VAL_03_BoolFalseInWatch)
{
    mgr.add("b");
    env.define("b", false);
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("false"), std::string::npos);
}

// TC-DBG-VAL-04: null(monostate) 감시 → "null" 출력
TEST_F(WatchValueTypeTest, TC_DBG_VAL_04_NullValueInWatch)
{
    mgr.add("n");
    env.define("n", std::monostate{});
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("null"), std::string::npos);
}

// TC-DBG-VAL-05: 배열 감시 → 실제 원소 표시 (PR #57 수정 반영)
// valueToString 내 배열 원소 출력 분기 (double/string/bool/null) 커버
TEST_F(WatchValueTypeTest, TC_DBG_VAL_05_ArrayValueInWatch)
{
    mgr.add("arr");
    auto arr = std::make_shared<ArrayValue>();
    arr->elements.push_back(1.0);               // double 원소
    arr->elements.push_back(std::string("hi")); // string 원소
    arr->elements.push_back(true);              // bool 원소
    arr->elements.push_back(std::monostate{}); // null 원소
    env.define("arr", arr);
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("["),     std::string::npos);
    EXPECT_NE(out.find("hi"),   std::string::npos);
    EXPECT_NE(out.find("true"), std::string::npos);
    EXPECT_NE(out.find("null"), std::string::npos);
}

// ================================================================
// 2. WatchManager / BreakpointManager — 빈 케이스 커버
// ================================================================

// TC-DBG-VAL-06: printInspect(nullptr) → "변수 없음" 출력
TEST(DebugControllerEmptyTest, TC_DBG_VAL_06_InspectNullEnv)
{
    WatchManager mgr;
    auto out = captureOutput([&]{ mgr.printInspect(nullptr); });
    EXPECT_NE(out.find("변수 없음"), std::string::npos);
}

// TC-DBG-VAL-07: printInspect 빈 환경 → "변수 없음" 출력
TEST(DebugControllerEmptyTest, TC_DBG_VAL_07_InspectEmptyEnv)
{
    WatchManager mgr;
    Environment  env;
    auto out = captureOutput([&]{ mgr.printInspect(&env); });
    EXPECT_NE(out.find("변수 없음"), std::string::npos);
}

// TC-DBG-BP-06: print() 브레이크포인트 없음 → 안내 메시지 출력
TEST(DebugControllerEmptyTest, TC_DBG_BP_06_PrintEmptyBreakpoints)
{
    BreakpointManager mgr;
    auto out = captureOutput([&]{ mgr.print(); });
    EXPECT_NE(out.find("없음"), std::string::npos);
}

// ================================================================
// 3. beforeExecute — RUNNING 상태 분기
// ================================================================

// TC-DBG-STATE-01: RUNNING + 브레이크포인트 없음 → else return 조기 반환 (블로킹 없음)
TEST(DebugControllerStateTest, TC_DBG_STATE_01_RunningNoBreakpoint_EarlyReturn)
{
    ExposedController ctrl;
    ctrl.setRunning();

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
    EXPECT_EQ(ctrl.getState(), ExecutionState::RUNNING);
}

// TC-DBG-STATE-02: RUNNING + 브레이크포인트 도달 → PAUSED, stdin "continue" 제공
TEST(DebugControllerStateTest, TC_DBG_STATE_02_RunningHitsBreakpoint_Pauses)
{
    CinRedirect redirect("continue\ncontinue\n");

    ExposedController ctrl;
    ctrl.setRunning();
    ctrl.addBreakpoint(1);

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// ================================================================
// 4. beforeExecute — sourceLines_ 분기 커버 (PR #57 신규)
// ================================================================

// TC-DBG-STATE-03: setSourceLines 설정 시 정지 메시지에 해당 줄 텍스트 표시
TEST(DebugControllerStateTest, TC_DBG_STATE_03_SourceLines_UsedInDisplay)
{
    CinRedirect redirect("continue\ncontinue\n");

    ExposedController ctrl;
    ctrl.setSourceLines({"var a = 1;", "var b = 2;"});
    // 초기 상태 STEP → 첫 정지에서 sourceLines_[0] 표시

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    auto out = captureOutput([&]{ exec.execute(stmts); });
    EXPECT_NE(out.find("var a = 1;"), std::string::npos);
}

// TC-DBG-STATE-04: setSourceLines 미설정 → currentSrcLine_ 폴백
TEST(DebugControllerStateTest, TC_DBG_STATE_04_SourceLines_FallbackToSrcLine)
{
    CinRedirect redirect("continue\ncontinue\n");

    ExposedController ctrl;
    // sourceLines_ 설정 안 함 → currentSrcLine_ 사용 (빈 문자열)

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    // 빈 currentSrcLine_ 으로도 정상 실행돼야 함
    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// ================================================================
// 5. beforeExecute — NEXT 상태 분기
// ================================================================

// TC-DBG-STATE-05: NEXT 상태 + depth > nextDepth_ → 조기 반환 (블록 내부 건너뜀)
TEST(DebugControllerStateTest, TC_DBG_STATE_05_NextState_SkipsDeepStmt)
{
    CinRedirect redirect("step\nstep\ncontinue\n");

    ExposedController ctrl;
    ctrl.setNext(0);

    auto stmts = parseSource("{ var a = 1; var b = 2; }");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// ================================================================
// 6. beforeExecute — watch 자동 출력 & 명령어 파싱
// ================================================================

// TC-DBG-STATE-06: STEP 정지 시 watch 자동 출력
TEST(DebugControllerStateTest, TC_DBG_STATE_06_WatchAutoPrintOnStep)
{
    CinRedirect redirect("step\ncontinue\n");

    ExposedController ctrl;
    ctrl.addWatch("a");

    auto stmts = parseSource("var a = 42;\nprint a;");
    Executor exec;
    exec.setDebugController(&ctrl);

    auto out = captureOutput([&]{ exec.execute(stmts); });
    EXPECT_TRUE(out.find("a") != std::string::npos);
}

// TC-DBG-STATE-07: 정지 중 break N / remove N / breakpoints 명령
TEST(DebugControllerStateTest, TC_DBG_STATE_07_BreakRemoveCommands)
{
    CinRedirect redirect("break 2\nremove 2\nbreakpoints\ncontinue\ncontinue\n");

    ExposedController ctrl;

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// TC-DBG-STATE-08: 정지 중 watch / watches / inspect / unwatch 명령
TEST(DebugControllerStateTest, TC_DBG_STATE_08_WatchInspectCommands)
{
    CinRedirect redirect("watch a\nwatches\ninspect\nstep\nunwatch a\ncontinue\n");

    ExposedController ctrl;

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    auto out = captureOutput([&]{ exec.execute(stmts); });
    EXPECT_FALSE(out.empty());
}

// TC-DBG-STATE-09: 알 수 없는 명령어 입력 → 안내 메시지 출력
TEST(DebugControllerStateTest, TC_DBG_STATE_09_UnknownCommandMessage)
{
    CinRedirect redirect("invalidcmd\ncontinue\ncontinue\n");

    ExposedController ctrl;

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    auto out = captureOutput([&]{ exec.execute(stmts); });
    EXPECT_NE(out.find("알 수 없는"), std::string::npos);
}
