# TC_Specification_Chapter4 — 최적화(정적 바인딩 · 상수 폴딩) 테스트 케이스 명세

담당자: 심민구 (Minku Sim)  
파일: `tests/OptimizationTest.cpp`  
프레임워크: Google Test (gtest) / Google Mock (gmock)

---

## 테스트 전략

- `Checker::check(stmts)` 를 통해 AST 에 정적 바인딩 distance 주입 및 상수 폴딩 적용
- 입력: 헬퍼 함수 (`makeLit`, `makeVar`, `makeBin`, `makeVarDecl`, `makePrint`, `makeBlock`, `S`) 로 구성한 AST 노드
- 검증: `dynamic_cast` + `EXPECT_EQ` 로 노드 타입·필드값 직접 비교
- E2E TC: `Shell::runLine()` 경유 → `CaptureStdout()` 으로 표준 출력 문자열 비교
- MockEnvironment TC: gmock `MOCK_METHOD` 로 `Environment::get`/`getAt` 오버라이드 → `EXPECT_CALL` 로 호출 횟수·인자 검증 (PDF 15p 행위 검증)
- 계산 횟수 TC: `countBinaryExpr(stmts)` AST 카운터로 폴딩 전 N개 → 폴딩 후 0개 검증 (PDF 15p 요건)
- 각 TC는 **Arrange → Act → Assert** 패턴으로 구성
- 현재 상태: **Green** (SB-TC-01~11, CF-TC-01~16, MX-TC-01~14 전체 통과)

---

## TC 목록

### 정적 바인딩 (SB)

| ID | 테스트 이름 | 입력 코드 | 검증 항목 | 상태 |
|---|---|---|---|---|
| SB-TC-01 | GlobalScopeVariableDistance | `var x = 1; print x;` | `VariableExpr.distance == 0` | 🟢 Green |
| SB-TC-02 | BlockOneLevel_OuterVarDistance | `var x = 1; { print x; }` | `VariableExpr.distance == 1` | 🟢 Green |
| SB-TC-03 | BlockTwoLevel_OuterVarDistance | `var x = 1; { { print x; } }` | `VariableExpr.distance == 2` | 🟢 Green |
| SB-TC-04 | InnerBlock_ShadowingDistance | `var x = 1; { var x = 2; print x; }` | 내부 `x.distance == 0` | 🟢 Green |
| SB-TC-05 | AssignExprDistance | `var x = 1; { x = 2; }` | `AssignExpr.distance == 1` | 🟢 Green |
| SB-TC-06 | UndeclaredVariable_NegativeDistance | `print y;` | `VariableExpr.distance == -1` | 🟢 Green |
| SB-TC-07 | MultipleShadowing_NearestScope | `var x=1; { var x=2; { print x; } }` | 중간 `x.distance == 1` | 🟢 Green |
| SB-TC-08 | E2E_NestedBlock_OuterVarRead | `var x = 10; { print x; }` | stdout `"10\n"` | 🟢 Green |
| SB-TC-09 | E2E_ForLoop_OuterVarCondition | `var limit = 3; for (var i = 0; i < limit; i = i+1) { print i; }` | stdout `"0\n1\n2\n"` | 🟢 Green |
| SB-TC-10 | Mock_GlobalVar_UsesGetAt_NotGet | `var x = 1; print x;` | `getAt(0,"x")` 1회, `get` 0회 | 🟢 Green |
| SB-TC-11 | Mock_BlockVar_NoChainTraversal | `var x = 1; { print x; }` | `mock.get` 0회 | 🟢 Green |

### 상수 폴딩 (CF)

