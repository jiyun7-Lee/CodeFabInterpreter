# CodeFab 프로젝트 통합 설계서 (v1)

## 담당자 - Role
송용길님    Tokenizer    TokenType, Token Class, Lexer, Tokenize 테스트

김홍원님    Expression Parser    LiteralExpr, VariableExpr, UnaryExpr, BinaryExpr, AssignExpr, GroupingExpr

심민구님    Statement Parser + Checker    VarStmt, PrintStmt, BlockStmt, IfStmt, ForStmt, Checker

이지윤님    Executor + Prompt Shell    Environment, Scope, Runtime Error, Executor, Shell


## 1. 프로젝트 개요

### CodeFab 이란?

-   Interpreter(인터프리터)를 의미
-   Python처럼 코드를 읽고 즉시 실행
-   Custom Language를 해석하고 실행하는 엔진

### 최종 목표

1.  Custom Language 개발
2.  Interpreter 개발
3.  Prompt Shell 개발
4.  테스트 스크립트 통과

------------------------------------------------------------------------

# 2. 전체 아키텍처

``` text
사용자 입력
    ↓
Prompt Shell
    ↓
Assembler
    ↓
Checker
    ↓
Executor
    ↓
실행 결과 출력
```

Prompt Shell에서 한 줄 입력될 때마다

Assembler → Checker → Executor

파이프라인이 수행된다.

------------------------------------------------------------------------

# 3. Unit 구조

## Assembler

역할 - Source Code → AST 변환

구성

``` text
Assembler
 ├─ Tokenizer
 └─ Parser
```

산출물 - Token List - AST

------------------------------------------------------------------------

## Checker

역할 - AST 의미 분석 - 실행 전 오류 검출

검사 항목 - 중복 선언 - 자기 초기화 참조 - 선언 전 사용(선택) - 기타
정적 오류

------------------------------------------------------------------------

## Executor

역할 - AST 실행 - Scope 관리 - Runtime Error 처리

------------------------------------------------------------------------

## Prompt Shell

역할 - 사용자 입력 수신 - 파이프라인 호출

------------------------------------------------------------------------

# 4. Token 설계

## TokenType

``` cpp
enum class TokenType
{
    LEFT_PAREN,
    RIGHT_PAREN,

    LEFT_BRACE,
    RIGHT_BRACE,

    COMMA,
    DOT,
    SEMICOLON,

    PLUS,
    MINUS,
    STAR,
    SLASH,

    EQUAL,
    GREATER,
    LESS,

    IDENTIFIER,
    STRING,
    NUMBER,

    VAR,
    PRINT,

    IF,
    ELSE,

    FOR,

    TRUE,
    FALSE,

    AND,
    OR,

    EOF_TOKEN
};
```

------------------------------------------------------------------------

## Token Class

``` cpp
class Token
{
public:
    TokenType type;

    std::string lexeme;

    int line;

    std::variant<
        std::monostate,
        double,
        std::string,
        bool
    > literal;
};
```

------------------------------------------------------------------------

## Tokenizer Interface

``` cpp
class Tokenizer
{
public:
    std::vector<Token>
    tokenize(const std::string& source);
};
```

------------------------------------------------------------------------

# 5. AST 구조

## Expr

값을 계산하는 노드

``` cpp
LiteralExpr
VariableExpr
UnaryExpr
BinaryExpr
AssignExpr
GroupingExpr
```

------------------------------------------------------------------------

## LiteralExpr

``` cpp
class LiteralExpr : public Expr
{
    Value value;
};
```

------------------------------------------------------------------------

## VariableExpr

``` cpp
class VariableExpr : public Expr
{
    Token name;
};
```

------------------------------------------------------------------------

## UnaryExpr

``` cpp
class UnaryExpr : public Expr
{
    Token op;
    Expr* right;
};
```

------------------------------------------------------------------------

## BinaryExpr

``` cpp
class BinaryExpr : public Expr
{
    Expr* left;
    Token op;
    Expr* right;
};
```

------------------------------------------------------------------------

## AssignExpr

