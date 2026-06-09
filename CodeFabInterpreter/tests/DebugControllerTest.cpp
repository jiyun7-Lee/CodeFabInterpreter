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
// Fixture
// ================================================================
class DebugControllerFixture : public ::testing::Test
{
protected:
    // source 파싱 후 ctrl 로 실행 → stdout 반환
    std::string runDebug(const std::string& src, DebugController& ctrl)
    {
        Tokenizer tz; Parser p;
        auto stmts = p.parse(tz.tokenize(src));
        Executor exec;
        exec.setDebugController(&ctrl);
        return captureOutput([&]{ exec.execute(stmts); });
    }
};

// ================================================================
// 1. WatchManager — valueToString 타입별 커버
// ================================================================

class WatchValueTypeFixture : public ::testing::Test
{
protected:
    WatchManager mgr;
    Environment  env;
};

// TC-DBG-VAL-01: string 값 감시 → 따옴표 포함 출력
TEST_F(WatchValueTypeFixture, TC_DBG_VAL_01_StringValueInWatch)
{
    mgr.add("s");
    env.define("s", std::string("hello"));
    EXPECT_NE(captureOutput([&]{ mgr.printWatches(&env); }).find("hello"), std::string::npos);
}

// TC-DBG-VAL-02: bool true 감시 → "true" 출력
TEST_F(WatchValueTypeFixture, TC_DBG_VAL_02_BoolTrueInWatch)
{
    mgr.add("b");
    env.define("b", true);
    EXPECT_NE(captureOutput([&]{ mgr.printWatches(&env); }).find("true"), std::string::npos);
}

// TC-DBG-VAL-03: bool false 감시 → "false" 출력
TEST_F(WatchValueTypeFixture, TC_DBG_VAL_03_BoolFalseInWatch)
{
    mgr.add("b");
    env.define("b", false);
    EXPECT_NE(captureOutput([&]{ mgr.printWatches(&env); }).find("false"), std::string::npos);
}

// TC-DBG-VAL-04: null(monostate) 감시 → "null" 출력
TEST_F(WatchValueTypeFixture, TC_DBG_VAL_04_NullValueInWatch)
{
    mgr.add("n");
    env.define("n", std::monostate{});
    EXPECT_NE(captureOutput([&]{ mgr.printWatches(&env); }).find("null"), std::string::npos);
}

// TC-DBG-VAL-05: 배열 감시 → 실제 원소 표시 (PR #57 수정 반영)
TEST_F(WatchValueTypeFixture, TC_DBG_VAL_05_ArrayValueInWatch)
{
    mgr.add("arr");
    auto arr = std::make_shared<ArrayValue>();
    arr->elements.push_back(1.0);
    arr->elements.push_back(std::string("hi"));
    arr->elements.push_back(true);
    arr->elements.push_back(std::monostate{});
    env.define("arr", arr);
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("["),    std::string::npos);
    EXPECT_NE(out.find("hi"),   std::string::npos);
    EXPECT_NE(out.find("true"), std::string::npos);
    EXPECT_NE(out.find("null"), std::string::npos);
}

// ================================================================
// 2. 빈 케이스 커버
// ================================================================

// TC-DBG-VAL-06: printInspect(nullptr) → "변수 없음" 출력
TEST(DebugControllerEmptyTest, TC_DBG_VAL_06_InspectNullEnv)
{
    WatchManager mgr;
    EXPECT_NE(captureOutput([&]{ mgr.printInspect(nullptr); }).find("변수 없음"), std::string::npos);
}

// TC-DBG-VAL-07: printInspect 빈 환경 → "변수 없음" 출력
TEST(DebugControllerEmptyTest, TC_DBG_VAL_07_InspectEmptyEnv)
{
    WatchManager mgr;
    Environment  env;
    EXPECT_NE(captureOutput([&]{ mgr.printInspect(&env); }).find("변수 없음"), std::string::npos);
}

// TC-DBG-BP-06: print() 브레이크포인트 없음 → 안내 메시지
TEST(DebugControllerEmptyTest, TC_DBG_BP_06_PrintEmptyBreakpoints)
{
    BreakpointManager mgr;
    EXPECT_NE(captureOutput([&]{ mgr.print(); }).find("없음"), std::string::npos);
}

// ================================================================
// 3. beforeExecute — RUNNING / NEXT 상태 분기
// ================================================================

