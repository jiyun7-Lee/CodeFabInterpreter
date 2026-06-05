# Chapter 4 테스트케이스 명세서

대상 브랜치: `feature/Optimization`  
대상 파일: `tests/OptimizationTest.cpp`  
검증 범위: 정적 바인딩(SB) · 상수 폴딩(CF) · 교차 검증(MX)

---

## 테스트 방식 공통 사항

| 구분 | 방식 |
|------|------|
| **AST 검사 TC** | `Checker`에 AST를 직접 주입 → `check()` 후 노드 타입·필드 값을 `dynamic_cast` + `EXPECT_EQ`로 검증 |
| **E2E TC** | `Shell::runLine()` 경유 → `CaptureStdout()`으로 표준 출력 문자열 비교 |
| **테스트 픽스처** | `MockEnvironment` 없음 — `Checker` 직접 생성. 필요한 AST 노드는 헬퍼 함수(`makeLit`, `makeVar`, `makeBin` 등)로 생성 |

---

## 정적 바인딩 (SB) — 9개

목적: `Checker`가 `VariableExpr`/`AssignExpr`의 `distance` 필드를 올바르게 기록하는지 확인.

### AST 검사 (7개)

| TC ID | 테스트명 | 입력 코드 | 검증 항목 | 기대값 |
|-------|---------|----------|----------|--------|
| SB-TC-01 | 글로벌 스코프 변수 참조 | `var x = 1; print x;` | `VariableExpr.distance` | `0` |
| SB-TC-02 | 블록 1단계 외부 변수 참조 | `var x = 1; { print x; }` | `VariableExpr.distance` | `1` |
| SB-TC-03 | 블록 2단계 외부 변수 참조 | `var x = 1; { { print x; } }` | `VariableExpr.distance` | `2` |
| SB-TC-04 | 내부 블록 섀도잉 | `var x = 1; { var x = 2; print x; }` | 내부 `x`의 `distance` | `0` (자기 스코프) |
| SB-TC-05 | `AssignExpr` distance 검증 | `var x = 1; { x = 2; }` | `AssignExpr.distance` | `1` |
| SB-TC-06 | 미선언 변수 | `print y;` | `VariableExpr.distance` | `-1` (탐색 실패) |
| SB-TC-07 | 다중 섀도잉 — 가장 가까운 스코프 | `var x=1; { var x=2; { print x; } }` | 내부에서 중간 `x`의 `distance` | `1` |

### E2E (2개)

| TC ID | 테스트명 | 입력 코드 | 기대 출력 |
|-------|---------|----------|----------|
| SB-TC-08 | 중첩 블록에서 외부 변수 읽기 | `var x = 10; { print x; }` | `10` |
| SB-TC-09 | for 루프 조건에서 외부 변수 참조 | `var limit = 3; for (var i = 0; i < limit; i = i+1) { print i; }` | `0\n1\n2` |

---

## 상수 폴딩 (CF) — 13개

목적: `foldExpr()` 호출 후 AST 노드가 `LiteralExpr`로 교체되거나 항등식으로 단순화됐는지 확인.

### 패턴별 분류

| 패턴 | 설명 |
|------|------|
| **A** | 양쪽 리터럴 이항 연산 → 단일 리터럴 |
| **B** | 단항 연산자 + 리터럴 → 단일 리터럴 |
| **C** | 괄호(`GroupingExpr`) 안이 리터럴 → 괄호 제거 |
| **D** | 항등식·흡수 법칙 (`+0`, `*1`, `*0`) |

### AST 검사 (13개)

| TC ID | 테스트명 | 패턴 | 입력 AST | 기대 노드 | 기대값 |
|-------|---------|------|---------|----------|--------|
| CF-TC-01 | 이항 `+` 폴딩 | A | `1.0 + 2.0` | `LiteralExpr<double>` | `3.0` |
| CF-TC-02 | 이항 `-` 폴딩 | A | `10.0 - 3.0` | `LiteralExpr<double>` | `7.0` |
| CF-TC-03 | 이항 `*` 폴딩 | A | `4.0 * 5.0` | `LiteralExpr<double>` | `20.0` |
| CF-TC-04 | 이항 `/` 폴딩 | A | `10.0 / 2.0` | `LiteralExpr<double>` | `5.0` |
| CF-TC-05 | 이항 `<` 폴딩 (bool 결과) | A | `2.0 < 5.0` | `LiteralExpr<bool>` | `true` |
| CF-TC-06 | 단항 `-` 폴딩 | B | `-7.0` | `LiteralExpr<double>` | `-7.0` |
| CF-TC-07 | 괄호 언랩 | C | `(9.0)` | `LiteralExpr<double>` | `9.0` |
| CF-TC-08 | `x + 0.0` 항등 제거 | D | `x + 0.0` | `VariableExpr` | `name == "x"` |
| CF-TC-09 | `x * 0.0` 흡수 | D | `x * 0.0` (x는 VariableExpr) | `LiteralExpr<double>` | `0.0` |
| CF-TC-10 | `1.0 * x` 항등 제거 | D | `1.0 * x` | `VariableExpr` | `name == "x"` |
| CF-TC-11 | 0 나누기 비폴딩 | A (예외) | `5.0 / 0.0` | `BinaryExpr` (유지) | `LiteralExpr`가 아님 |
| CF-TC-12 | 중첩 폴딩 | A+A | `(1.0 + 2.0) * 3.0` | `LiteralExpr<double>` | `9.0` |
| CF-TC-13 | 함수 호출 `* 0.0` 비폴딩 | D (예외) | `f() * 0.0` | `BinaryExpr` (유지) | 사이드 이펙트 보호 |