``` cpp
class AssignExpr : public Expr
{
    Token name;
    Expr* value;
};
```

------------------------------------------------------------------------

## GroupingExpr

``` cpp
class GroupingExpr : public Expr
{
    Expr* expression;
};
```

------------------------------------------------------------------------

# 6. Statement 구조

## Statement 종류

``` cpp
ExpressionStmt
PrintStmt
VarDeclareStmt
BlockStmt
IfStmt
ForStmt
```

------------------------------------------------------------------------

## ExpressionStmt

``` cpp
class ExpressionStmt : public Stmt
{
    Expr* expression;
};
```

------------------------------------------------------------------------

## PrintStmt

``` cpp
class PrintStmt : public Stmt
{
    Expr* expression;
};
```

------------------------------------------------------------------------

## VarDeclareStmt

``` cpp
class VarDeclareStmt : public Stmt
{
    Token name;
    Expr* initializer;
};
```

------------------------------------------------------------------------

## BlockStmt

``` cpp
class BlockStmt : public Stmt
{
    std::vector<Stmt*> statements;
};
```

------------------------------------------------------------------------

## IfStmt

``` cpp
class IfStmt : public Stmt
{
    Expr* condition;

    Stmt* thenBranch;

    Stmt* elseBranch;
};
```

------------------------------------------------------------------------

## ForStmt

``` cpp
class ForStmt : public Stmt
{
    Stmt* init;

    Expr* condition;

    Expr* increment;

    Stmt* body;
};
```

------------------------------------------------------------------------

# 7. AST 규칙

허용

``` text
Stmt
 ├─ Expr
 └─ Stmt
```

금지

``` text
Expr
 └─ Stmt
```

규칙

-   Expr 내부에 Stmt 금지
-   AST Root는 항상 Stmt
-   Token은 노드가 아니라 필드

------------------------------------------------------------------------

# 8. Environment 구조

## Scope 구조

``` text
Global Scope
      ↑
Local Scope
      ↑
Local Scope
```

------------------------------------------------------------------------

## Environment

``` cpp
class Environment
{
public:
    std::unordered_map<
        std::string,
        Value
    > values;

    Environment* parent;
};
```

------------------------------------------------------------------------

## 변수 탐색

``` text
현재 Scope
 ↓
부모 Scope
 ↓
부모 Scope
 ↓
Global Scope
```

------------------------------------------------------------------------

## Shadowing 허용

``` cpp
var x = "global";

{
    var x = "inner";
}
```

------------------------------------------------------------------------

# 9. Value 구조

``` cpp
using Value =
std::variant<
    std::monostate,
    double,
    std::string,
    bool
>;
```

지원 타입

-   Number
-   String
-   Boolean
-   Nil

------------------------------------------------------------------------

# 10. Checker 설계

## 역할

AST DFS 순회

------------------------------------------------------------------------

## 검사 항목

### 중복 선언

``` cpp
{
    var a = 1;
    var a = 2;
}
```

오류

------------------------------------------------------------------------

### 자기 초기화

``` cpp
{
    var a = a;
}
```

오류

------------------------------------------------------------------------

## 알고리즘

``` text
DFS
 ↓
Scope 생성
 ↓
선언 확인
 ↓
오류 보고
```

------------------------------------------------------------------------

# 11. Executor 설계

## Expr 평가

지원

-   Literal
-   Variable
-   Unary
-   Binary
-   Assign
-   Grouping

------------------------------------------------------------------------

## Statement 실행

지원

-   Print
-   Var
-   Block
-   If
-   For

------------------------------------------------------------------------

## Runtime Error

### Undefined Variable

``` cpp
print abc;
```

------------------------------------------------------------------------

### Type Error

``` cpp
print 1 + "HI";
```

------------------------------------------------------------------------

### Divide By Zero

``` cpp
print 10 / 0;
```

------------------------------------------------------------------------

# 12. Prompt Shell

예시

``` text
>>> var a = 5;
>>> var b = 10;
>>> print a+b;

15
```

동작

``` text
입력
 ↓
Assembler
 ↓
Checker
 ↓
Executor
 ↓
출력
```

