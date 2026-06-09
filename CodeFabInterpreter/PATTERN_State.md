# State 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF State |
| 대상 | `ExecutionState` + `DebugController` — 디버그 실행 상태 전환 |
| 관련 파일 | `DebugController.h`, `DebugController.cpp` |

---

## 패턴 구조

GoF State 패턴은 객체의 내부 상태에 따라 동작이 달라지도록 상태를 캡슐화한다. 상태 전환은 명시적이며, 각 상태에서의 동작이 명확하게 분리된다.

```
ExecutionState (상태 열거형)
  ├─ STEP    : 매 Stmt마다 정지, 사용자 입력 대기
  ├─ RUNNING : 자유 실행, 브레이크포인트 도달 시 PAUSED 전환
  ├─ PAUSED  : 브레이크포인트에서 정지, 사용자 입력 대기
  └─ NEXT    : 현재 depth 이하만 통과, 같거나 얕아지면 STEP 전환
```

---

## 코드 구조

### DebugController.h — 상태 정의

```cpp
// 4가지 실행 상태
enum class ExecutionState { RUNNING, PAUSED, STEP, NEXT };

class DebugController
{
protected:
    ExecutionState state_     = ExecutionState::STEP;  // 초기 상태: STEP
    int            nextDepth_ = 0;   // NEXT 상태에서 통과 기준 depth
    // ...
};
```

### DebugController.cpp — 상태별 동작과 전환

```cpp
void DebugController::beforeExecute(Stmt* stmt, Environment* env, int depth)
{
    int effectiveLine = currentLineNo_ > 0 ? currentLineNo_ : stmt->line;

    // ── RUNNING 상태 ──────────────────────────────────────────────────
    // 브레이크포인트에 도달하면 PAUSED 로 전환, 아니면 즉시 반환(통과)
    if (state_ == ExecutionState::RUNNING)
    {
        if (breakpoints_.isBreakpoint(effectiveLine))
            state_ = ExecutionState::PAUSED;   // 상태 전환: RUNNING → PAUSED
        else
            return;                            // 통과
    }

    // ── NEXT 상태 ─────────────────────────────────────────────────────
    // 발행 당시 depth 보다 깊은 Stmt 는 통과
    // 같거나 얕아지면 STEP 으로 전환하여 정지
    if (state_ == ExecutionState::NEXT)
    {
        if (depth > nextDepth_) return;        // 통과
        state_ = ExecutionState::STEP;         // 상태 전환: NEXT → STEP
    }

    // ── STEP / PAUSED 상태 ────────────────────────────────────────────
    // 정지: 감시 변수 출력 후 사용자 명령어 입력 대기
    if (!watches_.empty()) watches_.printWatches(env);
    std::cout << "[DEBUG] " << effectiveLine << "번째 줄에서 정지 -> '"
              << currentSrcLine_ << "'\n> " << std::flush;

    std::string cmd;
    while (std::getline(std::cin, cmd))
    {
        const std::string low = toLower(cmd);

        // 명령어에 따른 상태 전환
        if (low == "step" || low == "s")
        {
            state_ = ExecutionState::STEP;    // 상태 전환: → STEP
            return;
        }
        if (low == "next" || low == "n")
        {
            nextDepth_ = depth;
            state_ = ExecutionState::NEXT;    // 상태 전환: → NEXT
            return;
        }
        if (low == "continue" || low == "c")
        {
            state_ = ExecutionState::RUNNING; // 상태 전환: → RUNNING
            return;
        }
        // break / watch / inspect 등은 상태 전환 없이 처리
        // ...
    }
}
```

---

## 상태 전이 다이어그램

```
              [초기]
                │
                ▼
           ┌─────────┐
    ┌─────▶│  STEP   │◀──────────────────┐
    │      └────┬────┘                   │
    │           │ 실행 → beforeExecute()  │
    │           │ 정지 + 사용자 입력 대기  │
    │           │                         │
    │    "step" │       "next"            │
    │       ────┤     ──────────▶ ┌──────┴──────┐
    │           │                 │    NEXT      │
    │           │                 │  (depth 감시) │
    │           │                 └──────┬───────┘
    │           │                   depth ≤ nextDepth_
    │           │                        │
    │           │                        │ (STEP 으로 자동 전환)
    │           │                        │
    │      "continue"            ┌───────┴──────┐
    │       ────┴──────────────▶ │   RUNNING    │
    │                            │ (자유 실행)   │
    │                            └───────┬───────┘
    │                             브레이크포인트 도달
    │                                    │
    │                            ┌───────▼───────┐
    └────────────────────────────│    PAUSED      │
                   "step"/"next" │  (bp 정지)    │
                   "continue"    └───────────────┘
```

---

## 각 상태 요약

| 상태 | 진입 조건 | 동작 | 탈출 조건 |
|------|-----------|------|-----------|
| `STEP` | 초기 상태 / `step` 명령 | 매 Stmt 정지, 입력 대기 | `step` → STEP, `next` → NEXT, `continue` → RUNNING |
| `RUNNING` | `continue` 명령 | 정지 없이 통과 | 브레이크포인트 도달 → PAUSED |
| `PAUSED` | 브레이크포인트 도달 | 정지, 입력 대기 | `step` → STEP, `next` → NEXT, `continue` → RUNNING |
| `NEXT` | `next` 명령 | depth > nextDepth_ 이면 통과 | depth ≤ nextDepth_ → STEP |

---

## 장점

### 1. 조건문 분산 방지
상태에 따른 분기가 `beforeExecute()` 한 곳에 집중되어 있다. 상태별 동작을 추가·변경할 때 이 메서드만 수정하면 된다.

### 2. 상태 전환의 명시성
`state_ = ExecutionState::RUNNING` 처럼 상태 전환이 코드에서 명확히 보인다. 디버깅 시 현재 상태를 `state_` 하나만 보면 전체 흐름을 파악할 수 있다.

### 3. NEXT 의 depth 기반 정밀 제어
`nextDepth_` 를 명령 발행 시점의 `depth` 로 기록하여, 블록 내부(`depth > nextDepth_`)는 통과하고 같은 레벨로 돌아오면 자동으로 `STEP` 으로 복귀하는 세밀한 제어가 가능하다.