| ID | 테스트 이름 | 입력 AST / 코드 | 검증 항목 | 상태 |
|---|---|---|---|---|
| CF-TC-01 | BinaryPlus_Fold | `1.0 + 2.0` | `LiteralExpr == 3.0` | 🟢 Green |
| CF-TC-02 | BinaryMinus_Fold | `10.0 - 3.0` | `LiteralExpr == 7.0` | 🟢 Green |
| CF-TC-03 | BinaryStar_Fold | `4.0 * 5.0` | `LiteralExpr == 20.0` | 🟢 Green |
| CF-TC-04 | BinarySlash_Fold | `10.0 / 2.0` | `LiteralExpr == 5.0` | 🟢 Green |
| CF-TC-05 | BinaryLess_Fold_BoolResult | `2.0 < 5.0` | `LiteralExpr<bool> == true` | 🟢 Green |
| CF-TC-06 | UnaryMinus_Fold | `-7.0` | `LiteralExpr == -7.0` | 🟢 Green |
| CF-TC-07 | Grouping_Unwrap | `(9.0)` | `LiteralExpr == 9.0` | 🟢 Green |
| CF-TC-08 | IdentityAdd_Zero | `x + 0.0` | `VariableExpr name == "x"` | 🟢 Green |
| CF-TC-09 | AbsorptionMul_Zero | `x * 0.0` | `LiteralExpr == 0.0` | 🟢 Green |
| CF-TC-10 | IdentityMul_One | `1.0 * x` | `VariableExpr name == "x"` | 🟢 Green |
| CF-TC-11 | DivByZero_NoFold | `5.0 / 0.0` | `BinaryExpr` 유지 | 🟢 Green |
| CF-TC-12 | NestedFold | `(1.0 + 2.0) * 3.0` | `LiteralExpr == 9.0` | 🟢 Green |
| CF-TC-13 | FunctionCall_MulZero_NoFold | `f() * 0.0` | `BinaryExpr` 유지 | 🟢 Green |
| CF-TC-14 | BinaryPercent_Fold | `9.0 % 4.0` | `LiteralExpr == 1.0` | 🟢 Green |
| CF-TC-15 | PercentByZero_NoFold | `5.0 % 0.0` | `BinaryExpr` 유지 | 🟢 Green |
| CF-TC-16 | BinaryCount_NToZero | `(1+2)*(3-4)+(5*6)` | count `5 → 0`, 결과 `27.0` | 🟢 Green |

### 교차 검증 (MX)

| ID | 테스트 이름 | 입력 코드 | 검증 항목 | 상태 |
|---|---|---|---|---|
| MX-TC-01 | CF_PlusZero_SB_DistancePreserved | `var x=1.0; { var y = x+0.0; }` | CF: `x+0→VarExpr`, `x.distance==1` | 🟢 Green |
| MX-TC-02 | CF_MulOne_SB_DistancePreserved | `var x=2.0; { var y = x*1.0; }` | CF: `x*1→VarExpr`, `x.distance==1` | 🟢 Green |
| MX-TC-03 | CF_ZeroPlus_SB_DistancePreserved | `var x=3.0; { var y = 0.0+x; }` | CF: `0+x→VarExpr`, `x.distance==1` | 🟢 Green |
| MX-TC-04 | CF_PureVar_MulZero_Allowed | `var x=5.0; { var y = x*0.0; }` | `LiteralExpr == 0.0` | 🟢 Green |
| MX-TC-05 | CF_PartialFold_LeftLiteral | `var x=1.0; { var y = (2.0+3.0)+x; }` | left `== 5.0`, `x.distance==1` | 🟢 Green |
| MX-TC-06 | CF_InitFold_SB_Ref | `var a = 1.0+2.0; { print a; }` | `a.init==3.0`, `a.distance==1` | 🟢 Green |
| MX-TC-07 | CF_PlusZero_SB_Distance2 | `var x=1.0; { { var y = x+0.0; } }` | `x.distance==2` | 🟢 Green |
| MX-TC-08 | E2E_OuterVar_IdentityFold | `var x = 10; { print x + 0; }` | stdout `"10\n"` | 🟢 Green |
| MX-TC-09 | E2E_FoldedInit_Print | `var a = 1 + 2; print a;` | stdout `"3\n"` | 🟢 Green |
| MX-TC-10 | E2E_TwoFoldedVars_Sum | `var a = 1+2; var b = 3; print a+b;` | stdout `"6\n"` | 🟢 Green |
| MX-TC-11 | E2E_MulOne_Identity | `var x = 4; print x * 1;` | stdout `"4\n"` | 🟢 Green |
| MX-TC-12 | E2E_FoldedLimit_ForLoop | `var limit = 2+1; for (var i=0; i<limit; i=i+1) { print i; }` | stdout `"0\n1\n2\n"` | 🟢 Green |
| MX-TC-13 | E2E_PureVar_MulZero | `var x = 3; { var y = x * 0; print y; }` | stdout `"0\n"` | 🟢 Green |
| MX-TC-14 | E2E_PartialFold_RuntimeEval | `var x = 5; print (2+3) * x;` | stdout `"25\n"` | 🟢 Green |

