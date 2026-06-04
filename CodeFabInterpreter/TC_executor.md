# Executor 테스트 케이스 명세서

## 대상 파일

`tests/ExecutorTest.cpp`

---

## TC 목록

| TC ID  | 테스트 함수명                   | 구현 상태 |
|--------|-------------------------------|---------|
| TC1    | StmtReceivedCorrectly         | 🟢 Green  |
| TC2    | PrintLiteral                  | 🟢 Green  |
| TC3    | ArithmeticExpr                | 🟢 Green  |
| TC4    | VarDeclareAndUse              | 🟢 Green  |
| TC5    | IfStatement                   | 🟢 Green  |
| TC6    | ForStatement                  | 🟢 Green  |
| TC7    | BlockScope_ScopeLifecycle     | 🟢 Green  |
| TC7-1  | BlockScope_NestedScopes       | 🟢 Green  |
| TC8    | UndefinedVariable             | 🟢 Green  |
| TC9    | TypeError                     | 🟢 Green  |
| TC10   | DivideByZero                  | 🟢 Green  |
| TC11   | UnaryExpr                     | 🟢 Green  |

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
