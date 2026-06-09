# CodeFabInterpreter

Custom Language를 해석하고 실행하는 인터프리터입니다.

---

## 아키텍처

```
사용자 입력
    ↓
FactoryShell       ← 실행 모드 분기 (REPL / FILE / DEBUG)
    ↓
Tokenizer          ← 소스 코드 → Token List
    ↓
Parser             ← Token List → AST
    ↓
Checker            ← AST 정적 분석 (중복 선언, 자기 초기화 등)
    ↓
Executor           ← AST 실행, Scope 관리, Runtime Error 처리
    ↓
실행 결과 출력
```

### 컴포넌트 설명

| 컴포넌트 | 역할 |
|---|---|
| `FactoryShell` | 실행 인자에 따라 REPL / FILE / DEBUG 모드 분기 |
| `Shell` | REPL 대화형 실행, 멀티라인 brace 누적 처리 |
| `FileRunner` | 소스 파일을 읽어 줄 단위로 누적 후 실행 |
| `DebugShell` | 파일 실행 + 줄별 정지 / 명령어 입력 디버그 |
| `Tokenizer` | 소스 코드를 Token List로 변환 |
| `Parser` | Token List를 AST(Expr/Stmt 트리)로 변환 |
| `Checker` | 실행 전 정적 오류 검출 |
| `Executor` | AST 순회 실행, Environment(Scope) 관리 |

---

## 디자인 패턴

### 리팩토링을 통해 적용된 패턴

| 패턴 | 적용 위치 | 문서 |
|------|-----------|------|
| **Visitor** | `Expr` 계층 × `Executor` — dynamic_cast 체인을 이중 디스패치로 대체 | [PATTERN_Visitor.md](CodeFabInterpreter/PATTERN_Visitor.md) |
| **Strategy** | `Parser` — 표현식 파싱 알고리즘을 합성으로 교체 가능하도록 분리 | [PATTERN_Strategy.md](CodeFabInterpreter/PATTERN_Strategy.md) |

### 초기 설계 시 적용된 패턴

| 패턴 | 적용 위치 | 문서 |
|------|-----------|------|
| **Interpreter** | `Expr`/`Stmt` 계층 — 언어 문법 규칙을 클래스로 표현, `Executor`가 AST를 해석 | [PATTERN_Interpreter.md](CodeFabInterpreter/PATTERN_Interpreter.md) |
| **Factory Method** | `FactoryShell` — 실행 인자에 따라 `Shell`/`FileRunner`/`DebugShell` 생성 | [PATTERN_Factory.md](CodeFabInterpreter/PATTERN_Factory.md) |
| **Template Method** | `BaseRunner::executeSource()` — tokenize→parse→check→execute 파이프라인 고정 | [PATTERN_TemplateMethod.md](CodeFabInterpreter/PATTERN_TemplateMethod.md) |
| **Observer** | `Executor` ↔ `DebugController` — 각 Stmt 실행 전 `beforeExecute()` 이벤트 후킹 | [PATTERN_Observer.md](CodeFabInterpreter/PATTERN_Observer.md) |
| **State** | `ExecutionState` + `DebugController` — STEP/RUNNING/PAUSED/NEXT 상태 전환 | [PATTERN_State.md](CodeFabInterpreter/PATTERN_State.md) |
| **Composite** | `Stmt` 계층 — `BlockStmt`/`IfStmt`/`ForStmt`가 자식 Stmt를 포함하는 트리 구조 | [PATTERN_Composite.md](CodeFabInterpreter/PATTERN_Composite.md) |

---

## 실행 방법

```bash
# REPL 모드 — 대화형 입력
./CodeFabInterpreter

# FILE 모드 — 소스 파일 실행
./CodeFabInterpreter run <파일경로>

# DEBUG 모드 — 줄별 정지 디버그
./CodeFabInterpreter debug <파일경로>
```

### REPL 모드

```
> var x = 5;
> print x;
5
> exit
```

멀티라인 블록은 자동 누적됩니다.

```
> if (x > 0) {
...   print x;
... }
5
```

### DEBUG 모드 명령어

| 명령어 | 설명 |
|---|---|
| `step` / `s` | 현재 줄 실행 후 다음 줄에서 정지 |
| `next` / `n` | 블록 내부 진입 없이 다음 줄로 이동 |
| `continue` / `c` | breakpoint까지 자유 실행 |
| `break N` | N번째 줄에 breakpoint 설정 |
| `remove N` | N번째 줄 breakpoint 해제 |
| `breakpoints` | 설정된 breakpoint 목록 출력 |
| `watch V` | 변수 V를 감시 등록 (정지 시 자동 출력) |
| `unwatch V` | 변수 V 감시 해제 |
| `watches` | 현재 감시 중인 변수 목록 및 값 출력 |
| `inspect` | 현재 스코프의 모든 변수 출력 |

---

## 언어 문법

모든 구문은 세미콜론(`;`)으로 끝나야 합니다.

### 변수 선언 및 할당

```
var x = 10;
var name = "hello";
var flag = true;
x = x + 1;
```

### 출력

