# 통합 테스트 케이스 명세서

## 대상 파일

`tests/integrationTest.cpp`

---

## 테스트 방식 공통 사항

| 구분 | 방식 |
|------|------|
| **REPL 모드 (단일 입력)** | `Shell::runLine(소스)` — 멀티라인 문자열을 한 번에 전달 |
| **REPL 모드 (줄 단위)** | `Shell::run()` + `std::cin` 리다이렉트 — 실제 REPL 루프 검증 |
| **파일 모드** | `FileRunner::runSource(줄 배열)` — `accumulateBraces` 축적 경로 검증 |
| **디버그 모드** | `DebugShell::runSource(줄 배열, 컨트롤러)` + 커스텀 `DebugController` |
| **정상 동작 검증** | `EXPECT_EQ(captured_stdout, 기대_문자열)` |
| **에러 검출 검증** | `EXPECT_NE(출력.find("[Error]" or "[Checker]"), npos)` |

---

## 1. 정상 동작 테스트 — Shell::runLine

| TC ID  | 테스트 함수명                        | 입력 코드 요약                                      | 기대 출력          | 상태       |
|--------|--------------------------------------|----------------------------------------------------|--------------------|------------|
| IT_01  | IT_01_ArithmeticPrecedence           | `print 1+2*3; ... print -3+2;`                     | `7\n9\n3\n2\n-1\n` | 🟢 Green   |
| IT_02  | IT_02_ComparisonOperators            | `print 1<2; print 3>5;`                            | `true\nfalse\n`    | 🟢 Green   |
| IT_03  | IT_03_StringConcatenation            | `print "Hello, " + "CodeFab!";`                    | `Hello, CodeFab!\n`| 🟢 Green   |
| IT_04  | IT_04_NumberFormat                   | `print 5; print 5.0; print 3.14;`                  | `5\n5\n3.14\n`     | 🟢 Green   |
| IT_05  | IT_05_BooleanLiterals                | `print true; print false;`                         | `true\nfalse\n`    | 🟢 Green   |
| IT_06  | IT_06_VarDeclareAndReassign          | `var a=10; var b=20; print a+b; a=a+5; print a;`   | `30\n15\n`         | 🟢 Green   |
| IT_07  | IT_07_BlockScopeAndShadowing         | 외부 `x="global"` / 내부 `x="inner"` 선언          | `inner\nglobal\n`  | 🟢 Green   |
| IT_08  | IT_08_BlockModifiesOuterVar          | 블록 내부에서 외부 `count` 증가                     | `1\n`              | 🟢 Green   |
| IT_09  | IT_09_NestedScope                    | 중첩 블록에서 `outer+"B"` 출력                      | `AB\n`             | 🟢 Green   |
| IT_10  | IT_10_IfSimple                       | `if (true) print "bbq";`                           | `bbq\n`            | 🟢 Green   |
| IT_11  | IT_11_IfElse                         | `if (false) print "no"; else print "kfc";`         | `kfc\n`            | 🟢 Green   |
| IT_12  | IT_12_DanglingElse                   | 중첩 if — else 가장 가까운 if 에 결합               | `bbq\n`            | 🟢 Green   |
| IT_13  | IT_13_ForLoop                        | `for (var j=0; j<3; j=j+1) { print j; }`          | `0\n1\n2\n`        | 🟢 Green   |
| IT_14  | IT_14_IfBlockNextLine                | `if (true)` 다음 줄에 `{...}`                      | `bbq\n`            | 🟢 Green   |
| IT_15  | IT_15_ForBlockNextLine               | `for (...)` 다음 줄에 `{...}`                      | `0\n1\n2\n`        | 🟢 Green   |

---

### IT_01 — ArithmeticPrecedence

**목적**  
산술 연산자 우선순위 및 좌결합성 확인

**입력 코드**
```
print 1 + 2 * 3;
print (1 + 2) * 3;
print 10 - 4 - 3;
print 8 / 2 / 2;
print -3 + 2;
```

**기대 출력**
```
7
9
3
2
-1
```

---

### IT_04 — NumberFormat

**목적**  
정수 값은 `.0` 없이 출력, 소수점 있는 값은 그대로 출력

**입력 코드**
```
print 5;
print 5.0;
print 3.14;
```

**기대 출력**
```
5
5
3.14
```

> `5.0` 은 내부적으로 `double(5.0)` 이지만 `std::ostream` 기본 포맷으로 `5` 출력

---

### IT_12 — DanglingElse

**목적**  
else 는 문법상 가장 가까운 if 에 결합 (C/Java 동일 규칙)

**입력 코드**
```
if (true)
  if (false) print "kfc";
  else print "bbq";
```

**기대 출력** : `bbq`  
(outer if true → inner if false → else 진입)

---

## 2. 에러 검출 테스트 — Shell::runLine

