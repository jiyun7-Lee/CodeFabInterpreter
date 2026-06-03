# TC_ExprParser — Expression Parser 테스트 케이스 명세

담당자: 김홍원 (Hongwon Kim)  
파일: `tests/ExpressionParserTest.cpp`  
프레임워크: Google Test (gtest) / Google Mock (gmock)

---

## 테스트 전략

- Parser의 public 인터페이스 `parse(tokens)` 를 통해 간접 검증
- 입력: 수동으로 구성한 `std::vector<Token>`
- 출력: `ExpressionStmt`로 래핑된 `Expr*` 를 `dynamic_cast` 로 검증
- 각 TC는 **Arrange → Act → Assert** 패턴으로 구성
- 현재 상태: **Green** (TC-01~13 전체 통과)

---

## TC 목록

| ID | 테스트 이름 | 입력 코드 | 검증 Expr 타입 | 상태 |
|---|---|---|---|---|
| TC-01 | ParsesNumberLiteral | `42;` | LiteralExpr | 🟢 Green |
| TC-02 | ParsesVariableExpr | `a;` | VariableExpr | 🟢 Green |
| TC-03 | RespectsMulBeforeAdd | `1 + 2 * 3;` | BinaryExpr (우선순위) | 🟢 Green |
| TC-04 | GroupingOverridesPrecedence | `(1 + 2) * 3;` | GroupingExpr | 🟢 Green |
| TC-05 | ParsesAssignExpr | `a = 10;` | AssignExpr | 🟢 Green |
| TC-06 | ParsesStringLiteral | `"hello";` | LiteralExpr (문자열) | 🟢 Green |
| TC-07 | ParsesBooleanTrue | `true;` | LiteralExpr (bool) | 🟢 Green |
| TC-08 | ParsesBooleanFalse | `false;` | LiteralExpr (bool) | 🟢 Green |
| TC-09 | ParsesUnaryMinus | `-5;` | UnaryExpr | 🟢 Green |
| TC-10 | ParsesComparisonGreater | `a > 3;` | BinaryExpr (비교) | 🟢 Green |
| TC-11 | ParsesSubtraction | `5 - 3;` | BinaryExpr (뺄셈) | 🟢 Green |
| TC-12 | ParsesLogicalAnd | `a and b;` | BinaryExpr (논리) | 🟢 Green |
| TC-13 | AssignIsRightAssociative | `a = b = 3;` | AssignExpr (우결합) | 🟢 Green |

---

## TC 상세

### TC-01 ParsesNumberLiteral

**목적**: 숫자 리터럴이 LiteralExpr로 파싱되는지 확인

**입력 토큰**
```
NUMBER("42", 42.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── LiteralExpr
    └── value: 42.0
```

| 단계 | 내용 |
|---|---|
| Arrange | 숫자 리터럴 42 하나짜리 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `expression`이 `LiteralExpr`이고 `value` == `42.0` |

---

### TC-02 ParsesVariableExpr

**목적**: 식별자(변수명)가 VariableExpr로 파싱되는지 확인

**입력 토큰**
```
IDENTIFIER("a") → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── VariableExpr
    └── name.lexeme: "a"
```

| 단계 | 내용 |
|---|---|
| Arrange | 식별자 `"a"` 하나짜리 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `expression`이 `VariableExpr`이고 `name.lexeme` == `"a"` |

---

### TC-03 RespectsMulBeforeAdd

**목적**: `*` 가 `+` 보다 높은 우선순위를 갖는지 AST 구조로 검증

