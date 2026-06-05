# Array 테스트 케이스 명세서

## 대상 파일

`tests/ArrayTest.cpp`

---

## TC 목록

| TC ID   | 테스트 함수명            | 구현 상태  |
|---------|------------------------|----------|
| TC-AR-01 | ParsesArrayLiteral     | 🟢 Green  |
| TC-AR-02 | ParsesArrayAccess      | 🟢 Green  |
| TC-AR-03 | ExecutesArrayAccess    | 🟢 Green  |
| TC-AR-04 | ArrayAccessOutOfBounds | 🟢 Green  |

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
