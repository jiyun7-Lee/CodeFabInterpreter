# Array 테스트 케이스 명세서

## 대상 파일

`tests/ArrayTest.cpp`

---

## TC 목록

| TC ID    | 테스트 함수명              | 구분     | 구현 상태  |
|----------|--------------------------|---------|----------|
| TC-AR-01 | ParsesArrayLiteral       | Positive | 🟢 Green  |
| TC-AR-02 | ParsesArrayAccess        | Positive | 🟢 Green  |
| TC-AR-03 | ExecutesArrayAccess      | Positive | 🟢 Green  |
| TC-AR-04 | ArrayAccessOutOfBounds   | Negative | 🟢 Green  |
| TC-AR-05 | ArrayCreation            | Positive | 🟢 Green  |
| TC-AR-06 | ArrayWrite               | Positive | 🟢 Green  |
| TC-AR-07 | ArrayIndexNotNumber      | Negative | 🟢 Green  |
| TC-AR-08 | IndexOnNonArray          | Negative | 🟢 Green  |
| TC-AR-09 | ArraySizeNotNumber       | Negative | 🟢 Green  |
| TC-AR-10 | NegativeIndexThrows      | Negative | 🟢 Green  |
| TC-AR-11 | NegativeArraySizeThrows  | Negative | 🟢 Green  |
| TC-AR-12 | ZeroSizeArrayAccessThrows| Negative | 🟢 Green  |
| TC-AR-13 | ParsesArrayWrite         | Positive | 🟢 Green  |

---

## TC 상세

---

### TC-AR-01 — ParsesArrayLiteral

**목적**
`[elem1, elem2, ...]` 형태의 배열 리터럴이 `ArrayLiteralExpr`로 파싱되는지 확인

**입력 토큰**
```
LEFT_BRACKET  NUMBER 1.0  COMMA  NUMBER 2.0  COMMA  NUMBER 3.0  RIGHT_BRACKET  SEMICOLON  EOF
```

**기대 AST**
```
ExpressionStmt
  ArrayLiteralExpr
    elements[0] = LiteralExpr(1.0)
    elements[1] = LiteralExpr(2.0)
    elements[2] = LiteralExpr(3.0)
```

| 단계 | 내용 |
|------|------|
| Arrange | [1, 2, 3]; 에 해당하는 토큰 벡터 수동 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `ExpressionStmt → ArrayLiteralExpr`, elements.size() == 3, elements[0] == 1.0 |

**구현 상태** : 완료

---

### TC-AR-02 — ParsesArrayAccess

**목적**
`arr[index]` 형태의 배열 접근이 `ArrayAccessExpr`로 파싱되는지 확인

**입력 토큰**
```
IDENTIFIER "arr"  LEFT_BRACKET  NUMBER 0.0  RIGHT_BRACKET  SEMICOLON  EOF
```

**기대 AST**
```
ExpressionStmt
  ArrayAccessExpr
    array = VariableExpr("arr")
    index = LiteralExpr(0.0)
```

| 단계 | 내용 |
|------|------|
| Arrange | arr[0]; 에 해당하는 토큰 벡터 수동 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `ExpressionStmt → ArrayAccessExpr`, array == VariableExpr("arr"), index == 0.0 |

**구현 상태** : 완료

---

### TC-AR-03 — ExecutesArrayAccess

**목적**
배열 선언 후 인덱스 접근 시 올바른 요소값이 출력되는지 확인

**시나리오**
```
var arr = [10, 20, 30];
print arr[1];
```

**기대 결과**
- stdout == `"20\n"`

| 단계 | 내용 |
|------|------|
| Arrange | VarDeclareStmt(arr, ArrayLiteralExpr([10,20,30])), PrintStmt(ArrayAccessExpr(arr, 1)) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | stdout == `"20\n"` |

**구현 상태** : 완료

---

### TC-AR-04 — ArrayAccessOutOfBounds

**목적**
배열 범위를 초과하는 인덱스 접근 시 `std::runtime_error`가 발생하는지 확인

**시나리오**
```
var arr = [1, 2];
print arr[5];
```

**기대 결과**
- `std::runtime_error("Array index out of bounds")` 발생

| 단계 | 내용 |
|------|------|
| Arrange | VarDeclareStmt(arr, ArrayLiteralExpr([1,2])), PrintStmt(ArrayAccessExpr(arr, 5)) 수동 구성 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-AR-05 — ArrayCreation

**목적**
`Array(n)` 으로 고정 크기 배열을 생성하면 초기값이 `null` 이고 인덱스 접근이 가능한지 확인

**시나리오**
```
var arr = Array(3);
print arr[0];
```

**기대 결과**
- stdout == `"null\n"`

| 단계 | 내용 |
|------|------|
| Arrange | `runSource("var arr = Array(3); print arr[0];")` |
| Act | 소스 문자열 파싱·실행 |
| Assert | stdout == `"null\n"` |

**구현 상태** : 완료

---

### TC-AR-06 — ArrayWrite

**목적**
`arr[i] = val` 로 요소를 쓴 뒤 읽으면 올바른 값이 반환되는지 확인

**시나리오**
```
var arr = Array(3);
arr[1] = 20;
print arr[1];
```

**기대 결과**
- stdout == `"20\n"`

| 단계 | 내용 |
|------|------|
| Arrange | `runSource(...)` |
| Act | 소스 문자열 파싱·실행 |
| Assert | stdout == `"20\n"` |

**구현 상태** : 완료

---

### TC-AR-07 — ArrayIndexNotNumber

**목적**
배열 인덱스에 숫자가 아닌 값을 전달하면 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
var arr = Array(3);
print arr["hello"];
```

**기대 결과**
- `std::runtime_error` 발생

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-AR-08 — IndexOnNonArray

**목적**
배열이 아닌 변수에 `[]` 접근 시 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
var x = 10;
print x[0];
```

**기대 결과**
- `std::runtime_error` 발생

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-AR-09 — ArraySizeNotNumber

**목적**
`Array()` 크기 인자에 숫자가 아닌 값을 전달하면 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
var arr = Array("hi");
```

**기대 결과**
- `std::runtime_error` 발생

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-AR-10 — NegativeIndexThrows _(global: A-23)_

**목적**
음수 인덱스로 배열 접근 시 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
var arr = Array(3);
print arr[-1];
```

**기대 결과**
- `std::runtime_error` 발생 (`idx < 0` 조건)

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-AR-11 — NegativeArraySizeThrows _(global: A-24)_

**목적**
`Array()` 크기에 음수를 전달하면 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
var arr = Array(-1);
```

**기대 결과**
- `std::runtime_error` 발생 (`size < 0` 조건)

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료

---

### TC-AR-12 — ZeroSizeArrayAccessThrows _(global: A-25)_

**목적**
크기 0 배열의 첫 번째 요소에 접근하면 `std::runtime_error` 가 발생하는지 확인

**시나리오**
```
var arr = Array(0);
print arr[0];
```

**기대 결과**
- `std::runtime_error` 발생 (`idx >= elements.size()` 조건)

| 단계 | 내용 |
|------|------|
| Arrange | 소스 문자열 토크나이즈·파싱 |
| Act | `executor.execute(stmts)` 호출 |
| Assert | `ASSERT_THROW(..., std::runtime_error)` |

**구현 상태** : 완료
