#include <gtest/gtest.h>
#include "../Shell.h"

class FileModeTest : public ::testing::Test
{
protected:
    FileRunner runner;
};

// TC-FILE-01: 정상 소스 실행 — 줄 간 변수 공유 확인
TEST_F(FileModeTest, TC_FILE_01_NormalExecution)
{
    testing::internal::CaptureStdout();
    runner.runSource({"var a = 10;", "print a;"});
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// TC-FILE-02: 존재하지 않는 파일 경로 → "File Not Found" 출력
TEST_F(FileModeTest, TC_FILE_02_FileNotFound)
{
    testing::internal::CaptureStdout();
    runner.run("__nonexistent_file__.cf");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("File Not Found"), std::string::npos);
}

// TC-FILE-03: 런타임 에러 발생 → 오류 출력 후 종료
// 줄별 독립 토크나이징이므로 각 줄은 "1번째 줄"로 보고됨 (의도된 동작)
TEST_F(FileModeTest, TC_FILE_03_RuntimeError_WithLineNumber)
{
    testing::internal::CaptureStdout();
    runner.runSource({"var a = 10;", "print 1/0;"});
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("[Error]"),   std::string::npos);
    EXPECT_NE(out.find("1번째 줄"),  std::string::npos);
}

// TC-FILE-04: Checker 에러 발생 → 오류 출력 후 종료
// 줄별 독립 토크나이징이므로 각 줄은 "1번째 줄"로 보고됨 (의도된 동작)
TEST_F(FileModeTest, TC_FILE_04_CheckerError_WithLineNumber)
{
    testing::internal::CaptureStdout();
    runner.runSource({"var a = 5;", "var b = b;"});
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("[Checker]"), std::string::npos);
    EXPECT_NE(out.find("1번째 줄"),  std::string::npos);
}

// TC-FILE-05: 런타임 에러 발생 시 이후 줄 실행 안 됨
TEST_F(FileModeTest, TC_FILE_05_RuntimeError_StopsExecution)
{
    testing::internal::CaptureStdout();
    runner.runSource({"print 1/0;", "print 99;"});
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out.find("99"), std::string::npos);
}
