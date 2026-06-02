#include <gtest/gtest.h>
#include "../Parser.h"

// ============================================================
// ParserTest.cpp  —  C파트: Statement Parser TC 목록
// ============================================================
//
// [공통] 토큰은 직접 구성, parseExpression()은 Mock 사용
//
// ------------------------------------------------------------
// VarDeclareStmt
// ------------------------------------------------------------
// P-V-01 : var a = 3;              → VarDeclareStmt, name="a", initializer != null
// P-V-02 : var abc = "hello";      → VarDeclareStmt, name="abc"
// P-V-03 : var flag = true;        → VarDeclareStmt, name="flag"
// P-V-04 : var a = b + 1;         → VarDeclareStmt, initializer != null (expr mock)
// P-V-05 : var a = 3  (세미콜론 없음)  → parse 오류
//
// ------------------------------------------------------------
// PrintStmt
// ------------------------------------------------------------
// P-P-01 : print a;               → PrintStmt, expression != null
// P-P-02 : print a + b;           → PrintStmt, expression != null (expr mock)
// P-P-03 : print a  (세미콜론 없음)    → parse 오류
//
// ------------------------------------------------------------
// BlockStmt
// ------------------------------------------------------------
// P-B-01 : {}                     → BlockStmt, statements.size() == 0
// P-B-02 : { print a; }           → BlockStmt, statements.size() == 1
// P-B-03 : { print a; print b; }  → BlockStmt, statements.size() == 2
// P-B-04 : { { print a; } }       → 중첩 BlockStmt
// P-B-05 : { print a;  (닫는 중괄호 없음) → parse 오류
//
// ------------------------------------------------------------
// IfStmt
// ------------------------------------------------------------
// P-I-01 : if (a > 0) print a;                    → IfStmt, e
// P-I-02 : if (a > 0) print a; else print b;      → IfStmt, elseBranch != null
// P-I-03 : if (a > 0) { print a; }                → IfStmt, thenBranch == BlockStmt
// P-I-04 : if (a > 0) { } else { }               → IfStmt, 양쪽 모두 BlockStmt
// P-I-05 : if (a > 0) if (b > 0) print b;         → 중첩 IfStmt
// P-I-06 : if a > 0) print a;  (여는 괄호 없음)      → parse 오류
// P-I-07 : if (a > 0 print a;  (닫는 괄호 없음)      → parse 오류
//
// ------------------------------------------------------------
// ForStmt
// ------------------------------------------------------------
// P-F-01 : for (var i = 0; i < 3; i = i+1) print i;   → ForSt
// P-F-02 : for (var i = 0; i < 3; i = i+1) { print i; } → body == BlockStmt
// P-F-03 : for (var i = 0; i < 3  i = i+1) ...  (세미콜론 없음) → parse 오류
// P-F-04 : for (var i = 0; i < 3; i = i+1) {}         → body
//
// ------------------------------------------------------------
// 복수 문장
// ------------------------------------------------------------
// P-M-01 : var a = 1; print a;    → statements.size() == 2
// P-M-02 : EOF만 있는 경우           → statements.size() == 0
