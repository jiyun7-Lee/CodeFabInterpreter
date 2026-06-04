# TC_StmtParserAndChecker — Statement Parser + Checker 테스트 케이스 명세

담당자: 심민구 (Mingu Sim)  
파일: `tests/ParserTest.cpp`, `tests/CheckerTest.cpp`  
프레임워크: Google Test (gtest) / Google Mock (gmock)

---

## 테스트 전략

- Parser: `parse(tokens)` 를 통해 반환된 `Stmt*` 트리를 `dynamic_cast` 로 검증
- Checker: `check(stmts)` 반환값(bool) + `getErrors()` 크기로 검증
- 표현식 서브트리는 **FakeExprParser** 가 토큰 1개를 소비해 `LiteralExpr` 반환 (B파트 독립성 확보)
- 각 TC는 **Arrange → Act → Assert** 패턴으로 구성
- 현재 상태: **🔴 Red** (Parser.cpp / Checker.cpp 미구현 상태)

---

## TC 목록 — Statement Parser

| ID | 테스트 이름 | 입력 코드 | 검증 Stmt 타입 | 상태 |
|---|---|---|---|---|
| P-TC-01 | VarWithNumber | `var a = 3;` | VarDeclareStmt | 🔴 Red |
| P-TC-02 | VarWithString | `var abc = "hello";` | VarDeclareStmt | 🔴 Red |
| P-TC-03 | VarWithBool | `var flag = true;` | VarDeclareStmt | 🔴 Red |
| P-TC-04 | VarWithExpr | `var a = b + 1;` | VarDeclareStmt | 🔴 Red |
| P-TC-05 | VarMissingSemicolon | `var a = 3` (`;` 없음) | runtime_error | 🔴 Red |
| P-TC-06 | PrintVariable | `print a;` | PrintStmt | 🔴 Red |
| P-TC-07 | PrintExpression | `print a + b;` | PrintStmt | 🔴 Red |
| P-TC-08 | PrintMissingSemicolon | `print a` (`;` 없음) | runtime_error | 🔴 Red |
| P-TC-09 | EmptyBlock | `{}` | BlockStmt (0개) | 🔴 Red |
| P-TC-10 | BlockOneStmt | `{ print a; }` | BlockStmt (1개) | 🔴 Red |
| P-TC-11 | BlockMultipleStmts | `{ print a; print b; }` | BlockStmt (2개) | 🔴 Red |
| P-TC-12 | NestedBlock | `{ { print a; } }` | BlockStmt 중첩 | 🔴 Red |
| P-TC-13 | BlockMissingClosingBrace | `{ print a;` (`}` 없음) | runtime_error | 🔴 Red |
| P-TC-14 | IfWithoutElse | `if (a > 0) print a;` | IfStmt (else 없음) | 🔴 Red |
| P-TC-15 | IfWithElse | `if (a > 0) print a; else print b;` | IfStmt (else 있음) | 🔴 Red |
| P-TC-16 | IfWithBlockBody | `if (a > 0) { print a; }` | thenBranch == BlockStmt | 🔴 Red |
| P-TC-17 | IfElseBothBlocks | `if (a > 0) { } else { }` | 양 분기 BlockStmt | 🔴 Red |
| P-TC-18 | NestedIf | `if (a > 0) if (b > 0) print b;` | IfStmt 중첩 | 🔴 Red |
| P-TC-19 | IfMissingLeftParen | `if a > 0) print a;` | runtime_error | 🔴 Red |
| P-TC-20 | IfMissingRightParen | `if (a > 0 print a;` | runtime_error | 🔴 Red |
| P-TC-21 | ForAllParts | `for (var i=0; i<3; i=i+1) print i;` | ForStmt (전 필드 ≠ null) | 🔴 Red |
| P-TC-22 | ForWithBlockBody | `for (...) { print i; }` | body == BlockStmt | 🔴 Red |
| P-TC-23 | ForMissingSemicolon | condition 뒤 `;` 없음 | runtime_error | 🔴 Red |
| P-TC-24 | ForWithEmptyBlock | `for (...) {}` | body == 빈 BlockStmt | 🔴 Red |
| P-TC-25 | MultipleStatements | `var a = 1; print a;` | stmts.size() == 2 | 🔴 Red |
| P-TC-26 | EmptyInput | EOF만 | stmts.size() == 0 | 🔴 Red |

