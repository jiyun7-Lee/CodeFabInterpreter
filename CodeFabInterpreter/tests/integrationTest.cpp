#include <gtest/gtest.h>
#include <sstream>
#include "../Shell.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include "../DebugController.h"
#include "../Stmt.h"

// ================================================================
// Fixture
// ================================================================
class IntegrationFixture : public ::testing::Test
{
protected:
    Shell shell;

    // Shell::runLine() 실행 후 stdout 반환
    std::string run(const std::string& source)
    {
        testing::internal::CaptureStdout();
        shell.runLine(source);
        return testing::internal::GetCapturedStdout();
    }

    // 출력에 특정 문자열이 포함되어 있는지 검증
    void runExpectContains(const std::string& source, const std::string& expected)
    {
        EXPECT_NE(run(source).find(expected), std::string::npos);
    }

    // FileRunner::runSource() 실행 후 stdout 반환
    std::string runFile(const std::vector<std::string>& lines)
    {
        FileRunner runner;
        testing::internal::CaptureStdout();
        runner.runSource(lines);
        return testing::internal::GetCapturedStdout();
    }
};

// ================================================================
// 1. Shell 통합 테스트 — 정상 실행 (IT_01~15)
// ================================================================

TEST_F(IntegrationFixture, IT_01_ArithmeticPrecedence)
{
    EXPECT_EQ(run(
        "print 1 + 2 * 3;\n"    // 7
        "print (1 + 2) * 3;\n"  // 9
        "print 10 - 4 - 3;\n"   // 3
        "print 8 / 2 / 2;\n"    // 2
        "print -3 + 2;"          // -1
    ), "7\n9\n3\n2\n-1\n");
}

TEST_F(IntegrationFixture, IT_02_ComparisonOperators)
{
    EXPECT_EQ(run(
        "print 1 < 2;\n"  // true
        "print 3 > 5;"    // false
    ), "true\nfalse\n");
}

TEST_F(IntegrationFixture, IT_03_StringConcatenation)
{
    EXPECT_EQ(run("print \"Hello, \" + \"CodeFab!\";"), "Hello, CodeFab!\n");
}

TEST_F(IntegrationFixture, IT_04_NumberFormat)
{
    EXPECT_EQ(run(
        "print 5;\n"     // 5
        "print 5.0;\n"   // 5
        "print 3.14;"    // 3.14
    ), "5\n5\n3.14\n");
}

TEST_F(IntegrationFixture, IT_05_BooleanLiterals)
{
    EXPECT_EQ(run(
        "print true;\n"   // true
        "print false;"    // false
    ), "true\nfalse\n");
}

TEST_F(IntegrationFixture, IT_06_VarDeclareAndReassign)
{
    EXPECT_EQ(run(
        "var a = 10;\n"
        "var b = 20;\n"
        "print a + b;\n"  // 30
        "a = a + 5;\n"
        "print a;"        // 15
    ), "30\n15\n");
}

TEST_F(IntegrationFixture, IT_07_BlockScopeAndShadowing)
{
    EXPECT_EQ(run(
        "var x = \"global\";\n"
        "{\n"
        "  var x = \"inner\";\n"
        "  print x;\n"           // inner
        "}\n"
        "print x;"               // global
    ), "inner\nglobal\n");
}

TEST_F(IntegrationFixture, IT_08_BlockModifiesOuterVar)
{
    EXPECT_EQ(run(
        "var count = 0;\n"
        "{\n"
        "  count = count + 1;\n"
        "}\n"
        "print count;"  // 1
    ), "1\n");
}

TEST_F(IntegrationFixture, IT_09_NestedScope)
{
    EXPECT_EQ(run(
        "var outer = \"A\";\n"
        "{\n"
        "  var inner = \"B\";\n"
        "  {\n"
        "    print outer + inner;\n"  // AB
        "  }\n"
        "}"
    ), "AB\n");
}

TEST_F(IntegrationFixture, IT_10_IfSimple)
{
    EXPECT_EQ(run("if (true) print \"bbq\";"), "bbq\n");
}

TEST_F(IntegrationFixture, IT_11_IfElse)
{
    EXPECT_EQ(run("if (false) print \"no\"; else print \"kfc\";"), "kfc\n");
}

TEST_F(IntegrationFixture, IT_12_DanglingElse)
{
    EXPECT_EQ(run(
        "if (true)\n"
        "  if (false) print \"kfc\";\n"
        "  else print \"bbq\";"  // bbq (가장 가까운 if 에 결합)
    ), "bbq\n");
}

TEST_F(IntegrationFixture, IT_13_ForLoop)
{
    EXPECT_EQ(run("for (var j = 0; j < 3; j = j + 1) { print j; }"), "0\n1\n2\n");
}

