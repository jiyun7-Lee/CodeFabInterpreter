#include <gtest/gtest.h>
#include "../DebugController.h"
#include "../Environment.h"

class WatchTest : public ::testing::Test
{
protected:
    WatchManager mgr;
    Environment  env;
};

// TC-WATCH-01: watch var → 감시 등록 후 값 출력 확인
TEST_F(WatchTest, TC_WATCH_01_AddWatch)
{
    mgr.add("x");
    env.define("x", 10.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("x"),  std::string::npos);
    EXPECT_NE(out.find("10"), std::string::npos);
}

// TC-WATCH-02: unwatch var → 감시 해제 후 출력 안 됨
TEST_F(WatchTest, TC_WATCH_02_RemoveWatch)
{
    mgr.add("x");
    mgr.remove("x");
    env.define("x", 10.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("감시 중인 변수 없음"), std::string::npos);
}

// TC-WATCH-03: watches → 감시 중인 여러 변수 값 출력
TEST_F(WatchTest, TC_WATCH_03_PrintMultipleWatches)
{
    mgr.add("a");
    mgr.add("b");
    env.define("a", 1.0);
    env.define("b", 2.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("a"), std::string::npos);
    EXPECT_NE(out.find("b"), std::string::npos);
}

// TC-WATCH-04: 스코프 밖 변수는 "(스코프 밖)" 출력
TEST_F(WatchTest, TC_WATCH_04_OutOfScopeVariable)
{
    mgr.add("unknown");

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("스코프 밖"), std::string::npos);
}

// TC-WATCH-05: inspect → 현재 스코프 전체 변수 출력
TEST_F(WatchTest, TC_WATCH_05_Inspect)
{
    env.define("x", 5.0);
    env.define("y", 10.0);

    testing::internal::CaptureStdout();
    mgr.printInspect(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("x"), std::string::npos);
    EXPECT_NE(out.find("y"), std::string::npos);
}

// TC-WATCH-06: watch 변수명 앞뒤 공백 → trim 후 정상 감시
TEST_F(WatchTest, TC_WATCH_06_AddWatch_TrimsWhitespace)
{
    mgr.add("  x  ");
    env.define("x", 42.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("x"),        std::string::npos);
    EXPECT_EQ(out.find("스코프 밖"), std::string::npos);
}

// TC-WATCH-07: inspect → 부모 스코프 변수는 노출되지 않음
TEST_F(WatchTest, TC_WATCH_07_Inspect_DoesNotExposeParentScope)
{
    Environment parent;
    parent.define("g", 100.0);

    Environment child;
    child.parent = &parent;
    child.define("x", 5.0);

    testing::internal::CaptureStdout();
    mgr.printInspect(&child);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("x"), std::string::npos);
    EXPECT_EQ(out.find("g"), std::string::npos);
}
