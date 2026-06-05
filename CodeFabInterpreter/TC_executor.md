# Executor 테스트 케이스 명세서

## 대상 파일

`tests/ExecutorTest.cpp`

---

## TC 목록

| TC ID  | 테스트 함수명                   | 구분     | 구현 상태 |
|--------|-------------------------------|---------|---------|
| TC0    | ExpressionStatement           | Positive | 🟢 Green  |
| TC1    | StmtReceivedCorrectly         | Positive | 🟢 Green  |
| TC2    | PrintLiteral                  | Positive | 🟢 Green  |
| TC3    | ArithmeticExpr                | Positive | 🟢 Green  |
| TC4    | VarDeclareAndUse              | Positive | 🟢 Green  |
| TC5    | IfStatement                   | Positive | 🟢 Green  |
| TC6    | ForStatement                  | Positive | 🟢 Green  |
| TC7    | BlockScope_ScopeLifecycle     | Positive | 🟢 Green  |
| TC7-1  | BlockScope_NestedScopes       | Positive | 🟢 Green  |
| TC8    | UndefinedVariable             | Negative | 🟢 Green  |
| TC9    | TypeError                     | Negative | 🟢 Green  |
| TC10   | DivideByZero                  | Negative | 🟢 Green  |
| TC11-1 | UnaryExpr_Minus               | Positive | 🟢 Green  |
| TC11-2 | UnaryExpr_Bang                | Positive | 🟢 Green  |
| TC11-3 | UnaryExpr_TypeMismatch        | Negative | 🟢 Green  |
| TC12   | GroupingExpr                  | Positive | 🟢 Green  |
| TC13   | LogicalAnd                    | Positive | 🟢 Green  |
| TC14   | LogicalOr                     | Positive | 🟢 Green  |
| TC15   | StringPlusNumberThrows        | Negative | 🟢 Green  |
| TC16   | StringComparisonThrows        | Negative | 🟢 Green  |
| TC17   | BoolArithmeticThrows          | Negative | 🟢 Green  |
| TC18   | NullArithmeticThrows          | Negative | 🟢 Green  |
| TC19   | AssignUndeclaredVarThrows     | Negative | 🟢 Green  |
| TC20   | NullReturnArithmeticThrows    | Negative | 🟢 Green  |

---

## TC 상세

---

### TC1 — StmtReceivedCorrectly

**목적**
`execute()`에 `Stmt*` 벡터가 정상적으로 전달되는지 확인

**사전 조건**
- `LiteralExpr` → `value = 42.0`
- `PrintStmt` → `expression = LiteralExpr`
- `std::vector<Stmt*> stmts = { PrintStmt }`

**실행**
```cpp
executor.execute(stmts);
```

**기대 결과**
- 예외 없이 실행 완료 (`ASSERT_NO_THROW`)
- `stmts.size() == 1`
- `stmts[0]`이 `PrintStmt*`로 캐스팅 가능
- `PrintStmt->expression == LiteralExpr`

**구현 상태** : 완료

---

### TC2 — PrintLiteral

**목적**
`PrintStmt + LiteralExpr` 조합에서 숫자/문자열 값이 stdout에 출력되는지 확인

**사전 조건**
- 숫자 리터럴 케이스 : `LiteralExpr { value = 3.14 }`
- 문자열 리터럴 케이스 : `LiteralExpr { value = "hello" }`
- 각 `PrintStmt`에 위 `Expr` 연결

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- 숫자 케이스 : stdout = `"3.14\n"` (또는 정수면 `"3\n"`)
- 문자열 케이스 : stdout = `"hello\n"`

**구현 상태** : 완료

---

### TC3 — ArithmeticExpr

**목적**
`BinaryExpr` 사칙연산 결과가 올바르게 계산되는지 확인

**사전 조건**

| 케이스 | 연산식        | 기대 결과 |
|--------|-------------|---------|
| 덧셈   | `3.0 + 4.0` | `7`     |
| 뺄셈   | `10.0 - 3.0`| `7`     |
| 곱셈   | `2.0 * 5.0` | `10`    |
| 나눗셈 | `9.0 / 3.0` | `3`     |

**실행**
```cpp
executor.execute({ printStmt });  // BinaryExpr를 expression으로 갖는 PrintStmt
```

**기대 결과**
- 각 케이스별 stdout에 계산 결과 출력

**구현 상태** : 완료

---

### TC4 — VarDeclareAndUse

**목적**
`VarDeclareStmt`로 선언한 변수를 `VariableExpr`로 참조할 수 있는지 확인

**사전 조건**
```text
var x = 10.0;
print x;
```
- `VarDeclareStmt { name = "x", initializer = LiteralExpr(10.0) }`
- `PrintStmt { expression = VariableExpr { name = "x" } }`

**실행**
```cpp
executor.execute({ varDeclareStmt, printStmt });
```

**기대 결과**
- stdout = `"10\n"`

**구현 상태** : 완료

---

### TC5 — IfStatement

**목적**
`IfStmt`의 condition 결과에 따라 `thenBranch` / `elseBranch`가 선택 실행되는지 확인

**사전 조건**

