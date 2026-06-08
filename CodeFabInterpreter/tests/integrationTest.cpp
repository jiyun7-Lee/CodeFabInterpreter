#include <gtest/gtest.h>
#include "../Shell.h"

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
