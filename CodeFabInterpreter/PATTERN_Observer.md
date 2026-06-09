# Observer 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Observer (Hook 변형) |
| 대상 | `Executor` ↔ `DebugController` — 실행 이벤트 감지 |
| 관련 파일 | `Executor.h`, `Executor.cpp`, `DebugController.h`, `DebugController.cpp` |

---

## 패턴 구조

GoF Observer 패턴은 Subject(발행자)의 상태 변화를 Observer(구독자)가 통보받아 처리하는 구조다. 이 코드베이스에서는 `Executor` 가 Subject, `DebugController` 가 Observer 역할을 한다.

```
Subject (발행자)       Observer (구독자)
─────────────────      ─────────────────────────
Executor               DebugController
  executeStatement()     beforeExecute(stmt, env, depth)
        │                       ▲
        └───── 이벤트 통보 ──────┘
         (각 Stmt 실행 직전)
```

---

## 코드 구조

### Executor.h — Subject: Observer 등록 인터페이스

```cpp
class Executor : public ExprVisitor
{
public:
    // Observer 등록 (setDebugController(nullptr) 로 해제)
    void setDebugController(DebugController* ctrl) { debug_ = ctrl; }

private:
    DebugController* debug_ = nullptr;  // 등록된 Observer (없으면 null)

    void executeStatement(Stmt* stmt, Environment* env);
};
```

### Executor.cpp — Subject: 이벤트 발행

```cpp
void Executor::executeStatement(Stmt* stmt, Environment* env)
{
    if (stmt->line != 0) currentLine_ = stmt->line;

    // 각 Stmt 실행 직전, 등록된 Observer 에 이벤트를 발행한다.
    if (debug_) debug_->beforeExecute(stmt, env, depth_);

    // 이후 실제 실행 로직 ...
}
```

### DebugController.h — Observer: 이벤트 수신 인터페이스

```cpp
class DebugController
{
public:
    virtual ~DebugController() = default;

    // 이벤트 핸들러 — Executor 가 각 Stmt 실행 전에 호출한다.
    virtual void beforeExecute(Stmt* stmt, Environment* env, int depth = 0);

    // 외부에서 상태 설정 (DebugShell 이 줄 정보를 주입)
    void setLineContext(int lineNo, const std::string& srcLine);
    // ...
};
```

### DebugController.cpp — Observer: 이벤트 처리

```cpp
void DebugController::beforeExecute(Stmt* stmt, Environment* env, int depth)
{
    // 이벤트 수신 후 현재 State에 따라 동작 결정
    if (state_ == ExecutionState::RUNNING)
    {
        if (breakpoints_.isBreakpoint(effectiveLine))
            state_ = ExecutionState::PAUSED;   // 브레이크포인트 도달 → 정지
        else
            return;                            // 자유 실행 중 → 통과
    }

    if (state_ == ExecutionState::NEXT)
    {
        if (depth > nextDepth_) return;        // 더 깊은 곳 → 통과
        state_ = ExecutionState::STEP;
    }

    // 정지: 감시 변수 출력 + 사용자 명령어 입력 대기
    if (!watches_.empty()) watches_.printWatches(env);
    std::cout << "[DEBUG] " << effectiveLine << "번째 줄에서 정지\n> ";

    std::string cmd;
    while (std::getline(std::cin, cmd)) { /* 명령어 처리 */ }
}
```

---

## 이벤트 흐름

```
DebugShell::run()
  └─ executor.setDebugController(&ctrl)   ← Observer 등록

  [각 Stmt 실행 시]
  Executor::executeStatement(stmt, env)
    └─ debug_->beforeExecute(stmt, env, depth_)   ← 이벤트 발행
         └─ DebugController::beforeExecute()       ← 이벤트 수신
              ├─ RUNNING: 브레이크포인트 검사
              ├─ NEXT: depth 비교
              ├─ STEP/PAUSED: 사용자 입력 대기
              └─ 감시 변수 자동 출력
```

---

## virtual 설계 — Observer 교체 가능

`beforeExecute()` 가 `virtual` 이므로 `DebugController` 를 서브클래스로 확장할 수 있다.

```cpp
// 테스트용 — stdin 없이 자동으로 계속 실행
class AutoContinueController : public DebugController {
public:
    void beforeExecute(Stmt*, Environment*, int) override {
        state_ = ExecutionState::RUNNING;  // 항상 RUNNING 상태로 통과
    }
};

// 실제 사용
AutoContinueController ctrl;
executor.setDebugController(&ctrl);
```

이 패턴은 `integrationTest.cpp` 의 디버그 모드 통합 테스트(IT_COMP_03)에서 활용된다.

---

## 장점

### 1. Executor와 DebugController 결합도 최소화
`Executor` 는 `DebugController*` 포인터만 알고, `beforeExecute()` 를 호출하는 방법만 안다. 디버그 로직(브레이크포인트, watch, step/next 등)이 어떻게 구현되어 있는지 전혀 알 필요가 없다.

### 2. 디버그 기능의 선택적 활성화
`debug_` 가 `nullptr` 이면 이벤트 발행이 일어나지 않아 일반 실행 모드에서는 오버헤드가 없다.

```cpp
if (debug_) debug_->beforeExecute(stmt, env, depth_);  // null 이면 건너뜀
```

### 3. Observer 교체·확장
`setDebugController()` 로 런타임에 Observer 를 교체하거나, `virtual beforeExecute()` 를 오버라이드하여 다양한 디버그 전략을 구현할 수 있다.