| 케이스      | condition    | 기대 실행 분기  |
|-------------|-------------|--------------|
| true  케이스 | `true` 리터럴 | `thenBranch` |
| false 케이스 | `false` 리터럴| `elseBranch` |

**실행**
```cpp
executor.execute({ ifStmt });
```

**기대 결과**
- true 케이스 : `thenBranch` 내 `PrintStmt` 실행, stdout = `"then\n"`
- false 케이스 : `elseBranch` 내 `PrintStmt` 실행, stdout = `"else\n"`

**구현 상태** : 완료

---

### TC6 — ForStatement

**목적**
`ForStmt`의 `init → condition → body → increment` 순서가 올바르게 반복 실행되는지 확인

**사전 조건**
```text
for (var i = 0; i < 3; i = i + 1) { print i; }
```
- `init` : `VarDeclareStmt { name = "i", initializer = 0.0 }`
- `condition` : `BinaryExpr { i < 3.0 }`
- `increment` : `AssignExpr { i = i + 1.0 }`
- `body` : `PrintStmt { VariableExpr("i") }`

**실행**
```cpp
executor.execute({ forStmt });
```

**기대 결과**
- stdout = `"0\n1\n2\n"`

**구현 상태** : 완료

---

### TC7 — BlockScope_ScopeLifecycle

**목적**
`LEFT_BRACE` 진입 시 새 변수 저장소 생성, `RIGHT_BRACE` 탈출 시 소멸됨을 Shadowing으로 검증

**사전 조건**
```text
var x = 1.0;
{
    var x = 2.0;
    print x;     // 2 출력
}
print x;         // 1 출력
```

**실행**
```cpp
executor.execute({ globalVar, block, printOuter });
```

**기대 결과**
- stdout = `"2\n1\n"`

**구현 상태** : 완료

---

### TC7-1 — BlockScope_NestedScopes

**목적**
중첩 블록마다 독립적인 로컬 저장소가 생성/소멸되는지 확인

**사전 조건**
```text
var x = 1.0;
{
    var x = 2.0;
    {
        var x = 3.0;
        print x;    // 3 출력
    }
    print x;        // 2 출력
}
print x;            // 1 출력
```

**실행**
```cpp
executor.execute({ decl1, outerBlock, print1 });
```

**기대 결과**
- stdout = `"3\n2\n1\n"`

**구현 상태** : 완료

---

### TC8 — UndefinedVariable

**목적**
선언되지 않은 변수 참조 시 `RuntimeError`가 발생하는지 확인

**사전 조건**
```text
print abc;   // 'abc' 미선언
```
- `PrintStmt { expression = VariableExpr { name = "abc" } }`
- Environment에 `"abc"` 없음

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- `RuntimeError` (또는 `std::runtime_error`) 예외 발생
- 정상 종료 안 됨

**구현 상태** : 완료

---

### TC9 — TypeError

**목적**
타입 불일치 연산 (`number + string` 등) 시 `RuntimeError`가 발생하는지 확인

**사전 조건**
```text
print 1 + "HI";
```
- `BinaryExpr { LiteralExpr(1.0) PLUS LiteralExpr("HI") }`

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- `RuntimeError` 예외 발생

**구현 상태** : 완료

---

### TC10 — DivideByZero

**목적**
0으로 나누기 시 `RuntimeError`가 발생하는지 확인

**사전 조건**
```text
print 10 / 0;
```
- `BinaryExpr { LiteralExpr(10.0) SLASH LiteralExpr(0.0) }`

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- `RuntimeError` 예외 발생

**구현 상태** : 완료

---

### TC11 — UnaryExpr

**목적**
`UnaryExpr`의 MINUS(부호 반전), BANG(논리 반전) 연산이 올바르게 동작하는지 확인

**사전 조건**

| 케이스 | 입력 | 기대 결과 |
|--------|------|---------|
| MINUS  | `UnaryExpr { MINUS, LiteralExpr(3.0) }` | `"-3\n"` |
| BANG   | `UnaryExpr { BANG, LiteralExpr(true) }` | `"false\n"` |
| 타입 불일치 | `UnaryExpr { MINUS, LiteralExpr("hello") }` | `RuntimeError` |

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- MINUS 케이스 : stdout = `"-3\n"`
- BANG 케이스 : stdout = `"false\n"`
- 타입 불일치 케이스 : `RuntimeError` 예외 발생

**구현 상태** : 완료

---

### TC12 — GroupingExpr

**목적**
괄호로 묶인 `GroupingExpr` 내부 expression이 올바르게 평가되는지 확인

**사전 조건**

| 케이스 | 입력 | 기대 결과 |
|--------|------|---------|
| 괄호 산술 | `GroupingExpr { BinaryExpr(1.0 + 2.0) }` | `"3\n"` |
| 중첩 (괄호 + 단항) | `GroupingExpr { UnaryExpr(MINUS, 3.0) }` | `"-3\n"` |

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- 괄호 산술 케이스 : stdout = `"3\n"`
- 중첩 케이스 : stdout = `"-3\n"`

**구현 상태** : 완료

---

### TC13 — LogicalAnd

