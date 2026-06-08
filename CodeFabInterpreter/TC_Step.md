# Step/Next/Continue 디버거 테스트 케이스 명세서

## 대상 파일

`tests/StepTest.cpp`

---

## 테스트 방식 공통 사항

| 구분 | 방식 |
|------|------|
| **제어 방식** | `ScriptedController` (DebugController 서브클래스) — stdin 없이 명령 스크립트로 실행 |
| **정지 기록** | `ScriptedController::pausedLines` — `beforeExecute` 호출 시마다 `stmt->line` 기록 |
| **명령 종류** | `STEP` (다음 Stmt 정지) / `NEXT` (현재 depth 유지, 더 깊은 Stmt 통과) / `CONT` (다음 breakpoint까지 실행) |
| **초기 상태** | `ExecutionState::STEP` — 첫 번째 Stmt에서 즉시 정지 |

---

## TC 목록

| TC ID       | 테스트 함수명                          | 구분     | 구현 상태  |
|-------------|--------------------------------------|---------|----------|
| TC-STEP-01  | TC_STEP_01_StepEntersBlockInternals   | Positive | 🟢 Green  |
| TC-STEP-02  | TC_STEP_02_NextSkipsBlockInternals    | Positive | 🟢 Green  |
| TC-STEP-03  | TC_STEP_03_ContinueRunsToBreakpoint   | Positive | 🟢 Green  |
| TC-STEP-04  | TC_STEP_04_NextSkipsFunctionBody      | Positive | 🟡 Pending (PR #41 머지 후 Green) |

---

## TC 상세

---

### TC-STEP-01 — StepEntersBlockInternals

**목적**
`step` 명령 시 블록 내부 Stmt에도 모두 정지하는지 확인

**입력 코드**
```
var a = 1;
if (a > 0) {
print a;
}
```

**명령 스크립트**
`STEP, STEP, STEP, STEP`

**기대 정지 순서**

| 순서 | 줄번호 | Stmt 설명 |
|------|--------|-----------|
| 1    | 1      | VarDeclareStmt |
| 2    | 2      | IfStmt |
| 3    | 2      | BlockStmt (then branch) |
| 4    | 3      | PrintStmt |

---

### TC-STEP-02 — NextSkipsBlockInternals

**목적**
`next` 명령 시 블록 내부 Stmt를 건너뛰고 다음 같은 depth의 Stmt에서 정지하는지 확인

**입력 코드**
```
var a = 1;
if (a > 0) {
print a;
}
print 2;
```

**명령 스크립트**
`STEP` (줄 1), `NEXT` (줄 2 if)

**기대 정지 순서**

| 순서 | 줄번호 | Stmt 설명 |
|------|--------|-----------|
| 1    | 1      | VarDeclareStmt |
| 2    | 2      | IfStmt → next 발행 |
| 3    | 5      | PrintStmt (블록 내부 skip) |

---

### TC-STEP-03 — ContinueRunsToBreakpoint

**목적**
`continue` 명령 시 다음 breakpoint까지 실행 후 정지하는지 확인

**입력 코드**
```
print 1;
print 2;
print 3;
```

**설정:** breakpoint at 줄 3

**명령 스크립트**
`CONT` (줄 1 초기 정지)

**기대 정지 순서**

| 순서 | 줄번호 | 설명 |
|------|--------|------|
| 1    | 1      | 초기 STEP 정지 |
| 2    | 3      | breakpoint 도달 |

---

### TC-STEP-04 — NextSkipsFunctionBody

**목적**
`next` 명령 시 함수 호출 body 내부에 진입하지 않고 다음 Stmt에서 정지하는지 확인
(**"next-over-call 버그"** 수정 검증 — `Executor.cpp` `FunctionCallExpr` 실행 시 `DepthGuard` 누락 버그)

**입력 코드**
```
func foo() {
print 99;
}
foo();
print 2;
```

**명령 스크립트**
`STEP` (줄 1 func 선언), `NEXT` (줄 4 foo() 호출)

**기대 정지 순서 (수정 후)**

| 순서 | 줄번호 | Stmt 설명 |
|------|--------|-----------|
| 1    | 1      | FunctionDeclareStmt |
| 2    | 4      | ExpressionStmt `foo()` → next 발행 |
| 3    | 5      | PrintStmt `print 2` (함수 body skip) |

**버그 시 실제 동작 (수정 전)**

| 순서 | 줄번호 | 설명 |
|------|--------|------|
| 1    | 1      | FunctionDeclareStmt |
| 2    | 4      | ExpressionStmt `foo()` → next 발행 |
| 3    | **1**  | foo body BlockStmt(line=1) 에 **잘못 진입** |

**원인 분석**
`FunctionCallExpr` 평가 시 `DepthGuard`가 없어 `depth_`가 증가하지 않음.
`beforeExecute(blockStmt, depth=0)` → `depth(0) > nextDepth_(0)` → false → 진입.

**수정**
`Executor.cpp` `FunctionCallExpr` 평가 직전에 `DepthGuard g(depth_)` 추가 (PR #41).

**구현 상태** : 🟡 PR #41 (`feature/factoryShell`) 머지 후 Green

---

## 요약

| TC ID      | 명령 | 검증 항목 | 상태 |
|------------|------|---------|------|
| TC-STEP-01 | step | 블록 내부 진입 | 🟢 Green |
| TC-STEP-02 | next | 블록 내부 skip | 🟢 Green |
| TC-STEP-03 | cont | breakpoint 도달 | 🟢 Green |
| TC-STEP-04 | next | 함수 body skip (next-over-call 버그 수정) | 🟡 Pending |