> **CF-TC-11, CF-TC-13 음성 케이스 근거**  
> - CF-TC-11: 스펙 "런타임 이전에 100% 확정 가능한 경우만 최적화" — `0` 나누기 결과는 런타임 의존적  
> - CF-TC-13: `isPure()` 가드 — `FunctionCallExpr`/`AssignExpr`는 사이드 이펙트 보유, `x*0→0` 패턴 D 적용 불가

---

## 교차 검증 (MX) — 14개

목적: CF가 노드를 교체한 뒤에도 SB `distance`가 보존되는지, 두 최적화가 동시에 적용됐을 때 런타임 결과가 정확한지 확인.

### AST 검사 (7개)

| TC ID | 테스트명 | 시나리오 | CF 결과 | SB 검증 항목 |
|-------|---------|---------|---------|------------|
| MX-TC-01 | `x+0` 폴딩 후 distance 보존 | `var x=1.0; { var y = x+0.0; }` | `x+0 → VariableExpr{x}` | `x.distance == 1` |
| MX-TC-02 | `x*1` 폴딩 후 distance 보존 | `var x=2.0; { var y = x*1.0; }` | `x*1 → VariableExpr{x}` | `x.distance == 1` |
| MX-TC-03 | `0+x` 폴딩 후 distance 보존 | `var x=3.0; { var y = 0.0+x; }` | `0+x → VariableExpr{x}` | `x.distance == 1` |
| MX-TC-04 | 순수 변수 `x*0` 폴딩 허용 | `var x=5.0; { var y = x*0.0; }` | `x*0 → LiteralExpr{0.0}` | 폴딩 발생 확인 |
| MX-TC-05 | 부분 폴딩 — `(2+3)+x` | `var x=1.0; { var y = (2.0+3.0)+x; }` | `2+3 → 5.0`, 외부 Binary 유지 | `x.distance == 1`, `left == 5.0` |
| MX-TC-06 | 초기화식 폴딩 + 참조 distance | `var a = 1.0+2.0; { print a; }` | `a.init → 3.0` | `a.distance == 1` |
| MX-TC-07 | 2단계 중첩 — `x+0` 폴딩 후 distance=2 | `var x=1.0; { { var y = x+0.0; } }` | `x+0 → VariableExpr{x}` | `x.distance == 2` |

### E2E (7개)

| TC ID | 테스트명 | 입력 코드 | 기대 출력 |
|-------|---------|----------|----------|
| MX-TC-08 | 외부 변수 + 항등 폴딩 E2E | `var x = 10; { print x + 0; }` | `10` |
| MX-TC-09 | 폴딩된 초기화식 출력 | `var a = 1 + 2; print a;` | `3` |
| MX-TC-10 | 폴딩 변수 둘의 합산 | `var a = 1+2; var b = 3; print a+b;` | `6` |
| MX-TC-11 | `x*1` 항등 폴딩 E2E | `var x = 4; print x * 1;` | `4` |
| MX-TC-12 | 폴딩된 limit으로 for 루프 | `var limit = 2+1; for (var i=0; i<limit; i=i+1) { print i; }` | `0\n1\n2` |
| MX-TC-13 | 순수 변수 `x*0` 폴딩 E2E | `var x = 3; { var y = x * 0; print y; }` | `0` |
| MX-TC-14 | 부분 폴딩 후 런타임 평가 | `var x = 5; print (2+3) * x;` | `25` |

---

## 요약

| 분류 | AST 검사 | E2E | 합계 |
|------|---------|-----|-----|
| 정적 바인딩 (SB) | 7 | 2 | **9** |
| 상수 폴딩 (CF) | 13 | 0 | **13** |
| 교차 검증 (MX) | 7 | 7 | **14** |
| **총계** | **27** | **9** | **36** |