**목적**
`BinaryExpr AND` 연산이 올바르게 동작하고 단락 평가가 적용되는지 확인

**사전 조건**

| 케이스 | 입력 | 기대 결과 |
|--------|------|---------|
| true and true  | `LiteralExpr(true) AND LiteralExpr(true)` | `"true\n"` |
| true and false | `LiteralExpr(true) AND LiteralExpr(false)` | `"false\n"` |
| false and true | `LiteralExpr(false) AND LiteralExpr(true)` | `"false\n"` |
| false and false| `LiteralExpr(false) AND LiteralExpr(false)` | `"false\n"` |
| 단락 평가 | `false AND (1/0)` | 예외 없이 실행 (`ASSERT_NO_THROW`) |

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- 4가지 bool 조합 결과 정확히 출력
- 왼쪽이 false일 때 오른쪽 평가 없이 반환 (단락 평가)

**구현 상태** : 완료

---

### TC14 — LogicalOr

**목적**
`BinaryExpr OR` 연산이 올바르게 동작하고 단락 평가가 적용되는지 확인

**사전 조건**

| 케이스 | 입력 | 기대 결과 |
|--------|------|---------|
| true or true   | `LiteralExpr(true) OR LiteralExpr(true)` | `"true\n"` |
| true or false  | `LiteralExpr(true) OR LiteralExpr(false)` | `"true\n"` |
| false or true  | `LiteralExpr(false) OR LiteralExpr(true)` | `"true\n"` |
| false or false | `LiteralExpr(false) OR LiteralExpr(false)` | `"false\n"` |
| 단락 평가 | `true OR (1/0)` | 예외 없이 실행 (`ASSERT_NO_THROW`) |

**실행**
```cpp
executor.execute({ printStmt });
```

**기대 결과**
- 4가지 bool 조합 결과 정확히 출력
- 왼쪽이 true일 때 오른쪽 평가 없이 반환 (단락 평가)

**구현 상태** : 완료

---

### TC15 — StringPlusNumberThrows _(global: E-17)_

**목적**
문자열(lhs) + 숫자 연산 시 `std::runtime_error` 가 발생하는지 확인

**사전 조건**
- `BinaryExpr { LiteralExpr("hello") PLUS LiteralExpr(1.0) }`

**기대 결과**
- `std::runtime_error("Type error: operands must be numbers")` 발생

| 단계 | 내용 |
|------|------|
| Arrange | LiteralExpr("hello"), LiteralExpr(1.0), BinaryExpr(PLUS) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC16 — StringComparisonThrows _(global: E-18)_

**목적**
문자열끼리 대소 비교(`>`) 시 `std::runtime_error` 가 발생하는지 확인

**사전 조건**
- `BinaryExpr { LiteralExpr("a") GREATER LiteralExpr("b") }`

**기대 결과**
- `std::runtime_error` 발생

| 단계 | 내용 |
|------|------|
| Arrange | LiteralExpr("a"), LiteralExpr("b"), BinaryExpr(GREATER) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC17 — BoolArithmeticThrows _(global: E-19)_

**목적**
bool + 숫자 산술 연산 시 `std::runtime_error` 가 발생하는지 확인

**사전 조건**
- `BinaryExpr { LiteralExpr(true) PLUS LiteralExpr(1.0) }`

**기대 결과**
- `std::runtime_error` 발생

| 단계 | 내용 |
|------|------|
| Arrange | LiteralExpr(true), LiteralExpr(1.0), BinaryExpr(PLUS) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC18 — NullArithmeticThrows _(global: E-20)_

**목적**
null(monostate) + 숫자 산술 연산 시 `std::runtime_error` 가 발생하는지 확인

**사전 조건**
- `BinaryExpr { LiteralExpr(monostate) PLUS LiteralExpr(1.0) }`

**기대 결과**
- `std::runtime_error` 발생

| 단계 | 내용 |
|------|------|
| Arrange | LiteralExpr(std::monostate{}), LiteralExpr(1.0), BinaryExpr(PLUS) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC19 — AssignUndeclaredVarThrows _(global: E-21)_

**목적**
`var` 선언 없이 변수에 대입 시 `std::runtime_error` 가 발생하는지 확인

**사전 조건**
- `AssignExpr { name = "x", value = LiteralExpr(5.0) }` (환경에 `x` 없음)

**기대 결과**
- `std::runtime_error("Undefined variable 'x'")` 발생

| 단계 | 내용 |
|------|------|
| Arrange | AssignExpr(x, 5.0) → ExpressionStmt 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC20 — NullReturnArithmeticThrows _(global: E-22)_

**목적**
`null` 을 반환하는 함수 결과를 산술 연산에 사용하면 `std::runtime_error` 가 발생하는지 확인

**사전 조건**
```
func f() {}
print f() + 1;
```

**기대 결과**
- `std::runtime_error` 발생 (monostate + double)

| 단계 | 내용 |
|------|------|
| Arrange | FunctionDeclareStmt(f, [], BlockStmt{}), PrintStmt(BinaryExpr(FunctionCallExpr(f), PLUS, 1.0)) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료
