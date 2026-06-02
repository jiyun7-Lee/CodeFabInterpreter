# TC_ExprParser — Expression Parser 테스트 케이스 명세

담당자: 김홍원 (Hongwon Kim)  
파일: `tests/ExcutorParserTest.cpp`  
프레임워크: Google Test (gtest) / Google Mock (gmock)

---

## 테스트 전략

- Parser의 public 인터페이스 `parse(tokens)` 를 통해 간접 검증
- 입력: 수동으로 구성한 `std::vector<Token>`
- 출력: `ExpressionStmt`로 래핑된 `Expr*` 를 `dynamic_cast` 로 검증
- 각 TC는 **Arrange → Act → Assert** 패턴으로 구성
- 현재 상태: **Red** (Parser.cpp 미구현 — stub)

---

## TC 목록

| ID | 테스트 이름 | 입력 코드 | 검증 Expr 타입 | 상태 |
|---|---|---|---|---|
| TC-01 | ParsesNumberLiteral | `42;` | LiteralExpr | 🔴 Red |
| TC-02 | ParsesVariableExpr | `a;` | VariableExpr | 🔴 Red |
| TC-03 | RespectsMulBeforeAdd | `1 + 2 * 3;` | BinaryExpr (우선순위) | 🔴 Red |
| TC-04 | GroupingOverridesPrecedence | `(1 + 2) * 3;` | GroupingExpr | 🔴 Red |
| TC-05 | ParsesAssignExpr | `a = 10;` | AssignExpr | 🔴 Red |

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

## 추가 예정 TC (논의 후 결정)

| ID | 설명 | 입력 예시 |
|---|---|---|
| TC-06 | UnaryExpr 음수 | `-5;` |
| TC-07 | UnaryExpr 논리 부정 | `!true;` |
| TC-08 | LogicalExpr and/or | `a and b;` |
| TC-09 | 중첩 BinaryExpr | `1 + 2 + 3;` (좌결합) |
| TC-10 | 문자열 리터럴 | `"hello";` |