## TC 목록 — Checker

| ID | 테스트 이름 | 입력 코드 | 기대 결과 | 상태 |
|---|---|---|---|---|
| C-TC-01 | DuplicateVarSameBlock | `{ var a = 1; var a = 2; }` | false (오류) | 🔴 Red |
| C-TC-02 | DuplicateVarGlobal | `var a = 1; var a = 2;` | false (오류) | 🔴 Red |
| C-TC-03 | ShadowingAllowed | `var a = 1; { var a = 2; }` | true (OK) | 🟢 Green |
| C-TC-04 | SameNameDifferentBlocks | `{ var a = 1; } { var a = 2; }` | true (OK) | 🟢 Green |
| C-TC-05 | DifferentVarsSameBlock | `{ var a = 1; var b = 2; }` | true (OK) | 🟢 Green |
| C-TC-06 | SelfReference | `var a = a;` | false (오류) | 🔴 Red |
| C-TC-07 | SelfReferenceInBinary | `var a = a + 1;` | false (오류) | 🔴 Red |
| C-TC-08 | ReferenceAlreadyDeclared | `var a = 1; var b = a;` | true (OK) | 🟢 Green |
| C-TC-09 | ReferenceOtherVar | `var b = 1; var a = b;` | true (OK) | 🟢 Green |
| C-TC-10 | EmptyAST | `(빈 입력)` | true (OK) | 🟢 Green |
| C-TC-11 | MultipleDistinctVars | `var a = 1; var b = 2;` | true (OK) | 🟢 Green |
| C-TC-12 | VarInsideIfBlock | `if (true) { var x = 1; }` | true (OK) | 🟢 Green |

> Checker 🟢 Green 항목은 현재 stub(`return true`) 상태에서 우연히 통과 중 — 구현 후에도 유지돼야 함

---

## TC 상세 — Statement Parser

### P-TC-01 VarWithNumber

**목적**: 숫자 초기값을 갖는 변수 선언이 `VarDeclareStmt` 로 파싱되는지 확인

**입력 토큰**
```
VAR("var") → IDENTIFIER("a") → EQUAL("=") → NUMBER("3", 3.0) → SEMICOLON → EOF
```

**기대 AST**
```
VarDeclareStmt
├── name.lexeme: "a"
└── initializer: LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = 3;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | stmts[0]이 `VarDeclareStmt`, `name.lexeme` == `"a"`, `initializer` == `LiteralExpr(3.0)` |

---

### P-TC-02 VarWithString

**목적**: 문자열 초기값을 갖는 변수 선언이 파싱되는지 확인

**입력 토큰**
```
VAR → IDENTIFIER("abc") → EQUAL → STRING("hello") → SEMICOLON → EOF
```

**기대 AST**
```
VarDeclareStmt
├── name.lexeme: "abc"
└── initializer: LiteralExpr("hello")
```

| 단계 | 내용 |
|---|---|
| Arrange | `var abc = "hello";` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `name.lexeme` == `"abc"`, `initializer`의 value == `"hello"` |

---

### P-TC-03 VarWithBool

**목적**: 불리언 초기값을 갖는 변수 선언이 파싱되는지 확인

**입력 토큰**
```
VAR → IDENTIFIER("flag") → EQUAL → TRUE("true") → SEMICOLON → EOF
```

**기대 AST**
```
VarDeclareStmt
├── name.lexeme: "flag"
└── initializer: LiteralExpr(true)
```

| 단계 | 내용 |
|---|---|
| Arrange | `var flag = true;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `initializer`의 value == `true` (bool) |

---

### P-TC-04 VarWithExpr

**목적**: 표현식 초기값을 갖는 변수 선언 시 initializer가 null이 아닌지 확인

**입력 토큰**
```
VAR → IDENTIFIER("a") → EQUAL → IDENTIFIER("b") → SEMICOLON → EOF
```

**기대 AST**
```
VarDeclareStmt
├── name.lexeme: "a"
└── initializer: (non-null Expr*)
```

> FakeExprParser가 `IDENTIFIER("b")` 1개 소비 후 LiteralExpr 반환

| 단계 | 내용 |
|---|---|
| Arrange | `var a = b + 1;` 형태의 토큰 시퀀스 구성 (FakeExprParser는 IDENTIFIER 소비) |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `initializer != nullptr` |