---

## TC 상세

### SB-TC-01 GlobalScopeVariableDistance

**목적**: 글로벌 스코프에서 선언된 변수를 참조할 때 `distance == 0` 으로 기록되는지 확인

**입력 AST**
```
VarDeclareStmt("x", LiteralExpr(1.0))
PrintStmt(VariableExpr("x"))
```

**기대 결과**
```
VariableExpr("x").distance == 0
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeVarDecl("x", makeLit(1.0))`, `makePrint(makeVar("x"))` 로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `PrintStmt` 내 `VariableExpr.distance` == `0` |

---

### SB-TC-02 BlockOneLevel_OuterVarDistance

**목적**: 1단계 블록 안에서 외부 스코프 변수를 참조할 때 `distance == 1` 로 기록되는지 확인

**입력 AST**
```
VarDeclareStmt("x", LiteralExpr(1.0))
BlockStmt {
    PrintStmt(VariableExpr("x"))
}
```

**기대 결과**
```
VariableExpr("x").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeVarDecl`, `makeBlock(S(makePrint(makeVar("x"))))` 로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `BlockStmt` 내 `VariableExpr.distance` == `1` |

---

### SB-TC-03 BlockTwoLevel_OuterVarDistance

**목적**: 2단계 중첩 블록에서 최외부 스코프 변수를 참조할 때 `distance == 2` 로 기록되는지 확인

**입력 AST**
```
VarDeclareStmt("x", LiteralExpr(1.0))
BlockStmt {
    BlockStmt {
        PrintStmt(VariableExpr("x"))
    }
}
```

**기대 결과**
```
VariableExpr("x").distance == 2
```

| 단계 | 내용 |
|---|---|
| Arrange | 2단계 중첩 `makeBlock` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 내부 `VariableExpr.distance` == `2` |

---

### SB-TC-04 InnerBlock_ShadowingDistance

**목적**: 블록 내부에서 동명 변수를 재선언할 때 내부 선언이 `distance == 0` (자기 스코프) 으로 바인딩되는지 확인

**입력 AST**
```
VarDeclareStmt("x", LiteralExpr(1.0))
BlockStmt {
    VarDeclareStmt("x", LiteralExpr(2.0))
    PrintStmt(VariableExpr("x"))
}
```

**기대 결과**
```
내부 VariableExpr("x").distance == 0
```

| 단계 | 내용 |
|---|---|
| Arrange | 외부·내부 각각 `var x` 선언 포함 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 블록 내 `VariableExpr.distance` == `0` (섀도잉 확인) |

---

### SB-TC-05 AssignExprDistance

**목적**: 블록 안에서 외부 스코프 변수에 대입할 때 `AssignExpr.distance == 1` 로 기록되는지 확인

**입력 AST**
```
VarDeclareStmt("x", LiteralExpr(1.0))
BlockStmt {
    ExpressionStmt(AssignExpr("x", LiteralExpr(2.0)))
}
```

