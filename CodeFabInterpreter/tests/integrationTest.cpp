#include <gtest/gtest.h>
#include <sstream>
#include "../Shell.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include "../DebugController.h"
#include "../Stmt.h"

// ================================================================
// 통합 테스트 — 테스트 스크립트 명세서 기반
// Shell::runLine() 에 전체 소스를 한 번에 전달해 인터프리터 로직을 검증
// ================================================================

// ----------------------------------------------------------------
// 1-1. 산술 연산자 우선순위
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_01_ArithmeticPrecedence)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "print 1 + 2 * 3;\n"    // 7
        "print (1 + 2) * 3;\n"  // 9
        "print 10 - 4 - 3;\n"   // 3
        "print 8 / 2 / 2;\n"    // 2
        "print -3 + 2;"         // -1
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "7\n9\n3\n2\n-1\n");
}

// ----------------------------------------------------------------
// 1-2. 비교 연산자
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_02_ComparisonOperators)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "print 1 < 2;\n"  // true
        "print 3 > 5;"    // false
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "true\nfalse\n");
}

// ----------------------------------------------------------------
// 1-3. 문자열 연결
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_03_StringConcatenation)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print \"Hello, \" + \"CodeFab!\";");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "Hello, CodeFab!\n");
}

// ----------------------------------------------------------------
// 1-4. 숫자 출력 포맷 (정수는 .0 없이)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_04_NumberFormat)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "print 5;\n"     // 5
        "print 5.0;\n"   // 5
        "print 3.14;"    // 3.14
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "5\n5\n3.14\n");
}

// ----------------------------------------------------------------
// 1-5. boolean 리터럴 출력
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_05_BooleanLiterals)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "print true;\n"   // true
        "print false;"    // false
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "true\nfalse\n");
}

// ----------------------------------------------------------------
// 1-6. 변수 선언 & 재할당
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_06_VarDeclareAndReassign)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "var a = 10;\n"
        "var b = 20;\n"
        "print a + b;\n"  // 30
        "a = a + 5;\n"
        "print a;"        // 15
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "30\n15\n");
}

// ----------------------------------------------------------------
// 1-7. 블록 스코프 & 변수 섀도잉
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_07_BlockScopeAndShadowing)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "var x = \"global\";\n"
        "{\n"
        "  var x = \"inner\";\n"
        "  print x;\n"           // inner
        "}\n"
        "print x;"               // global
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "inner\nglobal\n");
}

// ----------------------------------------------------------------
// 1-8. 블록 내부에서 외부 변수 수정
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_08_BlockModifiesOuterVar)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "var count = 0;\n"
        "{\n"
        "  count = count + 1;\n"
        "}\n"
        "print count;"  // 1
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// ----------------------------------------------------------------
// 1-9. 중첩 스코프
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_09_NestedScope)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "var outer = \"A\";\n"
        "{\n"
        "  var inner = \"B\";\n"
        "  {\n"
        "    print outer + inner;\n"  // AB
        "  }\n"
        "}"
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "AB\n");
}

// ----------------------------------------------------------------
// 1-10. if 단순 실행
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_10_IfSimple)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("if (true) print \"bbq\";");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "bbq\n");
}

// ----------------------------------------------------------------
// 1-11. if / else
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_11_IfElse)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("if (false) print \"no\"; else print \"kfc\";");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "kfc\n");
}

// ----------------------------------------------------------------
// 1-12. dangling else — 가장 가까운 if 에 결합
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_12_DanglingElse)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "if (true)\n"
        "  if (false) print \"kfc\";\n"
        "  else print \"bbq\";"  // bbq
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "bbq\n");
}

// ----------------------------------------------------------------
// 1-13. for 반복문
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_13_ForLoop)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("for (var j = 0; j < 3; j = j + 1) { print j; }");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "0\n1\n2\n");
}

// ----------------------------------------------------------------
// 1-14. if 블록이 다음 줄에 오는 멀티라인
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_14_IfBlockNextLine)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "if (true)\n"
        "{\n"
        "  print \"bbq\";\n"  // bbq
        "}"
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "bbq\n");
}

// ----------------------------------------------------------------
// 1-15. for 블록이 다음 줄에 오는 멀티라인
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_15_ForBlockNextLine)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine(
        "for (var i = 0; i < 3; i = i + 1)\n"
        "{\n"
        "  print i;\n"  // 0, 1, 2
        "}"
    );
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "0\n1\n2\n");
}

// ================================================================
// 2. 에러 검출 테스트
// ================================================================