---

### P-TC-05 VarMissingSemicolon

**목적**: 세미콜론 누락 시 `std::runtime_error` 가 발생하는지 확인

**입력 토큰**
```
VAR → IDENTIFIER("a") → EQUAL → NUMBER("3", 3.0) → EOF  (SEMICOLON 없음)
```

| 단계 | 내용 |
|---|---|
| Arrange | 세미콜론 없는 `var a = 3` 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `std::runtime_error` throw 확인 |

---

### P-TC-06 PrintVariable

**목적**: 변수 출력 문장이 `PrintStmt` 로 파싱되는지 확인

**입력 토큰**
```
PRINT("print") → IDENTIFIER("a") → SEMICOLON → EOF
```

**기대 AST**
```
PrintStmt
└── expression: (non-null Expr*)
```

| 단계 | 내용 |
|---|---|
| Arrange | `print a;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | stmts[0]이 `PrintStmt`, `expression != nullptr` |

---

### P-TC-07 PrintExpression

**목적**: 표현식 출력 문장이 `PrintStmt` 로 파싱되는지 확인

**입력 토큰**
```
PRINT → IDENTIFIER("a") → SEMICOLON → EOF
```

| 단계 | 내용 |
|---|---|
| Arrange | `print a + b;` 형태의 토큰 시퀀스 구성 (FakeExprParser는 첫 IDENTIFIER 소비) |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `PrintStmt`, `expression != nullptr` |

---

### P-TC-08 PrintMissingSemicolon

**목적**: 세미콜론 누락 시 `std::runtime_error` 가 발생하는지 확인

**입력 토큰**
```
PRINT → IDENTIFIER("a") → EOF  (SEMICOLON 없음)
```

| 단계 | 내용 |
|---|---|
| Arrange | 세미콜론 없는 `print a` 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `std::runtime_error` throw 확인 |

---

### P-TC-09 EmptyBlock

**목적**: 빈 블록 `{}` 이 statements가 0개인 `BlockStmt` 로 파싱되는지 확인

**입력 토큰**
```
LEFT_BRACE("{") → RIGHT_BRACE("}") → EOF
```

**기대 AST**
```
BlockStmt
└── statements: [] (size == 0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `{}` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | stmts[0]이 `BlockStmt`, `statements.size()` == 0 |

---

### P-TC-10 BlockOneStmt

**목적**: 문장 1개를 포함한 블록이 올바르게 파싱되는지 확인

**입력 토큰**
```
LEFT_BRACE → PRINT → IDENTIFIER("a") → SEMICOLON → RIGHT_BRACE → EOF
```

**기대 AST**
```
BlockStmt
└── statements: [PrintStmt]  (size == 1)
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ print a; }` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `statements.size()` == 1 |

---

### P-TC-11 BlockMultipleStmts

**목적**: 문장 여러 개를 포함한 블록이 올바르게 파싱되는지 확인

**입력 토큰**
```
LEFT_BRACE → PRINT("a") → SEMICOLON → PRINT("b") → SEMICOLON → RIGHT_BRACE → EOF
```

**기대 AST**
```
BlockStmt
└── statements: [PrintStmt, PrintStmt]  (size == 2)
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ print a; print b; }` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `statements.size()` == 2 |

---

### P-TC-12 NestedBlock

**목적**: 중첩 블록이 올바르게 파싱되는지 확인

**입력 토큰**
```
LEFT_BRACE → LEFT_BRACE → PRINT("a") → SEMICOLON → RIGHT_BRACE → RIGHT_BRACE → EOF
```

**기대 AST**
```
BlockStmt (outer)
└── statements[0]: BlockStmt (inner)
                   └── statements[0]: PrintStmt
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ { print a; } }` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | outer.statements[0]이 `BlockStmt` |

---

### P-TC-13 BlockMissingClosingBrace

**목적**: 닫는 중괄호 누락 시 `std::runtime_error` 가 발생하는지 확인

**입력 토큰**
```
LEFT_BRACE → PRINT("a") → SEMICOLON → EOF  (RIGHT_BRACE 없음)
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ print a;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `std::runtime_error` throw 확인 |

---

### P-TC-14 IfWithoutElse

**목적**: else 없는 if 문이 `elseBranch == nullptr` 인 `IfStmt` 로 파싱되는지 확인

**입력 토큰**
```
IF → LEFT_PAREN → IDENTIFIER("a") → RIGHT_PAREN → PRINT("a") → SEMICOLON → EOF
```

**기대 AST**
```
IfStmt
├── condition:  (non-null Expr*)
├── thenBranch: PrintStmt
└── elseBranch: nullptr
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (a > 0) print a;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `condition != nullptr`, `thenBranch != nullptr`, `elseBranch == nullptr` |

---

### P-TC-15 IfWithElse

**목적**: else 분기를 포함한 if-else 문이 `elseBranch != nullptr` 인 `IfStmt` 로 파싱되는지 확인

**입력 토큰**
```
IF → LEFT_PAREN → IDENTIFIER("a") → RIGHT_PAREN
  → PRINT("a") → SEMICOLON
  → ELSE → PRINT("b") → SEMICOLON → EOF
```

**기대 AST**
```
IfStmt
├── condition:  (non-null Expr*)
├── thenBranch: PrintStmt("a")
└── elseBranch: PrintStmt("b")
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (a > 0) print a; else print b;` 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `elseBranch != nullptr` |

---

### P-TC-16 IfWithBlockBody

**목적**: 블록 몸체를 갖는 if 문에서 `thenBranch` 가 `BlockStmt` 인지 확인

**입력 토큰**
```
IF → LEFT_PAREN → IDENTIFIER("a") → RIGHT_PAREN
  → LEFT_BRACE → PRINT("a") → SEMICOLON → RIGHT_BRACE → EOF
```

**기대 AST**
```
IfStmt
├── condition:  (non-null Expr*)
└── thenBranch: BlockStmt
                └── [PrintStmt]
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (a > 0) { print a; }` 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `thenBranch`가 `BlockStmt` |

---

### P-TC-17 IfElseBothBlocks

**목적**: if-else 양 분기가 모두 `BlockStmt` 로 파싱되는지 확인

**입력 토큰**
```
IF → LEFT_PAREN → IDENTIFIER("a") → RIGHT_PAREN
  → LEFT_BRACE → RIGHT_BRACE
  → ELSE → LEFT_BRACE → RIGHT_BRACE → EOF
```

**기대 AST**
```
IfStmt
├── thenBranch: BlockStmt (빈 블록)
└── elseBranch: BlockStmt (빈 블록)
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (a > 0) { } else { }` 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `thenBranch`, `elseBranch` 모두 `BlockStmt` |

---

### P-TC-18 NestedIf

**목적**: 중첩 if 문에서 외부 IfStmt의 thenBranch가 IfStmt인지 확인

**입력 토큰**
```
IF → LEFT_PAREN("a") → RIGHT_PAREN
  → IF → LEFT_PAREN("b") → RIGHT_PAREN
    → PRINT("b") → SEMICOLON → EOF
```

**기대 AST**
```
IfStmt (outer)
└── thenBranch: IfStmt (inner)
                └── thenBranch: PrintStmt
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (a > 0) if (b > 0) print b;` 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | outer.thenBranch가 `IfStmt` |

---

### P-TC-19 IfMissingLeftParen

**목적**: `if` 뒤 여는 괄호 누락 시 `std::runtime_error` 발생 확인

**입력 토큰**
```
IF → IDENTIFIER("a") → RIGHT_PAREN → PRINT("a") → SEMICOLON → EOF  (LEFT_PAREN 없음)
```

| 단계 | 내용 |
|---|---|
| Arrange | `if a > 0) print a;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `std::runtime_error` throw 확인 |

---

### P-TC-20 IfMissingRightParen

**목적**: 조건식 뒤 닫는 괄호 누락 시 `std::runtime_error` 발생 확인

**입력 토큰**
```
IF → LEFT_PAREN → IDENTIFIER("a") → PRINT("a") → SEMICOLON → EOF  (RIGHT_PAREN 없음)
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (a > 0 print a;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `std::runtime_error` throw 확인 |

---

### P-TC-21 ForAllParts

**목적**: for 문의 init / condition / increment / body 가 모두 비어있지 않은지 확인

**입력 토큰**
```
FOR → LEFT_PAREN
  → VAR("i") → EQUAL → NUMBER(0.0) → SEMICOLON   (init: VarDeclareStmt)
  → IDENTIFIER("i") → SEMICOLON                   (condition)
  → IDENTIFIER("i") → RIGHT_PAREN                 (increment)
  → PRINT("i") → SEMICOLON → EOF                  (body)
```

**기대 AST**
```
ForStmt
├── init:      VarDeclareStmt(i = 0)
├── condition: (non-null Expr*)
├── increment: (non-null Expr*)
└── body:      PrintStmt
```

| 단계 | 내용 |
|---|---|
| Arrange | `for (var i = 0; i < 3; i = i+1) print i;` 형태의 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `init`, `condition`, `increment`, `body` 모두 `!= nullptr` |

---

### P-TC-22 ForWithBlockBody

**목적**: 블록 몸체를 갖는 for 문에서 body가 `BlockStmt` 인지 확인

**입력 토큰**
```
FOR → LEFT_PAREN → (init) → SEMICOLON → (cond) → SEMICOLON → (incr) → RIGHT_PAREN
  → LEFT_BRACE → PRINT("i") → SEMICOLON → RIGHT_BRACE → EOF
```

**기대 AST**
```
ForStmt
└── body: BlockStmt
          └── [PrintStmt]
```

| 단계 | 내용 |
|---|---|
| Arrange | `for (...) { print i; }` 형태의 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `body`가 `BlockStmt` |

---

### P-TC-23 ForMissingSemicolon

**목적**: condition 뒤 세미콜론 누락 시 `std::runtime_error` 발생 확인

**입력 토큰**
```
FOR → LEFT_PAREN → (init) → SEMICOLON → IDENTIFIER("i")
  → IDENTIFIER("i") → RIGHT_PAREN → EOF  (condition 뒤 SEMICOLON 없음)
```

| 단계 | 내용 |
|---|---|
| Arrange | condition 뒤 `;` 없는 for 문 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `std::runtime_error` throw 확인 |

---

### P-TC-24 ForWithEmptyBlock

**목적**: 빈 블록 몸체를 갖는 for 문에서 body가 statements 0개인 `BlockStmt` 인지 확인

**입력 토큰**
```
FOR → LEFT_PAREN → (init) → SEMICOLON → (cond) → SEMICOLON → (incr) → RIGHT_PAREN
  → LEFT_BRACE → RIGHT_BRACE → EOF
```

**기대 AST**
```
ForStmt
└── body: BlockStmt (statements.size() == 0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `for (...) {}` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `body`가 `BlockStmt`, `statements.size()` == 0 |

---

### P-TC-25 MultipleStatements

**목적**: 여러 문장이 순서대로 파싱되는지 확인

**입력 토큰**
```
VAR("a") → EQUAL → NUMBER(1.0) → SEMICOLON
→ PRINT("a") → SEMICOLON → EOF
```

**기대 AST**
```
[VarDeclareStmt, PrintStmt]  (size == 2)
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = 1; print a;` 에 해당하는 토큰 시퀀스 구성 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `stmts.size()` == 2 |

---

### P-TC-26 EmptyInput

**목적**: EOF만 있는 입력에서 빈 문장 목록이 반환되는지 확인

**입력 토큰**
```
EOF
```

| 단계 | 내용 |
|---|---|
| Arrange | EOF 토큰만으로 구성된 토큰 시퀀스 |
| Act | `parser.parse(tokens)` 호출 |
| Assert | `stmts.size()` == 0 |

---

## TC 상세 — Checker

### C-TC-01 DuplicateVarSameBlock

**목적**: 같은 블록 내 동일 이름 변수 선언 시 오류를 감지하는지 확인

**입력 AST**
```
BlockStmt
├── VarDeclareStmt(a = 1.0)
└── VarDeclareStmt(a = 2.0)   ← 중복
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ var a = 1; var a = 2; }` 에 해당하는 AST 직접 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `false` |

---

### C-TC-02 DuplicateVarGlobal

**목적**: 글로벌 스코프에서 동일 이름 변수 재선언 시 오류를 감지하는지 확인

**입력 AST**
```
[VarDeclareStmt(a = 1.0), VarDeclareStmt(a = 2.0)]
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = 1; var a = 2;` 에 해당하는 AST 직접 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `false` |

---

### C-TC-03 ShadowingAllowed

**목적**: 내부 블록에서의 같은 이름 선언(shadowing)이 허용되는지 확인

**입력 AST**
```
[
  VarDeclareStmt(a = 1.0),          ← 외부 스코프
  BlockStmt
  └── VarDeclareStmt(a = 2.0)       ← 내부 스코프 (shadowing)
]
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = 1; { var a = 2; }` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-04 SameNameDifferentBlocks

**목적**: 서로 다른 형제 블록에서의 같은 이름 선언이 허용되는지 확인

**입력 AST**
```
[
  BlockStmt → VarDeclareStmt(a),   ← 블록 1
  BlockStmt → VarDeclareStmt(a)    ← 블록 2 (별도 스코프)
]
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ var a = 1; } { var a = 2; }` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-05 DifferentVarsSameBlock

**목적**: 같은 블록 내 서로 다른 이름의 변수 선언이 정상 처리되는지 확인

**입력 AST**
```
BlockStmt
├── VarDeclareStmt(a = 1.0)
└── VarDeclareStmt(b = 2.0)   ← 다른 이름
```

| 단계 | 내용 |
|---|---|
| Arrange | `{ var a = 1; var b = 2; }` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-06 SelfReference

**목적**: 선언 중인 변수를 초기식에서 자기 참조하는 경우 오류를 감지하는지 확인

**입력 AST**
```
VarDeclareStmt
├── name: "a"
└── initializer: VariableExpr("a")   ← 자기 참조
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = a;` 에 해당하는 AST 직접 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `false` |

---

### C-TC-07 SelfReferenceInBinary

**목적**: 이항 표현식 내부에 자기 참조가 있는 경우 오류를 감지하는지 확인

**입력 AST**
```
VarDeclareStmt
├── name: "a"
└── initializer: BinaryExpr(PLUS)
                 ├── left:  VariableExpr("a")   ← 자기 참조
                 └── right: LiteralExpr(1.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = a + 1;` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `false` |

---

### C-TC-08 ReferenceAlreadyDeclared

**목적**: 이미 선언된 다른 변수를 초기식에서 참조하는 경우 정상 처리되는지 확인

**입력 AST**
```
[
  VarDeclareStmt(a = LiteralExpr(1.0)),
  VarDeclareStmt(b = VariableExpr("a"))   ← "a"는 이미 선언됨
]
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = 1; var b = a;` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-09 ReferenceOtherVar

**목적**: 다른 변수를 참조하는 초기식이 자기 참조로 오판되지 않는지 확인

**입력 AST**
```
[
  VarDeclareStmt(b = LiteralExpr(1.0)),
  VarDeclareStmt(a = VariableExpr("b"))   ← "b" 참조 (자기 참조 아님)
]
```

| 단계 | 내용 |
|---|---|
| Arrange | `var b = 1; var a = b;` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-10 EmptyAST

**목적**: 빈 입력에서 오류 없이 true 를 반환하는지 확인

**입력 AST**
```
[]
```

| 단계 | 내용 |
|---|---|
| Arrange | 빈 벡터 |
| Act | `checker.check({})` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-11 MultipleDistinctVars

**목적**: 이름이 다른 여러 변수 선언이 정상 처리되는지 확인

**입력 AST**
```
[VarDeclareStmt(a = 1.0), VarDeclareStmt(b = 2.0)]
```

| 단계 | 내용 |
|---|---|
| Arrange | `var a = 1; var b = 2;` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

### C-TC-12 VarInsideIfBlock

**목적**: if 블록 내 변수 선언이 정상 처리되는지 확인

**입력 AST**
```
IfStmt
└── thenBranch: BlockStmt
                └── VarDeclareStmt(x = 1.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `if (true) { var x = 1; }` 에 해당하는 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 반환값 `true` |

---

## 추가 예정 TC

| ID | 설명 | 입력 예시 | 비고 |
|---|---|---|---|
| P-TC-27 | 초기값 없는 변수 선언 | `var a;` | initializer == nullptr |
| P-TC-28 | else-if 체이닝 | `if (a) {} else if (b) {}` | elseBranch == IfStmt |
| C-TC-13 | 중첩 블록 깊이 3 중복 | `{ { { var a=1; var a=2; } } }` | 가장 내부 스코프에서 중복 감지 |
| C-TC-14 | 그루핑 내부 자기 참조 | `var a = (a + 1);` | GroupingExpr 내부 탐색 |
| C-TC-15 | 중첩 if 내 변수 선언 충돌 | `{ var a=1; if(x){ var a=2; } }` | shadowing → OK |
