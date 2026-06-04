# Function 테스트 케이스 명세서

## 대상 파일

`tests/FunctionTest.cpp`

---

## TC 목록

| TC ID   | 테스트 함수명              | 구현 상태  |
|---------|--------------------------|----------|
| TC-FN-01 | ParsesFunctionDeclaration | 🟢 Green  |
| TC-FN-02 | ParsesFunctionCall        | 🟢 Green  |
| TC-FN-03 | ParsesReturnStatement     | 🟢 Green  |
| TC-FN-04 | ExecutesFunctionCall      | 🟢 Green  |
| TC-FN-05 | FunctionScopeIsolation    | 🟢 Green  |

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
