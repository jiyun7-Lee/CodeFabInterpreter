# DebugController 테스트 케이스 명세서

## 대상 파일

`tests/DebugInfraTest.cpp`  
`tests/BreakpointTest.cpp`  
`tests/WatchTest.cpp`  
`tests/DebugControllerTest.cpp`

---

## 테스트 방식 공통 사항

| 구분 | 방식 |
|------|------|
| **상태 제어** | `ExposedController` — `state_` 노출 최소 서브클래스, stdin 없이 상태 직접 설정 |
| **stdin 차단** | `CinRedirect` RAII 가드 — `std::istringstream` 으로 stdin 리다이렉트 |
| **출력 캡처** | `captureOutput()` 헬퍼 (TestHelpers.h) |
| **초기 상태** | `ExecutionState::STEP` — 첫 번째 Stmt 에서 즉시 정지 |

---

## TC 목록 — 인프라 (DebugInfraTest)

| TC ID           | 테스트 함수명                           | 구분     | 구현 상태 |
|-----------------|---------------------------------------|---------|---------|
| TC-DBG-INFRA-01 | TC_DBG_INFRA_01_StmtHasLineNumber     | Positive | 🟢 Green |
| TC-DBG-INFRA-02 | TC_DBG_INFRA_02_HookCalledForEachStmt | Positive | 🟢 Green |
| TC-DBG-INFRA-03 | TC_DBG_INFRA_03_NoHookNormalExecution | Positive | 🟢 Green |

---

## TC 목록 — 브레이크포인트 (BreakpointTest)

| TC ID    | 테스트 함수명                                 | 구분     | 구현 상태 |
|----------|---------------------------------------------|---------|---------|
| TC-BP-01 | TC_BP_01_AddBreakpoint                      | Positive | 🟢 Green |
| TC-BP-02 | TC_BP_02_RemoveBreakpoint                   | Positive | 🟢 Green |
| TC-BP-03 | TC_BP_03_PrintBreakpoints                   | Positive | 🟢 Green |
| TC-BP-04 | TC_BP_04_PausesAtBreakpointLine             | Positive | 🟢 Green |
| TC-BP-05 | TC_BP_05_StepAfterBreakpointMovesToNextLine | Positive | 🟢 Green |

---

## TC 목록 — Watch / Inspect (WatchTest)

| TC ID       | 테스트 함수명                                  | 구분     | 구현 상태 |
|-------------|----------------------------------------------|---------|---------|
| TC-WATCH-01 | TC_WATCH_01_AddWatch                         | Positive | 🟢 Green |
| TC-WATCH-02 | TC_WATCH_02_RemoveWatch                      | Positive | 🟢 Green |
| TC-WATCH-03 | TC_WATCH_03_PrintMultipleWatches             | Positive | 🟢 Green |
| TC-WATCH-04 | TC_WATCH_04_OutOfScopeVariable               | Negative | 🟢 Green |
| TC-WATCH-05 | TC_WATCH_05_Inspect                          | Positive | 🟢 Green |
| TC-WATCH-06 | TC_WATCH_06_AddWatch_TrimsWhitespace         | Positive | 🟢 Green |
| TC-WATCH-07 | TC_WATCH_07_Inspect_DoesNotExposeParentScope | Positive | 🟢 Green |

---

## TC 목록 — 커버리지 향상 (DebugControllerTest)

| TC ID           | 테스트 함수명                                     | 구분     | 구현 상태 |
|-----------------|--------------------------------------------------|---------|---------|
| TC-DBG-VAL-01   | TC_DBG_VAL_01_StringValueInWatch                 | Positive | 🟢 Green |
| TC-DBG-VAL-02   | TC_DBG_VAL_02_BoolTrueInWatch                    | Positive | 🟢 Green |
| TC-DBG-VAL-03   | TC_DBG_VAL_03_BoolFalseInWatch                   | Positive | 🟢 Green |
| TC-DBG-VAL-04   | TC_DBG_VAL_04_NullValueInWatch                   | Positive | 🟢 Green |
| TC-DBG-VAL-05   | TC_DBG_VAL_05_ArrayValueInWatch                  | Positive | 🟢 Green |
| TC-DBG-VAL-06   | TC_DBG_VAL_06_InspectNullEnv                     | Negative | 🟢 Green |
| TC-DBG-VAL-07   | TC_DBG_VAL_07_InspectEmptyEnv                    | Negative | 🟢 Green |
| TC-DBG-BP-06    | TC_DBG_BP_06_PrintEmptyBreakpoints               | Negative | 🟢 Green |
| TC-DBG-STATE-01 | TC_DBG_STATE_01_RunningNoBreakpoint_EarlyReturn  | Positive | 🟢 Green |
| TC-DBG-STATE-02 | TC_DBG_STATE_02_RunningHitsBreakpoint_Pauses     | Positive | 🟢 Green |
| TC-DBG-STATE-03 | TC_DBG_STATE_03_SourceLines_UsedInDisplay        | Positive | 🟢 Green |
| TC-DBG-STATE-04 | TC_DBG_STATE_04_SourceLines_FallbackToSrcLine    | Positive | 🟢 Green |
| TC-DBG-STATE-05 | TC_DBG_STATE_05_NextState_SkipsDeepStmt          | Positive | 🟢 Green |
| TC-DBG-STATE-06 | TC_DBG_STATE_06_WatchAutoPrintOnStep             | Positive | 🟢 Green |
| TC-DBG-STATE-07 | TC_DBG_STATE_07_BreakRemoveCommands              | Positive | 🟢 Green |
| TC-DBG-STATE-08 | TC_DBG_STATE_08_WatchInspectCommands             | Positive | 🟢 Green |
| TC-DBG-STATE-09 | TC_DBG_STATE_09_UnknownCommandMessage            | Negative | 🟢 Green |

