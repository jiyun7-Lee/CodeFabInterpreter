#include <gtest/gtest.h>
#include "../DebugController.h"
#include "../Environment.h"

// TC-WATCH-01: watch var → 감시 등록 후 값 출력 확인
TEST(WatchTest, TC_WATCH_01_AddWatch)
{
    WatchManager mgr;
    mgr.add("x");

    Environment env;
    env.define("x", 10.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("x"), std::string::npos);
    EXPECT_NE(out.find("10"), std::string::npos);
}

// TC-WATCH-02: unwatch var → 감시 해제 후 출력 안 됨
TEST(WatchTest, TC_WATCH_02_RemoveWatch)
{
    WatchManager mgr;
    mgr.add("x");
    mgr.remove("x");

    Environment env;
    env.define("x", 10.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("감시 중인 변수 없음"), std::string::npos);
}

// TC-WATCH-03: watches → 감시 중인 여러 변수 값 출력
TEST(WatchTest, TC_WATCH_03_PrintMultipleWatches)
{
    WatchManager mgr;
    mgr.add("a");
    mgr.add("b");

    Environment env;
    env.define("a", 1.0);
    env.define("b", 2.0);

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("a"), std::string::npos);
    EXPECT_NE(out.find("b"), std::string::npos);
}

// TC-WATCH-04: 스코프 밖 변수는 "(스코프 밖)" 출력
TEST(WatchTest, TC_WATCH_04_OutOfScopeVariable)
{
    WatchManager mgr;
    mgr.add("unknown");

    Environment env;

    testing::internal::CaptureStdout();
    mgr.printWatches(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("스코프 밖"), std::string::npos);
}

// TC-WATCH-05: inspect → 현재 스코프 전체 변수 출력
TEST(WatchTest, TC_WATCH_05_Inspect)
{
    WatchManager mgr;

    Environment env;
    env.define("x", 5.0);
    env.define("y", 10.0);

    testing::internal::CaptureStdout();
    mgr.printInspect(&env);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("x"), std::string::npos);
    EXPECT_NE(out.find("y"), std::string::npos);
}