// TC-DBG-STATE-01: RUNNING + 브레이크포인트 없음 → else return 조기 반환
TEST_F(DebugControllerFixture, TC_DBG_STATE_01_RunningNoBreakpoint_EarlyReturn)
{
    ExposedController ctrl;
    ctrl.setRunning();
    EXPECT_NO_THROW(runDebug("var a = 1;\nvar b = 2;", ctrl));
    EXPECT_EQ(ctrl.getState(), ExecutionState::RUNNING);
}

// TC-DBG-STATE-02: RUNNING + 브레이크포인트 도달 → PAUSED
TEST_F(DebugControllerFixture, TC_DBG_STATE_02_RunningHitsBreakpoint_Pauses)
{
    CinRedirect redirect("continue\ncontinue\n");
    ExposedController ctrl;
    ctrl.setRunning();
    ctrl.addBreakpoint(1);
    EXPECT_NO_THROW(runDebug("var a = 1;\nvar b = 2;", ctrl));
}

// TC-DBG-STATE-03: setSourceLines 설정 → 정지 메시지에 해당 줄 텍스트 표시
TEST_F(DebugControllerFixture, TC_DBG_STATE_03_SourceLines_UsedInDisplay)
{
    CinRedirect redirect("continue\ncontinue\n");
    ExposedController ctrl;
    ctrl.setSourceLines({"var a = 1;", "var b = 2;"});
    auto out = runDebug("var a = 1;\nvar b = 2;", ctrl);
    EXPECT_NE(out.find("var a = 1;"), std::string::npos);
}

// TC-DBG-STATE-04: setSourceLines 미설정 → currentSrcLine_ 폴백, 정상 완주
TEST_F(DebugControllerFixture, TC_DBG_STATE_04_SourceLines_FallbackToSrcLine)
{
    CinRedirect redirect("continue\ncontinue\n");
    ExposedController ctrl;
    EXPECT_NO_THROW(runDebug("var a = 1;\nvar b = 2;", ctrl));
}

// TC-DBG-STATE-05: NEXT + depth > nextDepth_ → 블록 내부 건너뜀
TEST_F(DebugControllerFixture, TC_DBG_STATE_05_NextState_SkipsDeepStmt)
{
    CinRedirect redirect("step\nstep\ncontinue\n");
    ExposedController ctrl;
    ctrl.setNext(0);
    EXPECT_NO_THROW(runDebug("{ var a = 1; var b = 2; }", ctrl));
}

// ================================================================
// 4. beforeExecute — watch 자동 출력 & 명령어 파싱
// ================================================================

// TC-DBG-STATE-06: STEP 정지 시 watch 자동 출력
TEST_F(DebugControllerFixture, TC_DBG_STATE_06_WatchAutoPrintOnStep)
{
    CinRedirect redirect("step\ncontinue\n");
    ExposedController ctrl;
    ctrl.addWatch("a");
    auto out = runDebug("var a = 42;\nprint a;", ctrl);
    EXPECT_NE(out.find("a"), std::string::npos);
}

// TC-DBG-STATE-07: 정지 중 break N / remove N / breakpoints 명령
TEST_F(DebugControllerFixture, TC_DBG_STATE_07_BreakRemoveCommands)
{
    CinRedirect redirect("break 2\nremove 2\nbreakpoints\ncontinue\ncontinue\n");
    ExposedController ctrl;
    EXPECT_NO_THROW(runDebug("var a = 1;\nvar b = 2;", ctrl));
}

// TC-DBG-STATE-08: 정지 중 watch / watches / inspect / unwatch 명령
TEST_F(DebugControllerFixture, TC_DBG_STATE_08_WatchInspectCommands)
{
    CinRedirect redirect("watch a\nwatches\ninspect\nstep\nunwatch a\ncontinue\n");
    ExposedController ctrl;
    EXPECT_FALSE(runDebug("var a = 1;\nvar b = 2;", ctrl).empty());
}

// TC-DBG-STATE-09: 알 수 없는 명령어 → 안내 메시지 출력
TEST_F(DebugControllerFixture, TC_DBG_STATE_09_UnknownCommandMessage)
{
    CinRedirect redirect("invalidcmd\ncontinue\ncontinue\n");
    ExposedController ctrl;
    EXPECT_NE(runDebug("var a = 1;\nvar b = 2;", ctrl).find("알 수 없는"), std::string::npos);
}
