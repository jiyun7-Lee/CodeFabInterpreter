#include <gtest/gtest.h>
#include "../Shell.h"

// TC-FILE-01: 정상 소스 실행 — 변수 선언 후 출력
TEST(FileModeTest, TC_FILE_01_NormalExecution)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource("var a = 10;\nprint a;");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// TC-FILE-02: 존재하지 않는 파일 경로 → "File Not Found" 출력
TEST(FileModeTest, TC_FILE_02_FileNotFound)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.run("__nonexistent_file__.cf");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("File Not Found"), std::string::npos);
}

// TC-FILE-03: 런타임 에러 발생 → 줄번호 포함 오류 출력 후 종료
// "var a = 10;\nprint 1/0;" → 2번째 줄 Division by zero
TEST(FileModeTest, TC_FILE_03_RuntimeError_WithLineNumber)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource("var a = 10;\nprint 1/0;");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("[Error]"), std::string::npos);
    EXPECT_NE(out.find("2번째 줄"), std::string::npos);
}

// TC-FILE-04: Checker 에러 발생 → 줄번호 포함 오류 출력 후 종료
// "var a = 5;\nvar b = b;" → 2번째 줄 자기 참조 에러
TEST(FileModeTest, TC_FILE_04_CheckerError_WithLineNumber)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource("var a = 5;\nvar b = b;");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("[Checker]"), std::string::npos);
    EXPECT_NE(out.find("2번째 줄"), std::string::npos);
}

// TC-FILE-05: 런타임 에러 발생 시 이후 코드 실행 안 됨
// "print 1/0;\nprint 99;" → 99는 출력되지 않아야 함
TEST(FileModeTest, TC_FILE_05_RuntimeError_StopsExecution)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource("print 1/0;\nprint 99;");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out.find("99"), std::string::npos);
}
