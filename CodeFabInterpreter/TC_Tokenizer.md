# TC_Tokenizer — Tokenizer 테스트 케이스 명세

담당자: 송용길 (Yonggil Song)  
파일: `tests/TokenizerTest.cpp`  
프레임워크: Google Test (gtest) / Google Mock (gmock)

---

## 테스트 전략

- Tokenizer의 public 인터페이스 `tokenize(source)` 를 통해 간접 검증
- 입력: `std::string` 소스 코드
- 출력: `std::vector<Token>` — 각 Token의 `type`, `lexeme`, `literal`, `line` 검증
- 각 TC는 **Arrange → Act → Assert** 패턴으로 구성
- 현재 상태: **Green** (TC-01~13 전체 통과)

---

## TC 목록

| ID | 테스트 이름 | 입력 소스 | 검증 항목 | 상태 |
|---|---|---|---|---|
| TC-01 | TokenizesAllTokenTypesInOrder | 전체 토큰 나열 문자열 | 모든 type 순서 | 🟢 Green |
| TC-02 | EmptySourceReturnsOnlyEof | `""` | size == 1, EOF_TOKEN | 🟢 Green |
| TC-03 | LastTokenIsAlwaysEof | `"x"` | tokens.back() == EOF | 🟢 Green |
| TC-04 | WhitespaceIsSkipped | `"  +  \t  -  "` | PLUS, MINUS, EOF 만 존재 | 🟢 Green |
| TC-05 | SingleCharTokensHaveCorrectLexeme | `"( ) + - * /"` | lexeme 문자열 일치 | 🟢 Green |
| TC-06 | IdentifierHasCorrectLexeme | `"myVar"` | lexeme == "myVar" | 🟢 Green |
| TC-07 | StringLiteralHasCorrectValue | `"\"hello\""` | literal == "hello" (string) | 🟢 Green |
| TC-08 | IntegerLiteralHasCorrectValue | `"123"` | literal == 123.0 (double) | 🟢 Green |
| TC-09 | FloatLiteralHasCorrectValue | `"3.14"` | literal == 3.14 (double) | 🟢 Green |
| TC-10 | KeywordsAreNotIdentifiers | 키워드 9개 나열 | 각 keyword → 고유 TokenType | 🟢 Green |
| TC-11 | IdentifierStartingWithKeywordIsIdentifier | `"variable iffy forge"` | 모두 IDENTIFIER | 🟢 Green |
| TC-12 | NewlineIncrementsLineNumber | `"+\n-\n*"` | line 1, 2, 3 순서 | 🟢 Green |
| TC-13 | BangTokenHasCorrectLexeme | `"!"` | BANG, lexeme == "!" | 🟢 Green |

---

## TC 상세

### TC-01 TokenizesAllTokenTypesInOrder

**목적**: 지원하는 모든 TokenType이 올바른 순서로 인식되는지 통합 검증 (literal 검증은 TC-07~09 참조)

**입력 소스**
```
( ) { } , . ; + - * / ! = > < identifier "hello" 123 var print if else for true false and or
```

**기대 토큰 시퀀스**
```
LEFT_PAREN → RIGHT_PAREN → LEFT_BRACE → RIGHT_BRACE → COMMA → DOT → SEMICOLON
→ PLUS → MINUS → STAR → SLASH → BANG → EQUAL → GREATER → LESS
→ IDENTIFIER → STRING → NUMBER
→ VAR → PRINT → IF → ELSE → FOR → TRUE → FALSE → AND → OR
→ EOF_TOKEN
```

| 단계 | 내용 |
|---|---|
| Arrange | 모든 TokenType을 한 줄에 포함하는 소스 문자열 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `actualTypes` 벡터가 `expectedTypes` 벡터와 완전히 일치 |

---

### TC-02 EmptySourceReturnsOnlyEof

**목적**: 빈 문자열 입력 시 EOF_TOKEN 하나만 반환하는지 확인

**입력 소스**
```
""
```

**기대 결과**
```
[EOF_TOKEN]  (size == 1)
```

| 단계 | 내용 |
|---|---|
| Arrange | 빈 문자열 준비 |
| Act | `tokenizer.tokenize("")` 호출 |
| Assert | `tokens.size()` == 1, `tokens[0].type` == EOF_TOKEN |

---

### TC-03 LastTokenIsAlwaysEof

**목적**: 소스가 무엇이든 결과 벡터의 마지막 토큰은 항상 EOF_TOKEN임을 확인

**입력 소스**
```
x
```

**기대 결과**
```
IDENTIFIER → EOF_TOKEN
             ↑ back()
```

| 단계 | 내용 |
|---|---|
| Arrange | 공백 없는 단순 소스 구성 (TC-04 공백 처리에 의존하지 않음) |
| Act | `tokenizer.tokenize("x")` 호출 |
| Assert | `tokens.empty()` == false, `tokens.back().type` == EOF_TOKEN |

---

### TC-04 WhitespaceIsSkipped

**목적**: 스페이스와 탭이 토큰을 생성하지 않는지 확인

**입력 소스**
```
"  +  \t  -  "
```

**기대 결과**
```
PLUS → MINUS → EOF_TOKEN  (총 3개)
```

| 단계 | 내용 |
|---|---|
| Arrange | 공백/탭으로 둘러싸인 연산자 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `tokens.size()` == 3, 순서대로 PLUS / MINUS / EOF_TOKEN |

---

### TC-05 SingleCharTokensHaveCorrectLexeme

**목적**: 단일 문자 토큰의 lexeme이 소스의 해당 문자와 정확히 일치하는지 확인