```
print x;
print x + 1;
print "hello";
```

### 연산자

```
print 1 + 2 * 3;      // 7
print 10 / 2 - 1;     // 4
print 10 % 3;         // 1
print 3 > 2;          // true
print 3 < 2;          // false
print true and false; // false
print true or false;  // true
print !true;          // false
```

### 조건문

```
var x = 5;
if (x > 0) {
    print x;
} else {
    print 0;
}
```

### 반복문

```
for (var i = 0; i < 3; i = i + 1) {
    print i;
}
```

### 함수

`func` 또는 `Func` 키워드 모두 사용 가능합니다.

```
func add(a, b) {
    return a + b;
}

var result = add(3, 7);
print result;   // 10
```

재귀 함수:

```
func fact(n) {
    if (n < 2) { return 1; }
    return n * fact(n - 1);
}

print fact(5);  // 120
```

### 배열

`Array(n)` 으로 크기 n의 배열을 생성합니다. 초기값은 `null`입니다.

```
var arr = Array(3);
arr[0] = 10;
arr[1] = 20;
arr[2] = 30;
print arr[1];   // 20
```

### 스코프

블록 `{}` 안에서 선언한 변수는 블록 밖에서 접근 불가합니다. 같은 이름의 변수를 안쪽 스코프에서 재선언(shadowing)할 수 있습니다.

```
var x = "global";
{
    var x = "inner";
    print x;    // inner
}
print x;        // global
```

---

## 개발 룰

### 브랜치 전략

`main` 직접 Push 금지. 브랜치를 생성 후 PR로 병합합니다.

```
feature/기능명
bugfix/버그명
refactor/리팩토링명
```

### 커밋 메시지 규칙

헤더는 **소문자**로 작성합니다.

| 헤더 | 허용 표기 | 사용 상황 | 예시 |
|---|---|---|---|
| `[feat]` | `[feat]` / `[feature]` | 기능 구현 | `[feat] Implement Expression Parser` |
| `[test]` | `[test]` / `[unittest]` | 테스트 코드 작성 | `[test] Add TC-01 ParsesNumberLiteral` |
| `[fix]` | `[fix]` | 버그 수정 | `[fix] Set console output code page to UTF-8` |
| `[refactor]` | `[refactor]` / `[refactoring]` | 코드 개선 | `[refactor] Extract makeBinary helper` |
| `[docs]` | `[docs]` / `[doc]` | 문서 추가 / 수정 | `[docs] Update README` |
| `[build]` | `[build]` | 빌드 환경 / 설정 변경 | `[build] Add /utf-8 compiler flag` |
| `[chore]` | `[chore]` | 그 외 잡무 (설정, 의존성 등) | `[chore] Update .gitignore` |

---

### PR & 리뷰

#### A. PR 작성

PR에는 아래 3가지를 반드시 포함합니다.

| 항목 | 설명 |
|---|---|
| **변경 내용** | 무엇을 왜 바꿨는지 |
| **확인 방법** | 빌드·실행·테스트 방법 |
| **리뷰 포인트** | 리뷰어가 집중해서 봐야 할 부분 |

#### B. 리뷰

- 리뷰 요청 후 **3시간 이내** 1차 리뷰
- 코멘트 확인 후 **2시간 이내** 응답
- 리뷰 코멘트는 정중하게 작성

#### C. Merge

- 최소 **1명** 승인 후 Merge
- 핵심 기능은 **2명 이상** 리뷰

---

### 코드 품질

#### A. 검증

- Merge 전 직접 실행 또는 테스트 수행

#### B. Git 관리

- Push 전 최신 코드 Pull
- 충돌 여부 확인 후 작업

#### C. 코드 정리

- PR 전 코드 포맷팅 수행

---

## 테스트 케이스 명세

전체 TC: **312개**

| 명세 파일 | 대상 모듈 | TC 수 |
|-----------|-----------|------:|
| [TC_Tokenizer.md](CodeFabInterpreter/TC_Tokenizer.md) | Tokenizer | 26 |
| [TC_ExprParser.md](CodeFabInterpreter/TC_ExprParser.md) | Expression Parser | 22 |
| [TC_StmtParserAndChecker.md](CodeFabInterpreter/TC_StmtParserAndChecker.md) | Statement Parser + Checker | 58 |
| [TC_executor.md](CodeFabInterpreter/TC_executor.md) | Executor | 27 |
| [TC_Function.md](CodeFabInterpreter/TC_Function.md) | 함수 | 14 |
| [TC_Array.md](CodeFabInterpreter/TC_Array.md) | 배열 | 12 |
| [TC_Specification_Chapter4.md](CodeFabInterpreter/TC_Specification_Chapter4.md) | 최적화 | 41 |
| [TC_Step.md](CodeFabInterpreter/TC_Step.md) | 디버그 Step | 4 |
| [TC_Integration.md](CodeFabInterpreter/TC_Integration.md) | 통합 테스트 | 37 |
| [TC_NegativeTest.md](CodeFabInterpreter/TC_NegativeTest.md) | 네거티브 테스트 (Parser / Checker / Runtime 에러) | 27 |
