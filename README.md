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

| 헤더 | 사용 상황 | 예시 |
|---|---|---|
| `[unitTest]` | TDD Red — 테스트 코드 작성 | `[unitTest] Add TC-01 ParsesNumberLiteral` |
| `[feature]` | TDD Green — 기능 구현 | `[feature] Implement Expression Parser` |
| `[refactoring]` | TDD Refactor — 코드 개선 | `[refactoring] Extract makeBinary helper` |
| `[fix]` | 버그 수정 | `[fix] Set console output code page to UTF-8` |
| `[doc]` | 문서 추가 / 수정 | `[doc] Update README` |
| `[build]` | 빌드 환경 / 설정 변경 | `[build] Add /utf-8 compiler flag` |