**입력 토큰**
```
NUMBER("1", 1.0) → PLUS → NUMBER("2", 2.0) → STAR → NUMBER("3", 3.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── BinaryExpr(PLUS)
    ├── left:  LiteralExpr(1.0)
    └── right: BinaryExpr(STAR)
               ├── left:  LiteralExpr(2.0)
               └── right: LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"1 + 2 * 3"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | 루트가 `PLUS`, 우측 자식이 `STAR`인 트리 구조 확인 (우선순위 검증) |

---

### TC-04 GroupingOverridesPrecedence

**목적**: 괄호 `()` 로 연산 우선순위를 변경할 수 있는지 검증

**입력 토큰**
```
LEFT_PAREN → NUMBER("1", 1.0) → PLUS → NUMBER("2", 2.0) → RIGHT_PAREN → STAR → NUMBER("3", 3.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── BinaryExpr(STAR)
    ├── left:  GroupingExpr
    │          └── BinaryExpr(PLUS)
    │              ├── left:  LiteralExpr(1.0)
    │              └── right: LiteralExpr(2.0)
    └── right: LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"(1 + 2) * 3"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | 루트가 `STAR`, 좌측 자식이 `GroupingExpr(PLUS)`인지 확인 (괄호 우선순위 역전 검증) |

---

### TC-05 ParsesAssignExpr

**목적**: 변수 대입식이 AssignExpr로 파싱되는지 확인

**입력 토큰**
```
IDENTIFIER("a") → EQUAL → NUMBER("10", 10.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── AssignExpr
    ├── name.lexeme: "a"
    └── value: LiteralExpr(10.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"a = 10"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `expression`이 `AssignExpr`이고 `name.lexeme` == `"a"`, `value` == `10.0` |

---

### TC-06 ParsesStringLiteral

**목적**: 문자열 리터럴이 LiteralExpr 로 파싱되는지 확인

**입력 토큰**
```
STRING("hello") → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── LiteralExpr
    └── value: "hello" (std::string)
```

| 단계 | 내용 |
|---|---|
| Arrange | 문자열 리터럴 토큰 시퀀스 구성 (literal 필드에 std::string 저장) |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `LiteralExpr` 이고 `value` == `"hello"` |

---

### TC-07 ParsesBooleanTrue

**목적**: `true` 키워드가 불리언 LiteralExpr 로 파싱되는지 확인

**입력 토큰**
```
TRUE("true") → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── LiteralExpr
    └── value: true (bool)
```

| 단계 | 내용 |
|---|---|
| Arrange | TRUE 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `LiteralExpr` 이고 `value` == `true` |

---

### TC-08 ParsesBooleanFalse

**목적**: `false` 키워드가 불리언 LiteralExpr 로 파싱되는지 확인

**입력 토큰**
```
FALSE("false") → SEMICOLON → EOF
```

| 단계 | 내용 |
|---|---|
| Arrange | FALSE 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `LiteralExpr` 이고 `value` == `false` |

---

### TC-09 ParsesUnaryMinus

**목적**: 단항 음수 `-5` 가 UnaryExpr 로 파싱되는지 확인

**입력 토큰**
```
MINUS("-") → NUMBER("5", 5.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── UnaryExpr(MINUS)
    └── right: LiteralExpr(5.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | 단항 음수 `-5` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `UnaryExpr` 이고 `op.type` == `MINUS`, `right` == `LiteralExpr(5.0)` |

---

### TC-10 ParsesComparisonGreater

**목적**: 비교 연산자 `>` 가 BinaryExpr 로 파싱되는지 확인

**입력 토큰**
```
IDENTIFIER("a") → GREATER(">") → NUMBER("3", 3.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── BinaryExpr(GREATER)
    ├── left:  VariableExpr(a)
    └── right: LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"a > 3"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `BinaryExpr` 이고 `op.type` == `GREATER`, 좌우 피연산자 확인 |

---

### TC-11 ParsesSubtraction

**목적**: 뺄셈 연산이 BinaryExpr 로 파싱되는지 확인

**입력 토큰**
```
NUMBER("5", 5.0) → MINUS("-") → NUMBER("3", 3.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── BinaryExpr(MINUS)
    ├── left:  LiteralExpr(5.0)
    └── right: LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"5 - 3"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `BinaryExpr` 이고 `op.type` == `MINUS`, 좌우 피연산자 확인 |

---

### TC-12 ParsesLogicalAnd

**목적**: 논리 `and` 연산이 BinaryExpr 로 파싱되는지 확인

**입력 토큰**
```
IDENTIFIER("a") → AND("and") → IDENTIFIER("b") → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── BinaryExpr(AND)
    ├── left:  VariableExpr(a)
    └── right: VariableExpr(b)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"a and b"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `BinaryExpr` 이고 `op.type` == `AND`, 좌우 피연산자 확인 |

---

### TC-13 AssignIsRightAssociative

**목적**: 대입 연산자가 우결합으로 파싱되는지 확인 (`a = (b = 3)`)

**입력 토큰**
```
IDENTIFIER("a") → EQUAL → IDENTIFIER("b") → EQUAL → NUMBER("3", 3.0) → SEMICOLON → EOF
```

**기대 AST**
```
ExpressionStmt
└── AssignExpr(a)
    └── value: AssignExpr(b)
               └── value: LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `"a = b = 3"` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | 외부 `AssignExpr(a)` 내부에 `AssignExpr(b, LiteralExpr(3))` 중첩 확인 |

---

## 추가 예정 TC (논의 후 결정)

| ID | 설명 | 입력 예시 | 비고 |
|---|---|---|---|
| TC-14 | UnaryExpr 논리 부정 | `!true;` | Token.h 에 `!` 토큰 추가 필요 — 팀 협의 필요 |
| TC-15 | BinaryExpr 나눗셈 | `6 / 2;` | |
| TC-16 | or 논리 연산 | `a or b;` | |
| TC-17 | 좌결합 검증 | `1 + 2 + 3;` | BinaryExpr(PLUS, BinaryExpr(PLUS,1,2), 3) |
