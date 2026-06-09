# 네거티브 테스트 케이스 명세서

## 대상 파일

`tests/negativeTest.cpp`

---

## 테스트 방식 공통 사항

| 구분 | 방식 |
|------|------|
| **Shell (REPL) 모드** | `Shell::runLine(소스)` — 한 번에 소스 전달 |
| **File 모드** | `FileRunner::runSource(줄 배열)` — 줄 단위 배열 전달 |
| **Debug 모드** | `DebugShell::runSource(줄 배열, ctrl)` + `NonBlockingController` (stdin 없이 자동 step) |
| **에러 검증** | `EXPECT_NE(출력.find("[Error]" or "[Checker]"), npos)` |

> **File / Debug 모드 격리 이유**  
> 에러 발생 시 해당 소스 실행이 즉시 중단된다. 케이스를 한 파일에 묶으면  
> 앞 케이스의 에러로 뒤 케이스가 실행되지 않는다.  
> 따라서 각 케이스를 독립 `TEST_F` 로 분리하고, 픽스처도 매 TC 마다 새로 생성한다.

---

## 케이스 목록

### 1. Parser 에러

| TC ID | 테스트 함수명 | 입력 코드 | 기대 포함 문자열 | 상태 |
|-------|--------------|-----------|----------------|------|
| NT_04 | NT_04_{Shell\|File\|Debug}_MissingSemicolon | `print 1 + 2` | `[Error]` | 🟢 Green |
| NT_05 | NT_05_{Shell\|File\|Debug}_MissingClosingParen | `print (1 + 2;` | `[Error]` | 🟢 Green |
| NT_06 | NT_06_{Shell\|File\|Debug}_InvalidAssignmentTarget | `var a=1; var b=2; a+b=3;` | `[Error]` | 🟢 Green |
| NT_07 | NT_07_{Shell\|File\|Debug}_UnexpectedToken | `print * 5;` | `[Error]` | 🟢 Green |

### 2. Checker 에러 (정적 분석)

| TC ID | 테스트 함수명 | 입력 코드 | 기대 포함 문자열 | 상태 |
|-------|--------------|-----------|----------------|------|
| NT_08 | NT_08_{Shell\|File\|Debug}_SelfInitReference | `{ var a = a; }` | `[Checker]` | 🟢 Green |
| NT_09 | NT_09_{Shell\|File\|Debug}_DuplicateDeclaration | `{ var a = "hi"; var a = 3; }` | `[Checker]` | 🟢 Green |

### 3. Runtime 에러

| TC ID | 테스트 함수명 | 입력 코드 | 기대 포함 문자열 | 상태 |
|-------|--------------|-----------|----------------|------|
| NT_01 | NT_01_{Shell\|File\|Debug}_UndefinedVariable | `print notDefined;` | `[Error]` | 🟢 Green |
| NT_02 | NT_02_{Shell\|File\|Debug}_TypeMismatchPlus | `print 1 + "HI";` | `[Error]` | 🟢 Green |
| NT_03 | NT_03_{Shell\|File\|Debug}_UnaryMinusTypeMismatch | `print -"FabCoding";` | `[Error]` | 🟢 Green |

---

## 케이스별 상세

---

### NT_01 — 정의되지 않은 변수 참조 (Runtime)

**목적**  
선언하지 않은 변수를 `print` 할 때 런타임 에러가 발생하는지 확인

**입력 코드**
```
print notDefined;
```

**에러 발생 시점** : Executor — 환경에서 변수를 찾지 못할 때  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_01_Shell_UndefinedVariable` |
| File | `NT_01_File_UndefinedVariable` |
| Debug | `NT_01_Debug_UndefinedVariable` |

---

### NT_02 — `+` 연산자 숫자/문자열 혼용 (Runtime)

**목적**  
`+` 연산자 양쪽의 피연산자 타입이 다를 때 런타임 타입 에러가 발생하는지 확인

**입력 코드**
```
print 1 + "HI";
```

**에러 발생 시점** : Executor `visitBinary` — 피연산자가 모두 숫자이거나 모두 문자열이어야 하는 조건 위반  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_02_Shell_TypeMismatchPlus` |
| File | `NT_02_File_TypeMismatchPlus` |
| Debug | `NT_02_Debug_TypeMismatchPlus` |

---

### NT_03 — 단항 마이너스 피연산자 타입 오류 (Runtime)

**목적**  
문자열에 단항 `-` 를 적용할 때 런타임 타입 에러가 발생하는지 확인

**입력 코드**
```
print -"FabCoding";
```

