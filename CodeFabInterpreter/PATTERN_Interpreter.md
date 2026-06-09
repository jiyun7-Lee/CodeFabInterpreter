# Interpreter 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Interpreter |
| 대상 | `Expr` / `Stmt` 클래스 계층 전체 |
| 변경 파일 | `Expr.h`, `Stmt.h` (구조 정의), `Parser.cpp` (AST 생성), `Executor.cpp` (해석) |

---

## 패턴 구조

GoF Interpreter 패턴은 언어의 문법 규칙을 클래스 계층으로 표현하고, 각 노드가 자신을 해석(interpret)하는 메서드를 갖는다. 이 코드베이스에서는 Visitor 패턴과 결합하여 해석 로직을 노드 외부(Executor)에 분리했다.

```
문법 규칙               → 클래스
─────────────────────────────────────────
expression             → Expr (추상)
  literal              → LiteralExpr
  variable             → VariableExpr
  unary                → UnaryExpr
  binary               → BinaryExpr
  assign               → AssignExpr
  grouping             → GroupingExpr
  function call        → FunctionCallExpr
  array literal        → ArrayLiteralExpr
  array access         → ArrayAccessExpr
  array write          → ArrayWriteExpr

statement              → Stmt (추상)
  expression stmt      → ExpressionStmt
  print stmt           → PrintStmt
  var declaration      → VarDeclareStmt
  block                → BlockStmt
  if                   → IfStmt
  for                  → ForStmt
  function declaration → FunctionDeclareStmt
  return               → ReturnStmt
```

---

## 코드 구조

### Expr.h — 표현식 문법 규칙 → 클래스 계층

```cpp
// 추상 기반 — 모든 표현식의 공통 인터페이스
class Expr {
public:
    virtual ~Expr() = default;
    virtual Value accept(ExprVisitor& v) = 0;  // 해석 진입점
};

// 터미널 노드 — 더 이상 분해되지 않는 리터럴
class LiteralExpr : public Expr {
public:
    Value value;
    Value accept(ExprVisitor& v) override { return v.visitLiteral(*this); }
};

// 비터미널 노드 — 하위 표현식을 포함하는 이진 연산
class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token                 op;
    std::unique_ptr<Expr> right;
    Value accept(ExprVisitor& v) override { return v.visitBinary(*this); }
};
```

### Stmt.h — 문장 문법 규칙 → 클래스 계층

```cpp
class Stmt {
public:
    int line = 0;
    virtual ~Stmt() = default;
};

// 터미널 노드 — 단일 표현식 문장
class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
};

// 비터미널 노드 — 여러 문장을 포함하는 블록
class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
};

// 비터미널 노드 — 조건 분기
class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
};
```

### Parser.cpp — 토큰 스트림 → AST 생성

```cpp
// 파서가 문법 규칙에 따라 AST 노드를 생성한다.
// 각 parse 함수가 하나의 문법 규칙에 대응한다.

std::unique_ptr<Expr> Parser::parsePrimary()
{
    if (match({ TokenType::NUMBER, TokenType::STRING }))
    {
        auto lit   = std::make_unique<LiteralExpr>();
        lit->value = previous().literal;
        return lit;                           // ← 터미널 노드 생성
    }
    // ...
}

std::unique_ptr<Expr> Parser::parseBinary()   // (parseTerm / parseFactor 등)
{
    auto left = parseUnary();
    while (match({ TokenType::PLUS, TokenType::MINUS }))
    {
        Token op    = previous();
        auto  right = parseUnary();
        left = makeBinary(std::move(left), op, std::move(right));  // ← 비터미널 노드 생성
    }
    return left;
}
```

### Executor.cpp — AST 노드 해석 (ExprVisitor 구현)

```cpp
// Executor 가 ExprVisitor 를 구현하여 각 노드를 해석한다.
// accept() 호출 → visitXxx() 로 이중 디스패치 → 값(Value) 반환

Value Executor::evaluateExpr(Expr* expr, Environment* env)
{
    Environment* saved = env_;
    env_ = env;
    struct EnvGuard { ... } guard{*this, saved};
    return expr->accept(*this);   // ← Interpreter 해석 시작
}

Value Executor::visitLiteral(LiteralExpr& e)   { return e.value; }
Value Executor::visitBinary(BinaryExpr& e)
{
    Value lv = evaluateExpr(e.left.get(),  env_);
    Value rv = evaluateExpr(e.right.get(), env_);
    // ... 연산 수행 후 결과 반환
}
```

---

## AST 예시 — `print 1 + 2 * 3;`

```
PrintStmt
  └─ BinaryExpr (+)
       ├─ LiteralExpr (1)
       └─ BinaryExpr (*)
            ├─ LiteralExpr (2)
            └─ LiteralExpr (3)
```

해석 순서:
1. `PrintStmt` → `BinaryExpr(+)` 해석 요청
2. `BinaryExpr(+)` → 좌변 `LiteralExpr(1)` 해석 → `1.0`
3. `BinaryExpr(+)` → 우변 `BinaryExpr(*)` 해석
4. `BinaryExpr(*)` → `2.0 * 3.0` → `6.0`
5. `BinaryExpr(+)` → `1.0 + 6.0` → `7.0`
6. `PrintStmt` → `7` 출력

---

## 고전 GoF 방식과의 차이

| 항목 | 고전 GoF Interpreter | 이 코드베이스 |
|------|----------------------|---------------|
| 해석 메서드 위치 | 각 노드 내부 `interpret()` | 외부 `Executor` (`ExprVisitor` 구현) |
| 확장 방식 | 새 노드 클래스 추가 | 새 `ExprVisitor` 구현체 추가 |
| 이유 | 단순 구조 | Visitor 결합으로 해석 로직 분리 → 새 연산 추가 시 Expr.h 무변경 |

Visitor 패턴과 결합하여 Interpreter의 핵심 구조(문법 = 클래스 계층, 실행 = 트리 순회)를 유지하면서 확장성을 높인 형태다.

---

## 장점

### 1. 문법 규칙이 코드 구조로 직접 반영
언어 문법의 각 규칙이 클래스 하나에 대응하므로, 새 문법을 추가할 때 새 클래스를 추가하면 된다. 파서·체커·실행기가 자동으로 확장된다.

### 2. 재귀적 구성
AST 노드가 `std::unique_ptr<Expr>` / `std::unique_ptr<Stmt>` 로 자식 노드를 소유하여, 임의 깊이의 중첩 구문을 자연스럽게 표현한다.

### 3. 관심사 분리
- `Expr`/`Stmt` 계층 — **무엇을** 표현하는가 (구조)
- `Parser` — **어떻게** AST를 만드는가 (생성)
- `Checker` — **의미가 올바른가** (정적 분석)
- `Executor` — **어떻게 실행하는가** (해석)