TEST_F(IntegrationFixture, IT_14_IfBlockNextLine)
{
    EXPECT_EQ(run(
        "if (true)\n"
        "{\n"
        "  print \"bbq\";\n"
        "}"
    ), "bbq\n");
}

TEST_F(IntegrationFixture, IT_15_ForBlockNextLine)
{
    EXPECT_EQ(run(
        "for (var i = 0; i < 3; i = i + 1)\n"
        "{\n"
        "  print i;\n"
        "}"
    ), "0\n1\n2\n");
}

// ================================================================
// 2. Shell 통합 테스트 — 에러 검출 (IT_E01~09)
// ================================================================

TEST_F(IntegrationFixture, IT_E01_MissingSemicolon)
{
    runExpectContains("print 1 + 2", "[Error]");
}

TEST_F(IntegrationFixture, IT_E02_MissingClosingParen)
{
    runExpectContains("print (1 + 2;", "[Error]");
}

TEST_F(IntegrationFixture, IT_E03_InvalidAssignmentTarget)
{
    runExpectContains("var a = 1;\nvar b = 2;\na + b = 3;", "[Error]");
}

TEST_F(IntegrationFixture, IT_E04_UnexpectedToken)
{
    runExpectContains("print * 5;", "[Error]");
}

TEST_F(IntegrationFixture, IT_E05_SelfInitReference)
{
    runExpectContains("{ var a = a; }", "[Checker]");
}

TEST_F(IntegrationFixture, IT_E06_DuplicateDeclaration)
{
    runExpectContains("{ var a = \"hi\"; var a = 3; }", "[Checker]");
}

TEST_F(IntegrationFixture, IT_E07_UndefinedVariable)
{
    runExpectContains("print notDefined;", "[Error]");
}

TEST_F(IntegrationFixture, IT_E08_TypeMismatchPlus)
{
    runExpectContains("print 1 + \"HI\";", "[Error]");
}

TEST_F(IntegrationFixture, IT_E09_UnaryMinusTypeMismatch)
{
    runExpectContains("print -\"FabCoding\";", "[Error]");
}

// ================================================================
// 3. FileRunner 통합 테스트 — 멀티라인 파일 모드 (IT_16~22)
// ================================================================

TEST_F(IntegrationFixture, IT_16_FileMode_ForLoopMultiline)
{
    EXPECT_EQ(runFile({
        "for (var a = 3; a < 5; a = a + 1)",
        "{",
        "  print a;",
        "}"
    }), "3\n4\n");
}

TEST_F(IntegrationFixture, IT_17_FileMode_IfBlockMultiline)
{
    EXPECT_EQ(runFile({
        "if (true)",
        "{",
        "  print \"bbq\";",
        "}"
    }), "bbq\n");
}

TEST_F(IntegrationFixture, IT_18_FileMode_VarSharedAcrossLines)
{
    EXPECT_EQ(runFile({"var x = 10;", "print x;"}), "10\n");
}

TEST_F(IntegrationFixture, IT_19_FileMode_BlockScopeAndShadowing)
{
    EXPECT_EQ(runFile({
        "var x = \"global\";",
        "{",
        "  var x = \"inner\";",
        "  print x;",
        "}",
        "print x;"
    }), "inner\nglobal\n");
}

TEST_F(IntegrationFixture, IT_20_FileMode_BlockModifiesOuterVar)
{
    EXPECT_EQ(runFile({
        "var count = 0;",
        "{",
        "  count = count + 1;",
        "}",
        "print count;"
    }), "1\n");
}

TEST_F(IntegrationFixture, IT_21_FileMode_NestedScope)
{
    EXPECT_EQ(runFile({
        "var outer = \"A\";",
        "{",
        "  var inner = \"B\";",
        "  {",
        "    print outer + inner;",
        "  }",
        "}"
    }), "AB\n");
}

TEST_F(IntegrationFixture, IT_22_FileMode_DanglingElse)
{
    EXPECT_EQ(runFile({
        "if (true)",
        "  if (false) print \"kfc\";",
        "  else print \"bbq\";"
    }), "bbq\n");
}

// ================================================================
// 4. 디버그 모드 통합 테스트
// ================================================================

// 스크립트 기반 컨트롤러 — stdin 없이 step/cont 명령을 소화하며
// DebugController::beforeExecute 의 watch 자동 출력 동작을 재현한다
class WatchIntegrationController : public DebugController
{
public:
    enum class Cmd { STEP, CONT };
    std::vector<Cmd> script;

    void beforeExecute(Stmt* /*stmt*/, Environment* env, int /*depth*/) override
    {
        if (state_ == ExecutionState::RUNNING) return;

        if (!watches_.empty())
            watches_.printWatches(env);

        Cmd cmd = fired_ < script.size() ? script[fired_] : Cmd::CONT;
        ++fired_;
        state_ = (cmd == Cmd::STEP) ? ExecutionState::STEP : ExecutionState::RUNNING;
    }
private:
    size_t fired_ = 0;
};