**에러 발생 시점** : Executor `visitUnary` — 피연산자가 숫자여야 하는 조건 위반  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_03_Shell_UnaryMinusTypeMismatch` |
| File | `NT_03_File_UnaryMinusTypeMismatch` |
| Debug | `NT_03_Debug_UnaryMinusTypeMismatch` |

---

### NT_04 — 세미콜론 누락 (Parser)

**목적**  
`print` 문 끝에 `;` 가 없을 때 파서 에러가 발생하는지 확인

**입력 코드**
```
print 1 + 2
```

**에러 발생 시점** : Parser — `print` 문 파싱 후 `;` 를 기대하지만 EOF 또는 다음 토큰을 만남  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_04_Shell_MissingSemicolon` |
| File | `NT_04_File_MissingSemicolon` |
| Debug | `NT_04_Debug_MissingSemicolon` |

---

### NT_05 — 닫는 괄호 누락 (Parser)

**목적**  
그룹핑 식에서 닫는 `)` 가 없을 때 파서 에러가 발생하는지 확인

**입력 코드**
```
print (1 + 2;
```

**에러 발생 시점** : Parser `parseGrouping` — `)` 를 기대하지만 `;` 를 만남  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_05_Shell_MissingClosingParen` |
| File | `NT_05_File_MissingClosingParen` |
| Debug | `NT_05_Debug_MissingClosingParen` |

---

### NT_06 — 잘못된 할당 대상 (Parser)

**목적**  
식(expression) 결과에 값을 할당하려 할 때 파서 에러가 발생하는지 확인

**입력 코드**
```
var a = 1;
var b = 2;
a + b = 3;
```

**에러 발생 시점** : Parser `parseAssignment` — 좌변이 `VariableExpr` / `ArrayAccessExpr` 가 아닌 경우 에러  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_06_Shell_InvalidAssignmentTarget` |
| File | `NT_06_File_InvalidAssignmentTarget` |
| Debug | `NT_06_Debug_InvalidAssignmentTarget` |

> **Note** File / Debug 모드는 줄 배열로 전달 — `var a = 1;`, `var b = 2;`, `a + b = 3;` 각 줄 분리

---

### NT_07 — 예상치 못한 토큰 (Parser)

**목적**  
식이 와야 할 위치에 이항 연산자가 오는 등 예상치 못한 토큰이 있을 때 파서 에러가 발생하는지 확인

**입력 코드**
```
print * 5;
```

**에러 발생 시점** : Parser `parsePrimary` — 리터럴 / 식별자 / `(` 가 와야 할 위치에 `*` 를 만남  
**기대 포함 문자열** : `[Error]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_07_Shell_UnexpectedToken` |
| File | `NT_07_File_UnexpectedToken` |
| Debug | `NT_07_Debug_UnexpectedToken` |

---

### NT_08 — 지역 변수 자기 초기화 (Checker)

**목적**  
변수 선언의 초기화 식에서 자기 자신을 참조할 때 Checker 정적 에러가 발생하는지 확인

**입력 코드**
```
{ var a = a; }
```

**에러 발생 시점** : Checker — 변수 `a` 가 자신의 초기화식에서 아직 완전히 선언되기 전에 읽힘  
**기대 포함 문자열** : `[Checker]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_08_Shell_SelfInitReference` |
| File | `NT_08_File_SelfInitReference` |
| Debug | `NT_08_Debug_SelfInitReference` |

---

### NT_09 — 같은 스코프 중복 선언 (Checker)

**목적**  
동일 블록 스코프에서 같은 이름의 변수를 두 번 선언할 때 Checker 정적 에러가 발생하는지 확인

**입력 코드**
```
{ var a = "hi"; var a = 3; }
```

**에러 발생 시점** : Checker — 같은 스코프에 이미 `a` 가 존재함을 감지  
**기대 포함 문자열** : `[Checker]`

| 모드 | TC 함수 |
|------|---------|
| Shell | `NT_09_Shell_DuplicateDeclaration` |
| File | `NT_09_File_DuplicateDeclaration` |
| Debug | `NT_09_Debug_DuplicateDeclaration` |

---

## 요약

| 구분 | 케이스 수 | TC 수 (×3 모드) | 상태 |
|------|-----------|----------------|------|
| Parser 에러 | 4 | 12 | 🟢 All Green |
| Checker 에러 (정적) | 2 | 6 | 🟢 All Green |
| Runtime 에러 | 3 | 9 | 🟢 All Green |
| **합계** | **9** | **27** | 🟢 **27/27 통과** |