**기대 결과**
```
AssignExpr("x").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | 외부 `var x`, 블록 내 `x = 2` 대입식 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `AssignExpr.distance` == `1` |

---

### SB-TC-06 UndeclaredVariable_NegativeDistance

**목적**: 선언되지 않은 변수를 참조할 때 `distance == -1` (탐색 실패) 로 기록되는지 확인

**입력 AST**
```
PrintStmt(VariableExpr("y"))
```

**기대 결과**
```
VariableExpr("y").distance == -1
```

| 단계 | 내용 |
|---|---|
| Arrange | 선언 없이 `makePrint(makeVar("y"))` 만 포함한 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `VariableExpr.distance` == `-1` |

---

### SB-TC-07 MultipleShadowing_NearestScope

**목적**: 두 단계 섀도잉 상황에서 가장 가까운 스코프의 변수가 바인딩되는지 (`distance == 1`) 확인

**입력 AST**
```
VarDeclareStmt("x", LiteralExpr(1.0))
BlockStmt {
    VarDeclareStmt("x", LiteralExpr(2.0))
    BlockStmt {
        PrintStmt(VariableExpr("x"))
    }
}
```

**기대 결과**
```
가장 안쪽 VariableExpr("x").distance == 1  // 중간 스코프 x 바인딩
```

| 단계 | 내용 |
|---|---|
| Arrange | 외부·중간·내부 3단계 스코프 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 내부 `VariableExpr.distance` == `1` |

---

### SB-TC-08 E2E_NestedBlock_OuterVarRead

**목적**: 중첩 블록에서 외부 변수를 읽어 출력할 때 런타임 결과가 정확한지 E2E 검증

**입력 코드**
```
var x = 10; { print x; }
```

**기대 출력**
```
10
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine("var x = 10; { print x; }")` 호출 |
| Assert | 캡처된 stdout == `"10\n"` |

---

### SB-TC-09 E2E_ForLoop_OuterVarCondition

**목적**: for 루프 조건식에서 외부 스코프 변수를 참조할 때 정적 바인딩이 정확히 동작하는지 E2E 검증

**입력 코드**
```
var limit = 3; for (var i = 0; i < limit; i = i+1) { print i; }
```

**기대 출력**
```
0
1
2
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | for 루프 코드 `runLine()` 호출 |
| Assert | 캡처된 stdout == `"0\n1\n2\n"` |

---

### SB-TC-10 Mock_GlobalVar_UsesGetAt_NotGet

**목적**: 글로벌 스코프 변수 참조 시 `getAt(0, "x")` 가 호출되고 `get()` 이 호출되지 않음을 검증 — PDF 15p "스코프를 거슬러 올라가지 않고 즉시 접근" 행위 증명

**입력 코드**
```
var x = 1; print x;
```

**기대 동작**
```
mock.getAt(0, "x") → 1회 호출
mock.get(_, _)      → 0회 호출
```

| 단계 | 내용 |
|---|---|
| Arrange | `MockEnvironment` 생성, `ON_CALL(getAt/get)` 에 실제 구현 델리게이트, `EXPECT_CALL` 설정 |
| Act | `checker.check(stmts)` → `exec.execute(stmts, mock)` 호출 |
| Assert | gmock 검증: `getAt(0,"x")` Times(1), `get` Times(0) |

**🟢 Green**: `Executor::evaluateExpr` 의 `VariableExpr` 분기에서 `distance >= 0` 이면 `env->getAt(distance, name)` 직접 호출

---

### SB-TC-11 Mock_BlockVar_NoChainTraversal

**목적**: 블록 스코프에서 외부 변수를 참조할 때도 `get()` (체인 순회) 이 호출되지 않음을 검증

**입력 코드**
```
var x = 1; { print x; }
```

**기대 동작**
```
mock.get(_, _) → 0회 호출
```

| 단계 | 내용 |
|---|---|
| Arrange | `MockEnvironment` 생성, `ON_CALL(get)` 에 실제 구현 델리게이트, `EXPECT_CALL get Times(0)` 설정 |
| Act | `checker.check(stmts)` → `exec.execute(stmts, mock)` 호출 |
| Assert | gmock 검증: `get` Times(0) |

> 블록 내부 `blockEnv.getAt(1, "x")` 는 `mock` 이 아닌 `blockEnv` 에서 직접 호출 → `mock.getAt` 은 검증 대상 외. `mock.get` Times(0) 으로 체인 순회 부재를 간접 검증.

---

### CF-TC-01 BinaryPlus_Fold

**목적**: 두 숫자 리터럴의 `+` 연산이 단일 `LiteralExpr` 로 폴딩되는지 확인

**입력 AST**
```
BinaryExpr(PLUS, LiteralExpr(1.0), LiteralExpr(2.0))
```