**입력 소스**
```
( ) + - * /
```

**기대 lexeme**
```
tokens[0].lexeme == "("
tokens[1].lexeme == ")"
tokens[2].lexeme == "+"
tokens[3].lexeme == "-"
tokens[4].lexeme == "*"
tokens[5].lexeme == "/"
```

| 단계 | 내용 |
|---|---|
| Arrange | 단일 문자 6개 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | 각 토큰의 `lexeme` 이 대응하는 문자와 일치 |

---

### TC-06 IdentifierHasCorrectLexeme

**목적**: 식별자 토큰의 lexeme이 소스의 단어와 일치하는지 확인

**입력 소스**
```
myVar
```

**기대 결과**
```
IDENTIFIER, lexeme == "myVar"
```

| 단계 | 내용 |
|---|---|
| Arrange | 알파벳+숫자 혼합 식별자 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `tokens[0].type` == IDENTIFIER, `tokens[0].lexeme` == "myVar" |

---

### TC-07 StringLiteralHasCorrectValue

**목적**: 문자열 리터럴의 `literal` 값이 따옴표를 제외한 내용으로 저장되는지 확인

**입력 소스**
```
"hello"
```

**기대 결과**
```
STRING, literal == "hello"  (std::string, 따옴표 미포함)
```

| 단계 | 내용 |
|---|---|
| Arrange | 큰따옴표로 감싼 문자열 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `tokens[0].type` == STRING, `std::get<std::string>(tokens[0].literal)` == "hello" |

---

### TC-08 IntegerLiteralHasCorrectValue

**목적**: 정수 숫자 리터럴의 `literal` 값이 `double`로 변환되어 저장되는지 확인

**입력 소스**
```
123
```

**기대 결과**
```
NUMBER, literal == 123.0  (double)
```

| 단계 | 내용 |
|---|---|
| Arrange | 정수 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `tokens[0].type` == NUMBER, `std::get<double>(tokens[0].literal)` == 123.0 |

---

### TC-09 FloatLiteralHasCorrectValue

**목적**: 소수점 숫자 리터럴의 `literal` 값이 `double`로 정확히 저장되는지 확인

**입력 소스**
```
3.14
```

**기대 결과**
```
NUMBER, literal == 3.14  (double)
```

| 단계 | 내용 |
|---|---|
| Arrange | 소수점 포함 숫자 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `tokens[0].type` == NUMBER, `std::get<double>(tokens[0].literal)` ≈ 3.14 (`EXPECT_DOUBLE_EQ`) |

---

### TC-10 KeywordsAreNotIdentifiers

**목적**: 9개의 예약어가 IDENTIFIER가 아닌 각자의 고유 TokenType으로 인식되는지 확인

**입력 소스**
```
var print if else for true false and or
```

**기대 토큰 시퀀스**
```
VAR → PRINT → IF → ELSE → FOR → TRUE → FALSE → AND → OR → EOF_TOKEN
```

| 단계 | 내용 |
|---|---|
| Arrange | 모든 예약어를 나열한 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `actualTypes` 가 예약어 순서 벡터와 완전히 일치 |

---

### TC-11 IdentifierStartingWithKeywordIsIdentifier

**목적**: 예약어로 시작하지만 더 긴 단어는 IDENTIFIER로 인식되는지 확인

**입력 소스**
```
variable iffy forge
```
- `variable` ⊃ `var`
- `iffy` ⊃ `if`
- `forge` ⊃ `for`

**기대 결과**
```
IDENTIFIER("variable") → IDENTIFIER("iffy") → IDENTIFIER("forge") → EOF_TOKEN
```

| 단계 | 내용 |
|---|---|
| Arrange | 예약어 접두사를 포함하는 식별자 3개 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | 세 토큰 모두 IDENTIFIER, lexeme 이 소스 단어와 일치 |

---

### TC-12 NewlineIncrementsLineNumber

**목적**: 개행 문자를 만날 때마다 이후 토큰의 `line` 번호가 1씩 증가하는지 확인

**입력 소스**
```
+\n-\n*
```

**기대 결과**
```
PLUS  (line == 1)
MINUS (line == 2)
STAR  (line == 3)
```

| 단계 | 내용 |
|---|---|
| Arrange | `\n` 으로 구분된 3줄 소스 구성 |
| Act | `tokenizer.tokenize(source)` 호출 |
| Assert | `tokens[0].line` == 1, `tokens[1].line` == 2, `tokens[2].line` == 3 |

---

### TC-13 BangTokenHasCorrectLexeme

**목적**: `!` 문자가 BANG 타입으로 인식되고 lexeme이 `"!"`인지 확인

**입력 소스**
```
!
```

**기대 결과**
```
BANG, lexeme == "!"
```

| 단계 | 내용 |
|---|---|
| Arrange | `!` 단일 문자 소스 구성 |
| Act | `tokenizer.tokenize("!")` 호출 |
| Assert | `tokens[0].type` == BANG, `tokens[0].lexeme` == "!" |

---

## 추가 예정 TC

| ID | 설명 | 입력 예시 | 비고 |
|---|---|---|---|
| TC-14 | 문자열 lexeme이 따옴표를 포함한다 | `"hello"` | lexeme == `"\"hello\""` |
| TC-15 | 숫자 lexeme이 소스 문자열과 일치한다 | `3.14` | lexeme == `"3.14"` |
| TC-16 | 밑줄(_)로 시작하는 식별자 인식 | `_count` | IDENTIFIER, lexeme == "_count" |
| TC-17 | 여러 줄에 걸친 소스의 복합 검증 | `var\nx\n=\n1` | type + line 복합 검증 |
