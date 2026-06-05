# Function 테스트 케이스 명세서

## 대상 파일

`tests/FunctionTest.cpp`

---

## TC 목록

| TC ID     | 테스트 함수명                   | 구분     | 구현 상태  |
|-----------|-------------------------------|---------|----------|
| TC-FN-01  | ParsesFunctionDeclaration      | Positive | 🟢 Green  |
| TC-FN-02  | ParsesFunctionCall             | Positive | 🟢 Green  |
| TC-FN-03  | ParsesReturnStatement          | Positive | 🟢 Green  |
| TC-FN-03b | ParsesEmptyReturnStatement     | Positive | 🟢 Green  |
| TC-FN-04  | ExecutesFunctionCall           | Positive | 🟢 Green  |
| TC-FN-05  | FunctionScopeIsolation         | Positive | 🟢 Green  |
| TC-FN-06  | RecursiveFunctionCall          | Positive | 🟢 Green  |
| TC-FN-07  | ReturnValueAssignment          | Positive | 🟢 Green  |
| TC-FN-08  | CheckerReturnOutsideFunction   | Negative | 🟢 Green  |
| TC-FN-09  | CheckerDuplicateParameters     | Negative | 🟢 Green  |
| TC-FN-10  | CheckerCallingNonFunction      | Negative | 🟢 Green  |
| TC-FN-11  | CheckerArgumentCountMismatch   | Negative | 🟢 Green  |
| TC-FN-12  | UndefinedFunctionCallThrows    | Negative | 🟢 Green  |
| TC-FN-13  | CallWithTooManyArgsThrows      | Negative | 🟢 Green  |
| TC-FN-14  | NullReturnUsedInExprThrows     | Negative | 🟢 Green  |

---

## TC 상세

---

### TC-FN-01 — ParsesFunctionDeclaration

**목적**
`func` 키워드로 시작하는 함수 선언이 `FunctionDeclareStmt`로 파싱되는지 확인

**입력 토큰**
```
FUNC "func"  IDENTIFIER "add"  LEFT_PAREN  IDENTIFIER "a"  COMMA
IDENTIFIER "b"  RIGHT_PAREN  LEFT_BRACE
RETURN "return"  IDENTIFIER "a"  PLUS "+"  IDENTIFIER "b"  SEMICOLON
RIGHT_BRACE  EOF
```

**기대 AST**
```
FunctionDeclareStmt
  name   = Token(IDENTIFIER, "add")
  params = [Token(IDENTIFIER, "a"), Token(IDENTIFIER, "b")]
  body   = BlockStmt(ReturnStmt(BinaryExpr(a + b)))
```

| 단계 | 내용 |
|------|------|
| Arrange | func add(a, b) { return a+b; } 에 해당하는 토큰 벡터 수동 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `stmts[0]`이 `FunctionDeclareStmt*`, name == "add", params.size() == 2 |

**구현 상태** : 완료

---

### TC-FN-02 — ParsesFunctionCall

**목적**
`identifier(args...)` 형태의 함수 호출이 `FunctionCallExpr`로 파싱되는지 확인

**입력 토큰**
```
IDENTIFIER "add"  LEFT_PAREN  NUMBER 1.0  COMMA  NUMBER 2.0  RIGHT_PAREN  SEMICOLON  EOF
```

**기대 AST**
```
ExpressionStmt
  FunctionCallExpr
    callee = Token(IDENTIFIER, "add")
    args   = [LiteralExpr(1.0), LiteralExpr(2.0)]
```

| 단계 | 내용 |
|------|------|
| Arrange | add(1, 2); 에 해당하는 토큰 벡터 수동 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `ExpressionStmt → FunctionCallExpr`, callee == "add", args.size() == 2 |

**구현 상태** : 완료

---

### TC-FN-03 — ParsesReturnStatement

**목적**
`return expr;` 이 `ReturnStmt`로 파싱되는지 확인

**입력 토큰**
```
RETURN "return"  NUMBER 42.0  SEMICOLON  EOF
```

**기대 AST**
```
ReturnStmt
  value = LiteralExpr(42.0)
```

| 단계 | 내용 |
|------|------|
| Arrange | return 42; 에 해당하는 토큰 벡터 수동 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `stmts[0]`이 `ReturnStmt*`, `value`가 `LiteralExpr(42.0)` |

**구현 상태** : 완료

---

### TC-FN-04 — ExecutesFunctionCall

**목적**
함수 선언 후 호출 시 인자가 바인딩되어 올바른 반환값이 출력되는지 확인

**시나리오**
```
func add(a, b) { return a+b; }
print add(3, 4);
```

**기대 결과**
- stdout == `"7\n"`

| 단계 | 내용 |
|------|------|
| Arrange | FunctionDeclareStmt(add, [a,b], ReturnStmt(a+b)), PrintStmt(FunctionCallExpr(add, [3, 4])) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | stdout == `"7\n"` |

**구현 상태** : 완료

---

### TC-FN-05 — FunctionScopeIsolation

