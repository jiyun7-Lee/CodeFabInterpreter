# Composite 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Composite |
| 대상 | `Stmt` 계층 — AST 문장 트리 구조 |
| 관련 파일 | `Stmt.h`, `Checker.cpp`, `Executor.cpp` |

---

## 패턴 구조

GoF Composite 패턴은 단일 객체(Leaf)와 복합 객체(Composite)를 동일한 인터페이스로 처리할 수 있게 한다. 트리 구조를 표현하며, 클라이언트는 Leaf 인지 Composite 인지 구별하지 않아도 된다.

```
Stmt (추상 기반 — 공통 인터페이스)
  │
  ├─ Leaf 노드 (자식 없음)
  │    ├─ ExpressionStmt
  │    ├─ PrintStmt
  │    ├─ VarDeclareStmt
  │    ├─ FunctionDeclareStmt
  │    └─ ReturnStmt
  │
  └─ Composite 노드 (자식 Stmt 포함)
       ├─ BlockStmt      → std::vector<unique_ptr<Stmt>>
       ├─ IfStmt         → thenBranch + elseBranch (unique_ptr<Stmt>)
       └─ ForStmt        → init + body (unique_ptr<Stmt>)
```

---

## 코드 구조

### Stmt.h — Leaf / Composite 노드 정의

```cpp
// 공통 기반 — 모든 Stmt 의 인터페이스
class Stmt {
public:
    int line = 0;
    virtual ~Stmt() = default;
};

// ── Leaf 노드 ──────────────────────────────────────────────────────────
// 자식 Stmt 를 포함하지 않는 단말 노드들

class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
};

class PrintStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
};

class VarDeclareStmt : public Stmt {
public:
    Token                 name;
    std::unique_ptr<Expr> initializer;
};

class ReturnStmt : public Stmt {
public:
    Token                 keyword;
    std::unique_ptr<Expr> value;
};

// ── Composite 노드 ─────────────────────────────────────────────────────
// 자식 Stmt 를 포함하는 복합 노드들

class BlockStmt : public Stmt {
public:
    // 여러 Stmt 를 순서대로 포함
    std::vector<std::unique_ptr<Stmt>> statements;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;   // 자식 Stmt (또 다른 BlockStmt 가능)
    std::unique_ptr<Stmt> elseBranch;   // 자식 Stmt (선택적)
};

class ForStmt : public Stmt {
public:
    std::unique_ptr<Stmt> init;         // 자식 Stmt (VarDeclareStmt 또는 ExpressionStmt)
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;         // 자식 Stmt (BlockStmt 등)
};
```

### Executor.cpp — Composite 노드 재귀 처리

```cpp
void Executor::executeStatement(Stmt* stmt, Environment* env)
{
    // Leaf 노드 처리
    if (auto* s = dynamic_cast<PrintStmt*>(stmt))
    {
        printValue(evaluateExpr(s->expression.get(), env));
        return;
    }

    // Composite 노드 처리 — 자식 Stmt 를 재귀적으로 실행
    if (auto* s = dynamic_cast<BlockStmt*>(stmt))
    {
        Environment blockEnv;
        blockEnv.parent = env;
        for (const auto& st : s->statements)
            executeStatement(st.get(), &blockEnv);   // ← 재귀
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(stmt))
    {
        if (isTruthy(evaluateExpr(s->condition.get(), env)))
            executeStatement(s->thenBranch.get(), env);  // ← 재귀
        else if (s->elseBranch)
            executeStatement(s->elseBranch.get(), env);  // ← 재귀
        return;
    }

    if (auto* s = dynamic_cast<ForStmt*>(stmt))
    {
        Environment forEnv;
        forEnv.parent = env;
        if (s->init)
            executeStatement(s->init.get(), &forEnv);    // ← 재귀

        while (!s->condition || isTruthy(evaluateExpr(s->condition.get(), &forEnv)))
        {
            executeStatement(s->body.get(), &forEnv);    // ← 재귀
            if (s->increment)
                evaluateExpr(s->increment.get(), &forEnv);
        }
        return;
    }
    // ...
}
```

### Checker.cpp — Composite 노드 재귀 탐색

```cpp
static void checkStmt(Stmt* stmt, ...)
{
    // Composite 노드: 자식을 재귀적으로 검사
    if (auto* s = dynamic_cast<BlockStmt*>(stmt))
    {
        scopes.push_back({});
        for (const auto& st : s->statements)
            checkStmt(st.get(), ...);   // ← 재귀
        scopes.pop_back();
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(stmt))
    {
        checkExpr(s->condition.get(), ...);
        checkStmt(s->thenBranch.get(), ...);  // ← 재귀
        if (s->elseBranch)
            checkStmt(s->elseBranch.get(), ...);  // ← 재귀
        return;
    }

    if (auto* s = dynamic_cast<ForStmt*>(stmt))
    {
        // ...
        checkStmt(s->body.get(), ...);  // ← 재귀
        return;
    }
}
```

---

## AST 트리 예시

```
// 소스 코드:
// for (var i = 0; i < 3; i = i + 1) {
//     if (i > 1) {
//         print i;
//     }
// }

ForStmt
  ├─ init:      VarDeclareStmt ("i" = 0)      ← Leaf
  ├─ condition: BinaryExpr (i < 3)
  ├─ increment: AssignExpr (i = i + 1)
  └─ body:      BlockStmt                      ← Composite
                  └─ IfStmt                    ← Composite
                       ├─ condition: BinaryExpr (i > 1)
                       └─ thenBranch: BlockStmt ← Composite
                                        └─ PrintStmt  ← Leaf
```

`Executor::executeStatement()` 는 최상위 `ForStmt` 하나만 받아도, 재귀 호출을 통해 트리 전체를 순회하며 실행한다.

---

## 장점

### 1. 균일한 처리
`executeStatement(Stmt*)` 와 `checkStmt(Stmt*)` 는 `PrintStmt` (Leaf) 와 `BlockStmt` (Composite) 를 동일한 함수 시그니처로 처리한다. 호출부는 어떤 Stmt 타입인지 신경 쓰지 않아도 된다.

### 2. 임의 깊이 중첩 표현
`BlockStmt` 안에 `IfStmt`, `IfStmt` 안에 `ForStmt`, `ForStmt` 안에 `BlockStmt` … 임의 깊이의 중첩 구문을 자연스럽게 표현한다. 언어 스펙이 허용하는 한 제한이 없다.

### 3. 새 복합 구문 추가 용이
새 복합 구문(예: `WhileStmt`, `SwitchStmt`)을 추가할 때 `Stmt` 를 상속하는 새 클래스를 작성하고, `executeStatement()` / `checkStmt()` 에 처리 분기를 추가하면 된다. 기존 Leaf / Composite 노드는 변경하지 않아도 된다.
