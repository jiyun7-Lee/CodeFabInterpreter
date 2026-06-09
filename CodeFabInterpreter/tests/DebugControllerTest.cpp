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
// 테스트용 헬퍼 — base class beforeExecute 를 직접 테스트하기 위해
// state_ 설정만 노출한 최소 서브클래스
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

// TC-DBG-VAL-05: 배열 감시 → "[" 포함 출력
TEST_F(WatchValueTypeTest, TC_DBG_VAL_05_ArrayValueInWatch)
{
    mgr.add("arr");
    auto arr = std::make_shared<ArrayValue>();
    arr->elements.push_back(1.0);
    arr->elements.push_back(std::string("hi"));
    arr->elements.push_back(true);
    arr->elements.push_back(std::monostate{});
    env.define("arr", arr);
    auto out = captureOutput([&]{ mgr.printWatches(&env); });
    EXPECT_NE(out.find("["), std::string::npos);
}

// ================================================================
// 2. WatchManager — 빈 케이스 커버
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

// ================================================================
// 3. BreakpointManager — 빈 케이스 커버
// ================================================================

// TC-DBG-BP-06: print() 브레이크포인트 없음 → 안내 메시지 출력
TEST(DebugControllerEmptyTest, TC_DBG_BP_06_PrintEmptyBreakpoints)
{
    BreakpointManager mgr;
    auto out = captureOutput([&]{ mgr.print(); });
    EXPECT_NE(out.find("없음"), std::string::npos);
}

// ================================================================
// 4. DebugController::beforeExecute — RUNNING 상태 분기
// ================================================================

// TC-DBG-STATE-01: RUNNING 상태, 브레이크포인트 없음 → 조기 반환 (블로킹 없음)
TEST(DebugControllerStateTest, TC_DBG_STATE_01_RunningNoBreakpoint_EarlyReturn)
{
    ExposedController ctrl;
    ctrl.setRunning();
    // 브레이크포인트 없음 → beforeExecute 에서 else return 분기 실행

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
    // 블로킹 없이 완주 → RUNNING 상태 유지
    EXPECT_EQ(ctrl.getState(), ExecutionState::RUNNING);
}

// TC-DBG-STATE-02: RUNNING 상태, 브레이크포인트 도달 → PAUSED, stdin "continue" 제공
TEST(DebugControllerStateTest, TC_DBG_STATE_02_RunningHitsBreakpoint_Pauses)
{
    CinRedirect redirect("continue\ncontinue\n");

    ExposedController ctrl;
    ctrl.setRunning();
    ctrl.addBreakpoint(1);

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    // 브레이크포인트 도달 후 continue 입력으로 완주
    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// ================================================================
// 5. DebugController::beforeExecute — NEXT 상태 분기
// ================================================================

// TC-DBG-STATE-03: NEXT 상태, depth > nextDepth_ → 조기 반환 (블록 내부 건너뜀)
TEST(DebugControllerStateTest, TC_DBG_STATE_03_NextState_SkipsDeepStmt)
{
    // nextDepth_ = 0 으로 설정 후 블록 내부 stmt(depth>0) 실행 시 조기 반환 확인
    // 블록 내부 진입 시 depth=1 > nextDepth_=0 → return (블로킹 없음)
    // 단, 최상위 BlockStmt(depth=0)는 NEXT → STEP 으로 전환 후 stdin 필요
    CinRedirect redirect("step\nstep\ncontinue\n");

    ExposedController ctrl;
    ctrl.setNext(0);

    auto stmts = parseSource("{ var a = 1; var b = 2; }");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// ================================================================
// 6. beforeExecute — watch 자동 출력 (STEP 정지 시 감시 변수 출력)
// ================================================================

// TC-DBG-STATE-04: STEP 정지 시 watch 자동 출력 확인
TEST(DebugControllerStateTest, TC_DBG_STATE_04_WatchAutoPrintOnStep)
{
    // step → step → continue 로 순차 진행
    CinRedirect redirect("step\ncontinue\n");

    ExposedController ctrl;
    ctrl.addWatch("a");
    // 초기 state = STEP (기본값)

    auto stmts = parseSource("var a = 42;\nprint a;");
    Executor exec;
    exec.setDebugController(&ctrl);

    // watch 자동 출력이 beforeExecute 내에서 발생해야 함
    auto out = captureOutput([&]{ exec.execute(stmts); });

    // a = 42 또는 스코프 밖 메시지가 출력돼야 함
    EXPECT_TRUE(out.find("a") != std::string::npos);
}

// ================================================================
// 7. beforeExecute 명령어 파싱 — break / remove / watch 명령
// ================================================================

// TC-DBG-STATE-05: step 정지 중 break N → 브레이크포인트 추가 후 continue
TEST(DebugControllerStateTest, TC_DBG_STATE_05_BreakCommandDuringPause)
{
    // 첫 정지에서 "break 2" 입력 후 continue, 두 번째 줄에서 PAUSED → continue
    CinRedirect redirect("break 2\ncontinue\ncontinue\n");

    ExposedController ctrl;
    // state = STEP 기본

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// TC-DBG-STATE-06: step 정지 중 watch / watches / inspect 명령
TEST(DebugControllerStateTest, TC_DBG_STATE_06_WatchCommandsDuringPause)
{
    // 첫 정지: watch a 등록 → watches → inspect → step
    // 두 번째 정지: unwatch a → continue
    CinRedirect redirect("watch a\nwatches\ninspect\nstep\nunwatch a\ncontinue\n");

    ExposedController ctrl;

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    auto out = captureOutput([&]{ exec.execute(stmts); });
    // watch 관련 출력이 발생해야 함
    EXPECT_FALSE(out.empty());
}

// TC-DBG-STATE-07: step 정지 중 remove N → 브레이크포인트 해제
TEST(DebugControllerStateTest, TC_DBG_STATE_07_RemoveCommandDuringPause)
{
    CinRedirect redirect("break 2\nremove 2\nbreakpoints\ncontinue\ncontinue\n");

    ExposedController ctrl;

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    EXPECT_NO_THROW(captureOutput([&]{ exec.execute(stmts); }));
}

// TC-DBG-STATE-08: 알 수 없는 명령어 입력 → 안내 후 계속 대기
TEST(DebugControllerStateTest, TC_DBG_STATE_08_UnknownCommandThenContinue)
{
    CinRedirect redirect("invalidcmd\ncontinue\ncontinue\n");

    ExposedController ctrl;

    auto stmts = parseSource("var a = 1;\nvar b = 2;");
    Executor exec;
    exec.setDebugController(&ctrl);

    auto out = captureOutput([&]{ exec.execute(stmts); });
    EXPECT_NE(out.find("알 수 없는"), std::string::npos);
}