**목적**
함수 내부에서 선언한 변수가 외부 스코프에 영향을 주지 않는지 확인

**시나리오**
```
var x = 10;
func setLocal() { var x = 99; }
setLocal();
print x;
```

**기대 결과**
- stdout == `"10\n"` (함수 내부의 `var x = 99`가 외부 x에 영향 없음)

| 단계 | 내용 |
|------|------|
| Arrange | VarDeclareStmt(x=10), FunctionDeclareStmt(setLocal, [], BlockStmt(var x=99)), ExpressionStmt(setLocal()), PrintStmt(x) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | stdout == `"10\n"` |

**구현 상태** : 완료

---

### TC-FN-06 — RecursiveFunctionCall

**목적**
재귀 함수 호출이 올바르게 동작하고 반환값이 누적되는지 확인

**시나리오**
```
func fact(n) { if (n < 2) { return 1; } return n * fact(n - 1); }
print fact(5);
```

**기대 결과**
- stdout == `"120\n"`

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | stdout == `"120\n"` |

**구현 상태** : 완료

---

### TC-FN-07 — ReturnValueAssignment

**목적**
함수 반환값을 변수에 대입한 후 사용할 수 있는지 확인

**시나리오**
```
func add(a, b) { return a + b; }
var ret = add(3, 7);
print ret;
```

**기대 결과**
- stdout == `"10\n"`

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | stdout == `"10\n"` |

**구현 상태** : 완료

---

### TC-FN-08 — CheckerReturnOutsideFunction

**목적**
함수 외부에서 `return` 사용 시 Checker 가 오류를 감지하는지 확인

**시나리오**
```
return 5;
```

**기대 결과**
- `checker.getErrors()` 비어있지 않음
- 오류 메시지에 `"함수 외부"` 포함

| 단계 | 내용 |
|------|------|
| Arrange | `checkerErrors("return 5;")` |
| Act | Checker 실행 |
| Assert | `errors` 비어있지 않음, 메시지에 `"함수 외부"` 포함 |

**구현 상태** : 완료

---

### TC-FN-09 — CheckerDuplicateParameters

**목적**
함수 파라미터 이름이 중복될 때 Checker 가 오류를 감지하는지 확인

**시나리오**
```
func foo(a, a) { return a; }
```

**기대 결과**
- `checker.getErrors()` 비어있지 않음
- 오류 메시지에 `"중복"` 포함

| 단계 | 내용 |
|------|------|
| Arrange | `checkerErrors("func foo(a, a) { return a; }")` |
| Act | Checker 실행 |
| Assert | `errors` 비어있지 않음, 메시지에 `"중복"` 포함 |

**구현 상태** : 완료

---

### TC-FN-10 — CheckerCallingNonFunction

**목적**
변수를 함수처럼 호출할 때 Checker 가 오류를 감지하는지 확인

**시나리오**
```
var x = 10;
x();
```

**기대 결과**
- `checker.getErrors()` 비어있지 않음
- 오류 메시지에 `"함수가 아닙니다"` 포함

| 단계 | 내용 |
|------|------|
| Arrange | `checkerErrors("var x = 10; x();")` |
| Act | Checker 실행 |
| Assert | `errors` 비어있지 않음, 메시지에 `"함수가 아닙니다"` 포함 |

**구현 상태** : 완료

---

### TC-FN-11 — CheckerArgumentCountMismatch

**목적**
함수 호출 시 인자 개수가 파라미터 수와 다를 때 Checker 가 오류를 감지하는지 확인

**시나리오**
```
func foo(a, b, c) { return a; }
foo(1, 2);
```

**기대 결과**
- `checker.getErrors()` 비어있지 않음
- 오류 메시지에 `"불일치"` 포함

| 단계 | 내용 |
|------|------|
| Arrange | `checkerErrors(...)` |
| Act | Checker 실행 |
| Assert | `errors` 비어있지 않음, 메시지에 `"불일치"` 포함 |

**구현 상태** : 완료

---

### TC-FN-12 — UndefinedFunctionCallThrows _(global: F-26)_

**목적**
선언되지 않은 함수를 호출하면 Executor 가 `std::runtime_error` 를 발생시키는지 확인

**시나리오**
```
foo();
```

**기대 결과**
- `std::runtime_error("Undefined function 'foo'")` 발생

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-FN-13 — CallWithTooManyArgsThrows _(global: F-27)_

**목적**
Executor 레벨에서 인자 개수 초과 호출 시 `std::runtime_error` 가 발생하는지 확인  
(Checker 를 우회한 경우에도 Executor 가 독립적으로 검증)

**시나리오**
```
func f(a) { return a; }
f(1, 2);
```

**기대 결과**
- `std::runtime_error("'f': wrong number of arguments")` 발생

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-FN-14 — NullReturnUsedInExprThrows _(global: F-28)_

**목적**
`null` 을 반환하는 함수의 결과를 산술 연산에 사용하면 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
func f() {}
var r = f() + 1;
```

**기대 결과**
- `std::runtime_error("Type error: operands must be numbers")` 발생

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료