| TC ID   | 테스트 함수명                      | 입력 코드 요약                        | 기대 출력 패턴   | 상태     |
|---------|------------------------------------|--------------------------------------|-----------------|----------|
| IT_E01  | IT_E01_MissingSemicolon            | `print 1 + 2` (`;` 없음)             | `[Error]`       | 🟢 Green |
| IT_E02  | IT_E02_MissingClosingParen         | `print (1 + 2;` (`)` 없음)           | `[Error]`       | 🟢 Green |
| IT_E03  | IT_E03_InvalidAssignmentTarget     | `a + b = 3;`                         | `[Error]`       | 🟢 Green |
| IT_E04  | IT_E04_UnexpectedToken             | `print * 5;`                         | `[Error]`       | 🟢 Green |
| IT_E05  | IT_E05_SelfInitReference           | `{ var a = a; }`                     | `[Checker]`     | 🟢 Green |
| IT_E06  | IT_E06_DuplicateDeclaration        | `{ var a = "hi"; var a = 3; }`       | `[Checker]`     | 🟢 Green |
| IT_E07  | IT_E07_UndefinedVariable           | `print notDefined;`                  | `[Error]`       | 🟢 Green |
| IT_E08  | IT_E08_TypeMismatchPlus            | `print 1 + "HI";`                    | `[Error]`       | 🟢 Green |
| IT_E09  | IT_E09_UnaryMinusTypeMismatch      | `print -"FabCoding";`                | `[Error]`       | 🟢 Green |

> **IT_E03** : `Parser.cpp` `parseAssignment()` 에서 좌변이 `VariableExpr` / `ArrayAccessExpr` 아닌 경우 `throw std::runtime_error("잘못된 할당 대상")` 추가로 검출

---

## 3. 디버그 모드 테스트 — DebugController

| TC ID      | 테스트 함수명                           | 검증 항목                              | 상태     |
|------------|-----------------------------------------|---------------------------------------|----------|
| IT_DBG_01  | IT_DBG_01_WatchAutoPrintAtEachPause     | 정지 시마다 감시 변수 값 자동 출력     | 🟢 Green |

---

### IT_DBG_01 — WatchAutoPrintAtEachPause

**목적**  
`DebugController::beforeExecute` 정지 시 `watches_.printWatches(env)` 가 자동 호출되는지 확인

**헬퍼 클래스** : `WatchIntegrationController`  
`DebugController` 서브클래스. `script` 벡터의 `Cmd::STEP` / `Cmd::CONT` 로 stdin 없이 명령 소화.

**입력 코드**
```
var a = 1;
a = a + 1;
print a;
```

**명령 스크립트** : `STEP, STEP, CONT`

**감시 변수** : `a`

**기대 출력 포함 여부**

| 정지 순서 | 시점              | 기대 포함 문자열 |
|-----------|-------------------|-----------------|
| 1         | 줄 1 실행 전      | `스코프 밖`      |
| 2         | 줄 2 실행 전      | `a = 1`         |
| 3         | 줄 3 실행 전      | `a = 2`         |

---

## 4. 파일 모드 멀티라인 테스트 — FileRunner::runSource

`accumulateBraces` 가 여러 줄을 누적해 완성 문장을 올바르게 판별하는지 검증.  
`pendingControl` / `parenDepth` 추가로 아래 케이스들이 통과.

| TC ID  | 테스트 함수명                        | 입력 구조 요약                                       | 기대 출력         | 상태     |
|--------|--------------------------------------|-----------------------------------------------------|-------------------|----------|
| IT_16  | IT_16_FileMode_ForLoopMultiline      | `for (...)` 다음 줄 `{` (파일 모드)                 | `3\n4\n`          | 🟢 Green |
| IT_17  | IT_17_FileMode_IfBlockMultiline      | `if (true)` 다음 줄 `{` (파일 모드)                 | `bbq\n`           | 🟢 Green |
| IT_18  | IT_18_FileMode_VarSharedAcrossLines  | `var x = 10;` / `print x;` 줄 분리                 | `10\n`            | 🟢 Green |
| IT_19  | IT_19_FileMode_BlockScopeAndShadowing| 외부/내부 변수 섀도잉 (파일 모드)                    | `inner\nglobal\n` | 🟢 Green |
| IT_20  | IT_20_FileMode_BlockModifiesOuterVar | 블록 내부에서 외부 변수 수정 (파일 모드)              | `1\n`             | 🟢 Green |
| IT_21  | IT_21_FileMode_NestedScope           | 중첩 블록에서 외부 변수 접근 (파일 모드)              | `AB\n`            | 🟢 Green |
| IT_22  | IT_22_FileMode_DanglingElse          | dangling else 멀티라인 (파일 모드)                  | `bbq\n`           | 🟢 Green |

### accumulateBraces pendingControl 동작 원리