// IT_DBG_01: watch 자동 출력 — 정지 시점마다 감시 변수 값 출력
TEST(IntegrationTest, IT_DBG_01_WatchAutoPrintAtEachPause)
{
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("var a = 1;\na = a + 1;\nprint a;");
    Parser parser;
    auto stmts = parser.parse(tokens);

    WatchIntegrationController ctrl;
    using Cmd = WatchIntegrationController::Cmd;
    ctrl.addWatch("a");
    ctrl.script = { Cmd::STEP, Cmd::STEP, Cmd::CONT };

    Executor executor;
    executor.setDebugController(&ctrl);

    testing::internal::CaptureStdout();
    executor.execute(stmts);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("스코프 밖"), std::string::npos);
    EXPECT_NE(out.find("a = 1"),    std::string::npos);
    EXPECT_NE(out.find("a = 2"),    std::string::npos);
}

// ================================================================
// 5. 종합 시나리오 — REPL / 파일 / 디버그 모드 전체 통합
// ================================================================

static const std::vector<std::string> kComprehensiveLines = {
    "print 1 + 2 * 3;",
    "print (1 + 2) * 3;",
    "print 10 - 4 - 3;",
    "print 8 / 2 / 2;",
    "print -3 + 2;",
    "", "",
    "print 1 < 2;",
    "print 3 > 5;",
    "",
    "print \"Hello, \" + \"CodeFab!\";",
    "", "",
    "print 5;",
    "print 5.0;",
    "print 3.14;",
    "",
    "print true;",
    "print false;",
    "",
    "var a = 10;",
    "var b = 20;",
    "print a + b;",
    "", "",
    "a = a + 5;",
    "print a;",
    "",
    "var x = \"global\";",
    "{",
    "  var x = \"inner\";",
    "  print x;",
    "}",
    "print x;",
    "",
    "var count = 0;",
    "{",
    "  count = count + 1;",
    "}",
    "print count;",
    "",
    "var outer = \"A\";",
    "{",
    "  var inner = \"B\";",
    "  {",
    "    print outer + inner;",
    "  }",
    "}",
    "", "",
    "if (true) print \"bbq\";",
    "",
    "if (false) print \"no\"; else print \"kfc\";",
    "", "",
    "if (true)",
    "  if (false) print \"kfc\";",
    "  else print \"bbq\";",
    "", "",
    "for (var j = 0; j < 3; j = j + 1) { print j; }"
};

static const std::string kComprehensiveExpected =
    "7\n9\n3\n2\n-1\n"
    "true\nfalse\n"
    "Hello, CodeFab!\n"
    "5\n5\n3.14\n"
    "true\nfalse\n"
    "30\n15\n"
    "inner\nglobal\n"
    "1\n"
    "AB\n"
    "bbq\nkfc\nbbq\n"
    "0\n1\n2\n";

// IT_COMP_01: 종합 — REPL 모드 (Shell::run, stdin 리다이렉트)
TEST(IntegrationTest, IT_COMP_01_REPLMode)
{
    std::string input;
    for (const auto& line : kComprehensiveLines)
        input += line + "\n";
    input += "exit\n";

    std::istringstream iss(input);
    struct CinGuard {
        std::streambuf* orig;
        ~CinGuard() { std::cin.rdbuf(orig); }
    } guard{ std::cin.rdbuf(iss.rdbuf()) };

    Shell shell;
    testing::internal::CaptureStdout();
    shell.run();
    std::string out = testing::internal::GetCapturedStdout();

    // "> " / "... " 프롬프트 제거 후 프로그램 출력만 추출
    std::string clean;
    for (size_t i = 0; i < out.size(); )
    {
        if      (out.compare(i, 4, "... ") == 0) { i += 4; }
        else if (out.compare(i, 2, "> ")   == 0) { i += 2; }
        else                                       { clean += out[i++]; }
    }
    EXPECT_EQ(clean, kComprehensiveExpected);
}

// IT_COMP_02: 종합 — 파일 모드 (FileRunner::runSource)
TEST_F(IntegrationFixture, IT_COMP_02_FileMode)
{
    EXPECT_EQ(runFile(kComprehensiveLines), kComprehensiveExpected);
}

// 디버그 모드용 — beforeExecute 마다 즉시 RUNNING 으로 전환
class AutoContinueController : public DebugController
{
public:
    void beforeExecute(Stmt*, Environment*, int) override
    {
        state_ = ExecutionState::RUNNING;
    }
};

// IT_COMP_03: 종합 — 디버그 모드 (DebugShell::runSource, 자동 진행)
TEST(IntegrationTest, IT_COMP_03_DebugMode)
{
    DebugShell debugShell;
    AutoContinueController ctrl;

    testing::internal::CaptureStdout();
    debugShell.runSource(kComprehensiveLines, ctrl);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), kComprehensiveExpected);
}
