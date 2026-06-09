# Visitor 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Visitor (Style A — 구체 노드별 visit 메서드) |
| 대상 계층 | `Expr` 및 10개 서브클래스 |
| 적용 브랜치 | `refactor/expr-parser` |
| 변경 파일 | `Expr.h`, `Executor.h`, `Executor.cpp` |

---

## 적용 전 구조

`Executor::evaluateExpr` 이 `dynamic_cast` 체인으로 노드 타입을 판별했다.

```cpp
// Executor.cpp (변경 전)
Value Executor::evaluateExpr(Expr* expr, Environment* env)
{
    if (auto* e = dynamic_cast<LiteralExpr*>(expr))
        return e->value;

    if (auto* e = dynamic_cast<VariableExpr*>(expr))
        return e->distance >= 0 ? env->getAt(...) : env->get(...);

    if (auto* e = dynamic_cast<GroupingExpr*>(expr))
        return evaluateExpr(e->expression.get(), env);

    // ... 10개 타입에 대한 if-dynamic_cast 반복 ...

    return std::monostate{};
}
```

`Expr` 기반 클래스에는 `virtual ~Expr() = default;` 만 있었다.

```cpp
// Expr.h (변경 전)
class Expr {
public:
    virtual ~Expr() = default;
    // accept() 없음
};
```

---

## 적용 후 구조

### Expr.h — ExprVisitor 인터페이스 + accept() 추가

```cpp
// Expr.h (변경 후)

// 1. ExprVisitor 인터페이스 (노드 타입별 순수 가상 메서드)
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual Value visitLiteral      (LiteralExpr&       e) = 0;
    virtual Value visitVariable     (VariableExpr&      e) = 0;
    virtual Value visitUnary        (UnaryExpr&         e) = 0;
    virtual Value visitBinary       (BinaryExpr&        e) = 0;
    virtual Value visitAssign       (AssignExpr&        e) = 0;
    virtual Value visitGrouping     (GroupingExpr&      e) = 0;
    virtual Value visitFunctionCall (FunctionCallExpr&  e) = 0;
    virtual Value visitArrayLiteral (ArrayLiteralExpr&  e) = 0;
    virtual Value visitArrayAccess  (ArrayAccessExpr&   e) = 0;
    virtual Value visitArrayWrite   (ArrayWriteExpr&    e) = 0;
};

// 2. Expr 기반 클래스에 accept() 추상 메서드 추가
class Expr {
public:
    virtual ~Expr() = default;
    virtual Value accept(ExprVisitor& v) = 0;  // ← 추가
};

// 3. 각 서브클래스에 accept() 구현 (이중 디스패치)
class LiteralExpr : public Expr {
public:
    Value value;
    Value accept(ExprVisitor& v) override { return v.visitLiteral(*this); }
};

// ... 나머지 9개 서브클래스도 동일한 방식으로 구현
```

### Executor.h — ExprVisitor 상속 + env_ 멤버 추가

```cpp
// Executor.h (변경 후)
class Executor : public ExprVisitor   // ← ExprVisitor 구현체로 선언
{
private:
    Environment* env_ = nullptr;  // ← 현재 평가 환경 (visit 메서드에서 사용)

    // ExprVisitor 구현 선언
    Value visitLiteral      (LiteralExpr&       e) override;
    Value visitVariable     (VariableExpr&      e) override;
    // ... 10개 선언
};
```

### Executor.cpp — evaluateExpr 단순화 + visit 메서드 구현

```cpp
// Executor.cpp (변경 후)

// evaluateExpr: 환경 설정 후 Visitor 디스패치만 수행
Value Executor::evaluateExpr(Expr* expr, Environment* env)
{
    Environment* saved = env_;  // 재귀 호출 시 env_ 오염 방지
    env_ = env;
    Value result = expr->accept(*this);
    env_ = saved;
    return result;
}

// 노드별 처리 로직이 독립 메서드로 분리됨
Value Executor::visitLiteral(LiteralExpr& e)   { return e.value; }
Value Executor::visitVariable(VariableExpr& e)  { return e.distance >= 0 ? ... : ...; }
Value Executor::visitGrouping(GroupingExpr& e)  { return evaluateExpr(e.expression.get(), env_); }
// ...
```

---

## 이중 디스패치 흐름

```
evaluateExpr(expr, env)
    │
    ├─ env_ 설정
    └─ expr->accept(*this)         ← 1st dispatch: Expr의 구체 타입 결정
           │
           └─ v.visitLiteral(*this) ← 2nd dispatch: Executor의 visitLiteral 호출
```

`dynamic_cast` 없이 vtable 두 번의 간접 호출만으로 정확한 처리 메서드에 도달한다.

---

## env_ save/restore 적용 이유

`env_` 는 멤버 변수이므로, 함수 호출 내부에서 `executeStatement` → `evaluateExpr`
순서로 중첩될 경우 외부 환경 포인터가 덮어쓰여진다.

```
visitBinary
  evaluateExpr(left, env_)         env_ = outerEnv
    visitFunctionCall
      executeStatement(fn.body, &fnEnv)
        evaluateExpr(inner, &fnEnv)  env_ = &fnEnv  ← 덮어쓰기 발생
  evaluateExpr(right, env_)        env_ 가 &fnEnv (댕글링!) ← 버그
```

save/restore 패턴으로 각 `evaluateExpr` 호출이 반환 시 이전 환경을 복원한다.

---

## 장점

### 1. 타입 안전성
`dynamic_cast` 는 실패 시 `nullptr` 를 반환하고 체인 끝에서 `std::monostate{}` 를 반환해
**런타임에서야 누락을 발견**한다. Visitor 패턴은 컴파일러가 모든 `visitXxx()` 구현을 강제하므로,
새 Expr 서브클래스 추가 시 `Executor` 구현을 빠뜨리면 **컴파일 오류**로 즉시 알 수 있다.

### 2. 새 연산 추가 용이
Expr 노드를 수정하지 않고 새 `ExprVisitor` 구현체만 추가하면 새 연산(예: 타입 추론기, 코드 생성기, 프린터)을 붙일 수 있다.

```cpp
// 새 연산 추가 예시 — Expr.h 무변경
class ExprPrinter : public ExprVisitor {
    Value visitLiteral(LiteralExpr& e) override {
        std::cout << e.value; return {};
    }
    // ...
};
```

### 3. 책임 분리
각 노드 타입의 평가 로직이 독립 메서드(`visitXxx`)로 분리되어 가독성과 테스트성이 향상된다.
`visitBinary` 만 단독으로 읽고 이해할 수 있으며, 해당 메서드만 단위 테스트할 수 있다.

### 4. 유지보수성
기존에는 `evaluateExpr` 172줄 단일 함수 안에 모든 노드 처리가 뒤섞여 있었다.
리팩토링 후 `evaluateExpr` 는 7줄로 줄고, 각 `visitXxx` 는 평균 10~20줄의 독립 함수가 된다.

---

## 변경 범위 요약

| 파일 | 변경 유형 | 핵심 내용 |
|------|-----------|-----------|
| `Expr.h` | 추가 | `ExprVisitor` 인터페이스, 각 서브클래스 `accept()` |
| `Executor.h` | 수정 | `: public ExprVisitor` 상속, `env_` 멤버, 10개 visit 선언 |
| `Executor.cpp` | 수정 | `evaluateExpr` 10개 → 7줄, `visitXxx` 10개 신규 구현 |
| `Checker.cpp` | 무변경 | 내부 `checkExpr` 는 static 함수 + void 반환 구조로 Visitor 계층과 무관 |