// ----------------------------------------------------------------
// 2-1. 세미콜론 누락 (구문 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E01_MissingSemicolon)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print 1 + 2");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-2. 닫는 괄호 누락 (구문 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E02_MissingClosingParen)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print (1 + 2;");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-3. 잘못된 할당 대상 (구문 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E03_InvalidAssignmentTarget)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("var a = 1;\nvar b = 2;\na + b = 3;");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-4. 표현식 자리에 잘못된 토큰 (구문 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E04_UnexpectedToken)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print * 5;");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-5. 자기 초기화 참조 (Checker 정적 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E05_SelfInitReference)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("{ var a = a; }");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Checker]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-6. 같은 스코프 중복 선언 (Checker 정적 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E06_DuplicateDeclaration)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("{ var a = \"hi\"; var a = 3; }");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Checker]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-7. 정의되지 않은 변수 (런타임 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E07_UndefinedVariable)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print notDefined;");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-8. + 연산자 타입 불일치 (런타임 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E08_TypeMismatchPlus)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print 1 + \"HI\";");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ----------------------------------------------------------------
// 2-9. 단항 마이너스 타입 불일치 (런타임 에러)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_E09_UnaryMinusTypeMismatch)
{
    Shell shell;
    testing::internal::CaptureStdout();
    shell.runLine("print -\"FabCoding\";");
    EXPECT_NE(testing::internal::GetCapturedStdout().find("[Error]"), std::string::npos);
}

// ================================================================
// 3. 디버그 모드 통합 테스트
// DebugController + Executor 를 직접 연결해 디버그 동작을 검증
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

        // 정지 시 감시 변수 자동 출력 (DebugController::beforeExecute 와 동일)
        if (!watches_.empty())
            watches_.printWatches(env);

        Cmd cmd = fired_ < script.size() ? script[fired_] : Cmd::CONT;
        ++fired_;
        state_ = (cmd == Cmd::STEP) ? ExecutionState::STEP : ExecutionState::RUNNING;
    }
private:
    size_t fired_ = 0;
};

// ----------------------------------------------------------------
// 3-1. watch 자동 출력 — 정지 시점마다 감시 변수 값 출력
// "var a = 1;\na = a + 1;\nprint a;"  를 step → step → cont 로 실행
// 줄 1 정지 전: a 미정의 → "(스코프 밖)"
// 줄 2 정지 전: a = 1
// 줄 3 정지 전: a = 2
// ----------------------------------------------------------------
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

    EXPECT_NE(out.find("스코프 밖"), std::string::npos); // 줄 1 정지 전: a 미정의
    EXPECT_NE(out.find("a = 1"),    std::string::npos); // 줄 2 정지 전
    EXPECT_NE(out.find("a = 2"),    std::string::npos); // 줄 3 정지 전
}

// ================================================================
// 4. 파일 모드 (FileRunner) 통합 테스트
// FileRunner::runSource() 에 줄 배열을 전달해 파일 실행 경로를 검증
// ================================================================

// ----------------------------------------------------------------
// 3-1. for 블록이 다음 줄에 오는 멀티라인 — 파일 모드
// for (var a = 3; a < 5; a = a + 1)
// {
//   print a;
// }
// → "3\n4\n"
// FileRunner 가 줄 전체를 join 해서 파싱하도록 수정된 후 통과
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_16_FileMode_ForLoopMultiline)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "for (var a = 3; a < 5; a = a + 1)",
        "{",
        "  print a;",
        "}"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "3\n4\n");
}

// ----------------------------------------------------------------
// 3-2. if 블록이 다음 줄에 오는 멀티라인 — 파일 모드
// if (true)
// {
//   print "bbq";
// }
// → "bbq\n"
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_17_FileMode_IfBlockMultiline)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "if (true)",
        "{",
        "  print \"bbq\";",
        "}"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "bbq\n");
}

// ----------------------------------------------------------------
// 3-3. 줄 간 변수 공유 — 파일 모드
// var x = 10;
// print x;
// → "10\n"
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_18_FileMode_VarSharedAcrossLines)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "var x = 10;",
        "print x;"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "10\n");
}

// ----------------------------------------------------------------
// 3-4. 블록 스코프 & 변수 shadowing — 파일 모드
// var x = "global";
// {
//   var x = "inner";
//   print x;
// }
// print x;
// → "inner\nglobal\n"
// FileRunner 가 줄 전체를 join 해서 파싱하도록 수정된 후 통과
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_19_FileMode_BlockScopeAndShadowing)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "var x = \"global\";",
        "{",
        "  var x = \"inner\";",
        "  print x;",
        "}",
        "print x;"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "inner\nglobal\n");
}

// ----------------------------------------------------------------
// 3-5. 안쪽 블록에서 바깥 변수 수정 — 파일 모드
// var count = 0;
// {
//   count = count + 1;
// }
// print count;
// → "1\n"
// FileRunner 가 줄 전체를 join 해서 파싱하도록 수정된 후 통과
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_20_FileMode_BlockModifiesOuterVar)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "var count = 0;",
        "{",
        "  count = count + 1;",
        "}",
        "print count;"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// ----------------------------------------------------------------