**기대 결과**
```
LiteralExpr(3.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(1.0), PLUS, makeLit(2.0))` 을 초기화식으로 하는 `VarDeclareStmt` AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `LiteralExpr<double>` 이고 값 == `3.0` |

---

### CF-TC-02 BinaryMinus_Fold

**목적**: 두 숫자 리터럴의 `-` 연산이 폴딩되는지 확인

**입력 AST**
```
BinaryExpr(MINUS, LiteralExpr(10.0), LiteralExpr(3.0))
```

**기대 결과**
```
LiteralExpr(7.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(10.0), MINUS, makeLit(3.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<double>` 이고 값 == `7.0` |

---

### CF-TC-03 BinaryStar_Fold

**목적**: 두 숫자 리터럴의 `*` 연산이 폴딩되는지 확인

**입력 AST**
```
BinaryExpr(STAR, LiteralExpr(4.0), LiteralExpr(5.0))
```

**기대 결과**
```
LiteralExpr(20.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(4.0), STAR, makeLit(5.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<double>` 이고 값 == `20.0` |

---

### CF-TC-04 BinarySlash_Fold

**목적**: 두 숫자 리터럴의 `/` 연산이 폴딩되는지 확인

**입력 AST**
```
BinaryExpr(SLASH, LiteralExpr(10.0), LiteralExpr(2.0))
```

**기대 결과**
```
LiteralExpr(5.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(10.0), SLASH, makeLit(2.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<double>` 이고 값 == `5.0` |

---

### CF-TC-05 BinaryLess_Fold_BoolResult

**목적**: 비교 연산 `<` 가 폴딩될 때 결과 타입이 `bool` 인 `LiteralExpr` 로 교체되는지 확인

**입력 AST**
```
BinaryExpr(LESS, LiteralExpr(2.0), LiteralExpr(5.0))
```

**기대 결과**
```
LiteralExpr<bool>(true)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(2.0), LESS, makeLit(5.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<bool>` 이고 값 == `true` |

---

### CF-TC-06 UnaryMinus_Fold

**목적**: 단항 `-` 연산자가 숫자 리터럴에 적용될 때 폴딩되는지 확인

**입력 AST**
```
UnaryExpr(MINUS, LiteralExpr(7.0))
```

**기대 결과**
```
LiteralExpr(-7.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeUnary(MINUS, makeLit(7.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<double>` 이고 값 == `-7.0` |

---

### CF-TC-07 Grouping_Unwrap

**목적**: 괄호 안이 리터럴일 때 `GroupingExpr` 가 제거되고 내부 `LiteralExpr` 가 노출되는지 확인

**입력 AST**
```
GroupingExpr(LiteralExpr(9.0))
```

**기대 결과**
```
LiteralExpr(9.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeGrouping(makeLit(9.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `LiteralExpr<double>` 이고 값 == `9.0` (`GroupingExpr` 아님) |

---

### CF-TC-08 IdentityAdd_Zero

**목적**: `x + 0.0` 항등식에서 `0.0` 덧셈이 제거되고 `VariableExpr(x)` 만 남는지 확인

**입력 AST**
```
BinaryExpr(PLUS, VariableExpr("x"), LiteralExpr(0.0))
```

**기대 결과**
```
VariableExpr("x")
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeVar("x"), PLUS, makeLit(0.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `VariableExpr` 이고 `name.lexeme` == `"x"` |

---

### CF-TC-09 AbsorptionMul_Zero

**목적**: 순수 변수 `x * 0.0` 흡수 법칙에서 결과가 `LiteralExpr(0.0)` 으로 교체되는지 확인

**입력 AST**
```
BinaryExpr(STAR, VariableExpr("x"), LiteralExpr(0.0))
```

**기대 결과**
```
LiteralExpr(0.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeVar("x"), STAR, makeLit(0.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `LiteralExpr<double>` 이고 값 == `0.0` |

---

### CF-TC-10 IdentityMul_One

**목적**: `1.0 * x` 항등식에서 `1.0` 곱셈이 제거되고 `VariableExpr(x)` 만 남는지 확인

**입력 AST**
```
BinaryExpr(STAR, LiteralExpr(1.0), VariableExpr("x"))
```

**기대 결과**
```
VariableExpr("x")
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(1.0), STAR, makeVar("x"))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `VariableExpr` 이고 `name.lexeme` == `"x"` |