| 토큰 / 상황 | pendingControl 변화 | 비고 |
|-------------|--------------------|-----------------------------|
| `if` / `for` (최상위) | `++` | 바디 미수신 상태 등록 |
| `{` (최상위, pendingControl > 0) | `--` | 블록 바디 수신 |
| `;` (최상위·괄호 밖, pendingControl > 0) | `--` | 인라인 바디 수신 |
| `else` | 변화 없음 | 직전 if pending 그대로 승계 |
| `(` / `)` | parenDepth ±1 | `for(;;)` 내부 `;` 보호 |

---

### IT_16 — FileMode_ForLoopMultiline

**입력 줄 배열**
```
for (var a = 3; a < 5; a = a + 1)
{
  print a;
}
```

**누적 흐름**

| 줄 | braceDepth | pendingControl | 완성? |
|----|------------|----------------|-------|
| 1  | 0          | 1              | No    |
| 2  | 1          | 0              | No    |
| 3  | 1          | 0              | No    |
| 4  | 0          | 0              | **Yes** |

**기대 출력** : `3\n4\n`

---

### IT_22 — FileMode_DanglingElse

**입력 줄 배열**
```
if (true)
  if (false) print "kfc";
  else print "bbq";
```

**누적 흐름**

| 줄 | 이벤트 | pendingControl |
|----|--------|----------------|
| 1  | `if` (최상위) | 1 |
| 2  | `if` (최상위) → 2 / `;` → 1 | 1 |
| 3  | `else` 무시 / `;` → 0 | 0 → **완성** |

**기대 출력** : `bbq`

---

## 5. 종합 시나리오 — REPL / 파일 / 디버그 모드

동일 소스(`kComprehensiveLines`)를 세 가지 실행 경로로 실행해 동일한 출력을 내는지 검증.

**공통 소스 요약**
```
print 1 + 2 * 3;  ...  print -3 + 2;
print 1 < 2;  print 3 > 5;
print "Hello, " + "CodeFab!";
print 5;  print 5.0;  print 3.14;
print true;  print false;
var a = 10;  var b = 20;  print a + b;
a = a + 5;  print a;
var x = "global";  { var x = "inner"; print x; }  print x;
var count = 0;  { count = count + 1; }  print count;
var outer = "A";  { var inner = "B"; { print outer + inner; } }
if (true) print "bbq";
if (false) print "no"; else print "kfc";
if (true)  if (false) print "kfc";  else print "bbq";
for (var j = 0; j < 3; j = j + 1) { print j; }
```

**공통 기대 출력**
```
7  9  3  2  -1  true  false  Hello, CodeFab!
5  5  3.14  true  false  30  15
inner  global  1  AB
bbq  kfc  bbq  0  1  2
```

| TC ID       | 테스트 함수명              | 실행 경로                                          | 상태     |
|-------------|----------------------------|----------------------------------------------------|----------|
| IT_COMP_01  | IT_COMP_01_REPLMode        | `Shell::run()` + stdin 리다이렉트 + 프롬프트 제거   | 🟢 Green |
| IT_COMP_02  | IT_COMP_02_FileMode        | `FileRunner::runSource(kComprehensiveLines)`        | 🟢 Green |
| IT_COMP_03  | IT_COMP_03_DebugMode       | `DebugShell::runSource()` + `AutoContinueController`| 🟢 Green |

---

### IT_COMP_01 — REPLMode

**방식**  
`kComprehensiveLines` 를 `\n` 으로 연결 후 `"exit\n"` 추가 → `std::istringstream` 으로 `std::cin` 리다이렉트

**프롬프트 제거 로직**  
캡처된 stdout 에서 `"... "` → `"> "` 순으로 제거 후 프로그램 출력만 추출해 비교

---

### IT_COMP_03 — DebugMode

**헬퍼 클래스** : `AutoContinueController`  
`DebugController` 서브클래스. `beforeExecute` 에서 즉시 `state_ = ExecutionState::RUNNING` 으로 전환 → stdin 없이 전 구간 실행.

```cpp
class AutoContinueController : public DebugController
{
public:
    void beforeExecute(Stmt*, Environment*, int) override
    { state_ = ExecutionState::RUNNING; }
};
```

---

## 요약

| 구분 | TC 수 | 범위 | 상태 |
|------|--------|------|------|
| 정상 동작 (Shell::runLine) | 15 | IT_01 ~ IT_15 | 🟢 All Green |
| 에러 검출 (Shell::runLine) | 9  | IT_E01 ~ IT_E09 | 🟢 All Green |
| 디버그 모드 | 1  | IT_DBG_01 | 🟢 Green |
| 파일 모드 멀티라인 (FileRunner) | 7  | IT_16 ~ IT_22 | 🟢 All Green |
| 종합 시나리오 | 3  | IT_COMP_01 ~ IT_COMP_03 | 🟢 All Green |
| **합계** | **35** | | 🟢 **283/283 통과** |