// 3-6. 중첩 스코프 — 파일 모드
// var outer = "A";
// {
//   var inner = "B";
//   {
//     print outer + inner;
//   }
// }
// → "AB\n"
// FileRunner 가 줄 전체를 join 해서 파싱하도록 수정된 후 통과
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_21_FileMode_NestedScope)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "var outer = \"A\";",
        "{",
        "  var inner = \"B\";",
        "  {",
        "    print outer + inner;",
        "  }",
        "}"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "AB\n");
}

// ----------------------------------------------------------------
// 3-7. dangling else (멀티라인) — 파일 모드
// if (true)
//   if (false) print "kfc";
//   else print "bbq";
// → "bbq\n"  (else 는 가장 가까운 if 에 결합)
// FileRunner 가 줄 전체를 join 해서 파싱하도록 수정된 후 통과
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_22_FileMode_DanglingElse)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "if (true)",
        "  if (false) print \"kfc\";",
        "  else print \"bbq\";"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "bbq\n");
}

// ----------------------------------------------------------------
// 4-8. 중첩 인라인 if — 파일 모드 (Bug: pendingControl ; 로 1만 감소)
// if (true) if (true) if (true) print "x";
// → "x\n"
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_23_FileMode_NestedInlineIf)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({"if (true) if (true) if (true) print \"x\";"});
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "x\n");
}

// ----------------------------------------------------------------
// 4-9. else 블록이 다음 줄에 오는 멀티라인 — 파일 모드
// if (true)
// {
//   print "a";
// }
// else
// {
//   print "b";
// }
// → "a\n"  (if 바디 실행, else 블록도 정상 누적됨)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_24_FileMode_ElseBlockNextLine)
{
    // 파서 검증 1: 단일 줄 if-else 블록
    {
        Shell shell;
        testing::internal::CaptureStdout();
        shell.runLine("if (true) { print \"a\"; } else { print \"b\"; }");
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "a\n") << "single-line if-else block failed";
    }
    // 파서 검증 2: 멀티라인 if-else 블록
    {
        Shell shell;
        testing::internal::CaptureStdout();
        shell.runLine("if (true)\n{\n  print \"a\";\n}\nelse\n{\n  print \"b\";\n}");
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "a\n") << "multi-line if-else block failed";
    }

    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource({
        "if (true)",
        "{",
        "  print \"a\";",
        "}",
        "else",
        "{",
        "  print \"b\";",
        "}"
    });
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "a\n");
}

// ================================================================
// 5. 종합 시나리오 — REPL / 파일 / 디버그 모드 전체 통합
// 동일 소스를 세 가지 실행 경로로 모두 돌려 결과를 검증한다
// ================================================================

static const std::vector<std::string> kComprehensiveLines = {
    "print 1 + 2 * 3;",
    "print (1 + 2) * 3;",
    "print 10 - 4 - 3;",
    "print 8 / 2 / 2;",
    "print -3 + 2;",
    "",
    "",
    "print 1 < 2;",
    "print 3 > 5;",
    "",
    "print \"Hello, \" + \"CodeFab!\";",
    "",
    "",
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
    "",
    "",
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
    "",
    "",
    "if (true) print \"bbq\";",
    "",
    "if (false) print \"no\"; else print \"kfc\";",
    "",
    "",
    "if (true)",
    "  if (false) print \"kfc\";",
    "  else print \"bbq\";",
    "",
    "",
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

// ----------------------------------------------------------------
// 5-1. 종합 — REPL 모드 (Shell::run, stdin 리다이렉트)
// 줄 단위로 입력을 주입해 accumulateBraces 축적 경로를 검증.
// "> " / "... " 프롬프트를 제거한 뒤 프로그램 출력만 비교한다.
// ----------------------------------------------------------------
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

// ----------------------------------------------------------------
// 5-2. 종합 — 파일 모드 (FileRunner::runSource)
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_COMP_02_FileMode)
{
    FileRunner runner;
    testing::internal::CaptureStdout();
    runner.runSource(kComprehensiveLines);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), kComprehensiveExpected);
}

// 디버그 모드용 — beforeExecute 마다 즉시 RUNNING 으로 전환해 stdin 없이 전체 실행
class AutoContinueController : public DebugController
{
public:
    void beforeExecute(Stmt*, Environment*, int) override
    {
        state_ = ExecutionState::RUNNING;
    }
};

// ----------------------------------------------------------------
// 5-3. 종합 — 디버그 모드 (DebugShell::runSource, 자동 진행)
// AutoContinueController 가 매 정지 시점에 즉시 continue 를 발행해
// stdin 없이 프로그램을 완주한다. 출력은 파일 모드와 동일해야 한다.
// ----------------------------------------------------------------
TEST(IntegrationTest, IT_COMP_03_DebugMode)
{
    DebugShell debugShell;
    AutoContinueController ctrl;

    testing::internal::CaptureStdout();
    debugShell.runSource(kComprehensiveLines, ctrl);
    EXPECT_EQ(testing::internal::GetCapturedStdout(), kComprehensiveExpected);
}