---

## TC 상세 — 커버리지 향상 TC

---

### TC-DBG-VAL-01 ~ 05 — valueToString 타입별 커버

**목적**
`WatchManager::printWatches()` 내부 `valueToString()` 의 string / bool / null / array 타입 분기 커버

| TC ID | 입력 값 | 기대 출력 포함 문자열 |
|-------|--------|------------------|
| TC-DBG-VAL-01 | `std::string("hello")` | `hello` |
| TC-DBG-VAL-02 | `bool(true)` | `true` |
| TC-DBG-VAL-03 | `bool(false)` | `false` |
| TC-DBG-VAL-04 | `std::monostate{}` | `null` |
| TC-DBG-VAL-05 | `shared_ptr<ArrayValue>` (double/string/bool/null 원소 포함) | `[`, `hi`, `true`, `null` |

> TC-DBG-VAL-05: PR #57에서 배열 watch 출력이 `[array]` 대신 실제 원소를 표시하도록 수정됨. 배열 원소 각 타입(double/string/bool/null) 분기를 함께 커버.

---

### TC-DBG-VAL-06 ~ 07 / TC-DBG-BP-06 — 빈 케이스 커버

**목적**
`printInspect()` null/빈 환경, `BreakpointManager::print()` 빈 경우의 분기 커버

| TC ID | 시나리오 | 기대 출력 |
|-------|---------|---------|
| TC-DBG-VAL-06 | `printInspect(nullptr)` | `"변수 없음"` 포함 |
| TC-DBG-VAL-07 | `printInspect(&empty_env)` | `"변수 없음"` 포함 |
| TC-DBG-BP-06  | `BreakpointManager::print()` (브레이크포인트 없음) | `"없음"` 포함 |

---

### TC-DBG-STATE-01 — RUNNING + 브레이크포인트 없음

**목적**
`beforeExecute()` RUNNING 상태에서 브레이크포인트 미도달 시 `else return` 조기 반환 분기 커버

**입력 코드**: `var a = 1;\nvar b = 2;`  
**설정**: `setRunning()`, 브레이크포인트 없음  
**기대 동작**: 블로킹 없이 완주, 상태 RUNNING 유지

---

### TC-DBG-STATE-02 — RUNNING + 브레이크포인트 도달

**목적**
`beforeExecute()` RUNNING 상태에서 브레이크포인트 도달 시 PAUSED 전환 분기 커버

**입력 코드**: `var a = 1;\nvar b = 2;`  
**설정**: `setRunning()`, breakpoint at 줄 1, stdin `"continue\n"` 제공  
**기대 동작**: PAUSED 전환 후 stdin 명령으로 완주

---

### TC-DBG-STATE-03 ~ 04 — sourceLines_ 분기 커버 (PR #57 신규)

**목적**
`beforeExecute()` 정지 시 `sourceLines_` 우선 사용 / `currentSrcLine_` 폴백 분기 커버

| TC ID | 시나리오 | 기대 동작 |
|-------|---------|---------|
| TC-DBG-STATE-03 | `setSourceLines({"var a = 1;", ...})` 설정 | 출력에 `"var a = 1;"` 포함 |
| TC-DBG-STATE-04 | `setSourceLines()` 미호출 | 빈 `currentSrcLine_` 폴백, 정상 완주 |

---

### TC-DBG-STATE-05 — NEXT + depth > nextDepth_

**목적**
`beforeExecute()` NEXT 상태에서 `depth > nextDepth_` 조기 반환 분기 커버

**입력 코드**: `{ var a = 1; var b = 2; }`  
**설정**: `setNext(0)`, stdin `"step\nstep\ncontinue\n"` 제공  
**기대 동작**: 블록 내부 depth=1 stmt 건너뜀

---

### TC-DBG-STATE-06 — watch 자동 출력

**목적**
`beforeExecute()` 정지 시 `watches_.printWatches()` 자동 출력 분기 커버

**설정**: `addWatch("a")`, stdin `"step\ncontinue\n"` 제공  
**기대 동작**: 출력에 `"a"` 포함

---

### TC-DBG-STATE-07 ~ 09 — 명령어 파싱 분기

**목적**
`beforeExecute()` 내 명령어 파싱 분기 커버

| TC ID | stdin 입력 | 검증 항목 |
|-------|-----------|---------|
| TC-DBG-STATE-07 | `break 2\nremove 2\nbreakpoints\ncontinue\ncontinue\n` | break/remove/breakpoints 명령 처리 |
| TC-DBG-STATE-08 | `watch a\nwatches\ninspect\nstep\nunwatch a\ncontinue\n` | watch/watches/inspect/unwatch 명령 처리 |
| TC-DBG-STATE-09 | `invalidcmd\ncontinue\ncontinue\n` | `"알 수 없는"` 메시지 출력 |

---

## 커버리지 개선 결과

| 지표 | TC 추가 전 | TC 추가 후 |
|------|:---:|:---:|
| DebugController 라인 | 57% | **99.5%** |
| DebugController 브랜치 | 33% | **83%** |
| DebugController 함수 | 94% | **100%** |
| **전체 라인** | 84.99% | **90.66%** |
| **전체 브랜치** | 79.11% | **84.09%** |