------------------------------------------------------------------------

# 13. 역할 분담 추천

## A

Tokenizer

담당

-   TokenType
-   Token
-   Lexer

------------------------------------------------------------------------

## B

Expression Parser

담당

-   LiteralExpr
-   VariableExpr
-   UnaryExpr
-   BinaryExpr
-   AssignExpr
-   GroupingExpr

------------------------------------------------------------------------

## C

Statement Parser + Checker

담당

-   VarStmt
-   PrintStmt
-   IfStmt
-   ForStmt
-   BlockStmt
-   Checker

------------------------------------------------------------------------

## D

Executor + Shell

담당

-   Environment
-   Runtime Error
-   Executor
-   Prompt Shell

------------------------------------------------------------------------

# 14. 개발 룰

## Rule 1

코딩 전 반드시 확정

-   TokenType
-   Expr 종류
-   Stmt 종류
-   Environment 구조
-   Runtime Error 정책

------------------------------------------------------------------------

## Rule 2

main 직접 Push 금지

브랜치 예시

-   feature/tokenizer
-   feature/expr-parser
-   feature/stmt-checker
-   feature/executor
-   bugfix/console-encoding

------------------------------------------------------------------------

## Rule 3

Claude 활용 원칙

설계 → 사람이 결정

구현 → Claude 활용

------------------------------------------------------------------------

## Rule 4

TDD 우선

기능 구현 전 테스트 작성

------------------------------------------------------------------------

## Rule 5

커밋 메시지 헤더 규칙

TDD 사이클 및 작업 유형에 따라 아래 헤더를 붙인다.

| 헤더 | 사용 단계 / 상황 | 예시 |
|---|---|---|
| `[unitTest]` | TDD Red 단계 — 테스트 코드 작성 | `[unitTest] Add TC-01 ParsesNumberLiteral` |
| `[feature]` | TDD Green 단계 — 기능 구현 | `[feature] Implement Expression Parser` |
| `[refactoring]` | TDD Refactor 단계 — 코드 개선 | `[refactoring] Extract makeBinary helper` |
| `[doc]` | 문서 추가 / 수정 | `[doc] Add TC_ExprParser specification` |
| `[build]` | 빌드 환경 / 설정 변경 | `[build] Add /utf-8 compiler flag` |
| `[fix]` | 버그 수정 | `[fix] Set console output code page to UTF-8` |

------------------------------------------------------------------------

# 15. 일정

## Day1 오전

09:00 \~ 10:30

설계 확정

-   Token
-   Expr
-   Stmt
-   Environment

10:30 \~ 12:00

Claude 활용

기본 코드 생성

------------------------------------------------------------------------

## Day1 오후

병렬 개발

A - Tokenizer

B - Expr Parser

C - Stmt Parser - Checker

D - Executor - Shell

------------------------------------------------------------------------

## Day1 17:00

1차 통합

목표

print 1 + 2 \* 3;

------------------------------------------------------------------------

## Day1 20:00

2차 통합

목표

var a = 10; print a;

------------------------------------------------------------------------

## Day2 오전

구현

-   if
-   for
-   Scope
-   Shadowing

Runtime Error

-   Undefined Variable
-   Type Error
-   Divide By Zero

------------------------------------------------------------------------

## Day2 점심

테스트 스크립트 전체 통과

------------------------------------------------------------------------

# 16. 최종 체크리스트

\[ \] Tokenizer

\[x\] LiteralExpr \[x\] VariableExpr \[x\] UnaryExpr \[x\] BinaryExpr \[x\] AssignExpr \[x\] GroupingExpr

\[ \] VarStmt \[ \] PrintStmt \[ \] BlockStmt \[ \] IfStmt \[ \] ForStmt

\[ \] Checker

\[ \] Environment

\[ \] Runtime Error

\[ \] Prompt Shell

\[ \] 테스트 스크립트 통과

------------------------------------------------------------------------

핵심

AST 구조와 Environment 구조를 먼저 확정한다.

설계가 고정되면 Claude를 활용해 병렬 개발한다.