---

### CF-TC-11 DivByZero_NoFold

**목적**: `5.0 / 0.0` 은 런타임 에러이므로 폴딩하지 않고 `BinaryExpr` 를 유지하는지 확인

**입력 AST**
```
BinaryExpr(SLASH, LiteralExpr(5.0), LiteralExpr(0.0))
```

**기대 결과**
```
BinaryExpr (LiteralExpr 로 교체되지 않음)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(5.0), SLASH, makeLit(0.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `BinaryExpr` 임을 `dynamic_cast<BinaryExpr*> != nullptr` 로 확인 |

> 0 나누기 결과는 런타임 에러 — 컴파일 타임에 100% 확정 불가 (`foldExpr` 가 제로 제수를 폴딩 제외)

---

### CF-TC-12 NestedFold

**목적**: `(1.0 + 2.0) * 3.0` 처럼 중첩된 리터럴 연산이 재귀적으로 폴딩되어 단일 `LiteralExpr` 로 교체되는지 확인

**입력 AST**
```
BinaryExpr(STAR,
    GroupingExpr(BinaryExpr(PLUS, LiteralExpr(1.0), LiteralExpr(2.0))),
    LiteralExpr(3.0))
```

**기대 결과**
```
LiteralExpr(9.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | 중첩 `makeBin` / `makeGrouping` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<double>` 이고 값 == `9.0` |

---

### CF-TC-13 FunctionCall_MulZero_NoFold

**목적**: `f() * 0.0` 에서 `f()` 는 사이드 이펙트를 가질 수 있으므로 흡수 법칙을 적용하지 않고 `BinaryExpr` 를 유지하는지 확인

**입력 AST**
```
BinaryExpr(STAR, FunctionCallExpr("f", []), LiteralExpr(0.0))
```

**기대 결과**
```
BinaryExpr (LiteralExpr 로 교체되지 않음)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeCall("f"), STAR, makeLit(0.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `BinaryExpr` 임을 확인 |

> `isPure()` 가드: `FunctionCallExpr` / `AssignExpr` 는 사이드 이펙트 보유 → `x * 0 → 0` 패턴 적용 불가

---

### CF-TC-14 BinaryPercent_Fold

**목적**: 두 숫자 리터럴의 `%` 나머지 연산이 폴딩되는지 확인

**입력 AST**
```
BinaryExpr(PERCENT, LiteralExpr(9.0), LiteralExpr(4.0))
```

**기대 결과**
```
LiteralExpr(1.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(9.0), PERCENT, makeLit(4.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `LiteralExpr<double>` 이고 값 == `1.0` |

**🟢 Green**: PR #39 에서 `PERCENT` 연산자 추가 후 `foldExpr` 의 리터럴 이항 분기에서 `std::fmod` 로 처리

---

### CF-TC-15 PercentByZero_NoFold

**목적**: `5.0 % 0.0` 은 런타임 에러이므로 폴딩하지 않고 `BinaryExpr` 를 유지하는지 확인

**입력 AST**
```
BinaryExpr(PERCENT, LiteralExpr(5.0), LiteralExpr(0.0))
```

**기대 결과**
```
BinaryExpr (LiteralExpr 로 교체되지 않음)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(5.0), PERCENT, makeLit(0.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 초기화식이 `BinaryExpr` 임을 확인 |

---

### CF-TC-16 BinaryCount_NToZero

**목적**: `(1+2)*(3-4)+(5*6)` — 5개 `BinaryExpr` 가 폴딩 후 전부 `LiteralExpr` 로 교체되어 `countBinaryExpr == 0` 이 되는지 확인 (PDF 15p "계산 횟수 N→0" 요건)

**입력 AST**
```
BinaryExpr(PLUS,
    BinaryExpr(STAR,
        BinaryExpr(PLUS,  LiteralExpr(1), LiteralExpr(2)),
        BinaryExpr(MINUS, LiteralExpr(3), LiteralExpr(4))),
    BinaryExpr(STAR, LiteralExpr(5), LiteralExpr(6)))
```

**기대 결과**
```
폴딩 전: countBinaryExpr == 5
폴딩 후: countBinaryExpr == 0, 결과 LiteralExpr(27.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | 위 5-노드 BinaryExpr 트리로 AST 구성 |
| Act | `EXPECT_EQ(countBinaryExpr(stmts), 5)` 확인 후 `checker.check(stmts)` 호출 |
| Assert | `countBinaryExpr(stmts) == 0`, 초기화식 `LiteralExpr == 27.0` |

**🟢 Green**: `foldExpr` 재귀 호출로 모든 리터럴 이항 노드가 일괄 교체됨

---

### MX-TC-01 CF_PlusZero_SB_DistancePreserved

**목적**: `x + 0.0` CF 폴딩 후 `VariableExpr(x)` 로 교체됐을 때 SB `distance == 1` 이 보존되는지 확인

**입력 코드**
```
var x = 1.0; { var y = x + 0.0; }
```

**기대 결과**
```
y.initializer → VariableExpr("x")
VariableExpr("x").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeVarDecl("x", makeLit(1.0))`, 블록 내 `makeVarDecl("y", makeBin(makeVar("x"), PLUS, makeLit(0.0)))` AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `y` 초기화식이 `VariableExpr("x")` 이고 `distance == 1` |

---

### MX-TC-02 CF_MulOne_SB_DistancePreserved

**목적**: `x * 1.0` CF 폴딩 후 `VariableExpr(x)` 로 교체됐을 때 SB `distance == 1` 이 보존되는지 확인

**입력 코드**
```
var x = 2.0; { var y = x * 1.0; }
```

**기대 결과**
```
y.initializer → VariableExpr("x")
VariableExpr("x").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | 블록 내 `makeBin(makeVar("x"), STAR, makeLit(1.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `y` 초기화식이 `VariableExpr("x")` 이고 `distance == 1` |

---

### MX-TC-03 CF_ZeroPlus_SB_DistancePreserved

**목적**: `0.0 + x` CF 폴딩 후 `VariableExpr(x)` 로 교체됐을 때 SB `distance == 1` 이 보존되는지 확인

**입력 코드**
```
var x = 3.0; { var y = 0.0 + x; }
```

**기대 결과**
```
y.initializer → VariableExpr("x")
VariableExpr("x").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeLit(0.0), PLUS, makeVar("x"))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `y` 초기화식이 `VariableExpr("x")` 이고 `distance == 1` |

---

### MX-TC-04 CF_PureVar_MulZero_Allowed

**목적**: 순수 변수 `x * 0.0` 은 사이드 이펙트 없으므로 CF 가 `LiteralExpr(0.0)` 으로 교체 허용되는지 확인

**입력 코드**
```
var x = 5.0; { var y = x * 0.0; }
```

**기대 결과**
```
y.initializer → LiteralExpr(0.0)
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeVar("x"), STAR, makeLit(0.0))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `y` 초기화식이 `LiteralExpr<double>` 이고 값 == `0.0` |

---

### MX-TC-05 CF_PartialFold_LeftLiteral

**목적**: `(2.0 + 3.0) + x` 에서 좌측 리터럴쌍만 폴딩되고 외부 Binary 는 유지되며 SB distance 가 보존되는지 확인

**입력 코드**
```
var x = 1.0; { var y = (2.0 + 3.0) + x; }
```

**기대 결과**
```
y.initializer → BinaryExpr(PLUS)
    left:  LiteralExpr(5.0)
    right: VariableExpr("x").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeBin(makeBin(makeLit(2.0), PLUS, makeLit(3.0)), PLUS, makeVar("x"))` 으로 AST 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | 외부 `BinaryExpr` 유지, 좌측 `LiteralExpr == 5.0`, 우측 `VariableExpr.distance == 1` |

---

### MX-TC-06 CF_InitFold_SB_Ref

**목적**: 변수 초기화식이 폴딩되고 그 변수를 블록에서 참조할 때 SB distance 가 올바른지 확인

**입력 코드**
```
var a = 1.0 + 2.0; { print a; }
```

**기대 결과**
```
a.initializer → LiteralExpr(3.0)
PrintStmt 내 VariableExpr("a").distance == 1
```

| 단계 | 내용 |
|---|---|
| Arrange | `makeVarDecl("a", makeBin(makeLit(1.0), PLUS, makeLit(2.0)))` + 블록 내 `makePrint(makeVar("a"))` 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `a` 초기화식 == `LiteralExpr(3.0)`, `a.distance` == `1` |

---

### MX-TC-07 CF_PlusZero_SB_Distance2

**목적**: 2단계 중첩 블록에서 `x + 0.0` 폴딩 후 `VariableExpr(x).distance == 2` 가 보존되는지 확인

**입력 코드**
```
var x = 1.0; { { var y = x + 0.0; } }
```

**기대 결과**
```
y.initializer → VariableExpr("x")
VariableExpr("x").distance == 2
```

| 단계 | 내용 |
|---|---|
| Arrange | 2단계 중첩 `makeBlock` 내 `makeBin(makeVar("x"), PLUS, makeLit(0.0))` 구성 |
| Act | `checker.check(stmts)` 호출 |
| Assert | `y` 초기화식이 `VariableExpr("x")` 이고 `distance == 2` |

---

### MX-TC-08 E2E_OuterVar_IdentityFold

**목적**: 외부 변수에 항등 폴딩이 적용된 후 런타임 결과가 정확한지 E2E 검증

**입력 코드**
```
var x = 10; { print x + 0; }
```

**기대 출력**
```
10
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"10\n"` |

---

### MX-TC-09 E2E_FoldedInit_Print

**목적**: 초기화식이 폴딩된 변수를 출력할 때 런타임 결과가 정확한지 E2E 검증

**입력 코드**
```
var a = 1 + 2; print a;
```

**기대 출력**
```
3
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"3\n"` |

---

### MX-TC-10 E2E_TwoFoldedVars_Sum

**목적**: 두 폴딩 변수를 합산 출력할 때 런타임 결과가 정확한지 E2E 검증

**입력 코드**
```
var a = 1+2; var b = 3; print a+b;
```

**기대 출력**
```
6
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"6\n"` |

---

### MX-TC-11 E2E_MulOne_Identity

**목적**: `x * 1` 항등 폴딩 후 런타임 결과가 정확한지 E2E 검증

**입력 코드**
```
var x = 4; print x * 1;
```

**기대 출력**
```
4
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"4\n"` |

---

### MX-TC-12 E2E_FoldedLimit_ForLoop

**목적**: 폴딩된 limit 변수로 for 루프를 돌릴 때 런타임 결과가 정확한지 E2E 검증

**입력 코드**
```
var limit = 2+1; for (var i=0; i<limit; i=i+1) { print i; }
```

**기대 출력**
```
0
1
2
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"0\n1\n2\n"` |

---

### MX-TC-13 E2E_PureVar_MulZero

**목적**: 순수 변수 `x * 0` 폴딩 후 출력이 `0` 인지 E2E 검증

**입력 코드**
```
var x = 3; { var y = x * 0; print y; }
```

**기대 출력**
```
0
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"0\n"` |

---

### MX-TC-14 E2E_PartialFold_RuntimeEval

**목적**: 부분 폴딩 후 나머지 연산이 런타임에 정확히 평가되는지 E2E 검증

**입력 코드**
```
var x = 5; print (2+3) * x;
```

**기대 출력**
```
25
```

| 단계 | 내용 |
|---|---|
| Arrange | `Shell` 초기화, stdout 캡처 준비 |
| Act | `shell.runLine(...)` 호출 |
| Assert | 캡처된 stdout == `"25\n"` |

---

## 요약

| 분류 | AST 검사 | E2E | MockEnvironment | 합계 |
|------|---------|-----|----------------|-----|
| 정적 바인딩 (SB) | 7 | 2 | 2 | **11** |
| 상수 폴딩 (CF) | 16 | 0 | 0 | **16** |
| 교차 검증 (MX) | 7 | 7 | 0 | **14** |
| **총계** | **30** | **9** | **2** | **41** |
