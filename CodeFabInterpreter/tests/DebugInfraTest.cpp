#include <gtest/gtest.h>
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include "../DebugController.h"

// TC-DBG-INFRA-01: Parser가 Stmt에 line 번호를 올바르게 설정하는지 확인
TEST(DebugInfraTest, TC_DBG_INFRA_01_StmtHasLineNumber)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("var a = 1;\nprint a;");

    Parser parser;
    auto stmts = parser.parse(tokens);

    ASSERT_EQ(stmts.size(), 2u);
    EXPECT_EQ(stmts[0]->line, 1);
    EXPECT_EQ(stmts[1]->line, 2);
}

// TC-DBG-INFRA-02: executeStatement 호출 시 hook이 호출됨
class MockDebugController : public DebugController
{
public:
    int callCount = 0;
    void beforeExecute(Stmt* /*stmt*/, Environment* /*env*/, int /*depth*/) override { callCount++; }
};

TEST(DebugInfraTest, TC_DBG_INFRA_02_HookCalledForEachStmt)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("var a = 1;\nprint a;");

    Parser parser;
    auto stmts = parser.parse(tokens);

    MockDebugController mock;
    Executor executor;
    executor.setDebugController(&mock);
    executor.execute(stmts);

    // 최상위 Stmt 2개 → hook 2회 호출
    EXPECT_EQ(mock.callCount, 2);
}

// TC-DBG-INFRA-03: hook 미등록 시 기존 동작과 동일
TEST(DebugInfraTest, TC_DBG_INFRA_03_NoHookNormalExecution)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("print 1+2;");

    Parser parser;
    auto stmts = parser.parse(tokens);

    Executor executor; // debug_ = nullptr

    testing::internal::CaptureStdout();
    EXPECT_NO_THROW(executor.execute(stmts));
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "3\n");
}
