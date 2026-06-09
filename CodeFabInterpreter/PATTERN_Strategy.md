# Strategy 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Strategy (합성을 통한 알고리즘 교체) |
| 대상 | `Parser` 의 표현식 파싱 전략 (B파트) |
| 적용 브랜치 | `refactor/expr-parser` |
| 변경 파일 | `Parser.h`, `Parser.cpp` |

---

## 적용 전 구조

표현식 파싱 전략이 `virtual parseExpression()` 상속을 통해서만 교체 가능했다.  
테스트 코드(ParserTest.cpp)의 `FakeExprParser` 는 `Parser` 를 **상속**해 오버라이드했다.

```cpp
// Parser.h (변경 전) — 전략 교체 수단: 상속(Template Method)만 가능
class Parser
{
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens);

protected:
    virtual std::unique_ptr<Expr> parseExpression();  // 유일한 확장 지점

    std::vector<Token> m_tokens;
    int m_current = 0;
    bool  isAtEnd()  const;  // private
    Token peek()     const;  // private
    // ...
private:
    std::unique_ptr<Expr> parseAssignment();
    // ...
};
```

```cpp
// ParserTest.cpp (테스트) — 상속 방식 (리팩토링 전)
class FakeExprParser : public Parser {
protected:
    std::unique_ptr<Expr> parseExpression() override {
        Token t = advance();   // Parser 의 protected 커서 메서드 직접 사용
        auto lit = std::make_unique<LiteralExpr>();
        lit->value = t.literal;
        return lit;
    }
};
```

이 구조의 문제: 표현식 파서를 교체하려면 **반드시 Parser 를 상속**해야 하고,
Parser 내부의 토큰 커서 상태(`m_tokens`, `m_current`)에 직접 의존하게 된다.

> **현재 상태**: 리팩토링 완료 — `FakeExprParser` 는 `IExprParser` 를 구현하는 방식으로 전환됨 (하단 참고)

---

## 적용 후 구조

### Parser.h — IExprParser 인터페이스 추가 + 커서 메서드 public 이동

```cpp
// Parser.h (변경 후)

// 1. 표현식 파싱 전략 인터페이스
class IExprParser
{
public:
    virtual ~IExprParser() = default;
    // Parser& ctx 를 받아 토큰 커서를 직접 소비한다.
    virtual std::unique_ptr<Expr> parseExpression(Parser& ctx) = 0;
};

class Parser
{
public:
    Parser() = default;
    explicit Parser(IExprParser* exprStrategy);  // ← 전략 주입 생성자 추가

    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens);

    // 커서 메서드: IExprParser 전략 구현체가 호출할 수 있도록 public 으로 이동
    bool  isAtEnd()  const;
    Token peek()     const;
    Token previous() const;
    Token advance();
    bool  check(TokenType type) const;
    bool  match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& msg = "");

protected:
    // 하위 호환성 유지: FakeExprParser 상속 방식도 여전히 동작
    virtual std::unique_ptr<Expr> parseExpression();

    std::vector<Token> m_tokens;
    int m_current = 0;

private:
    IExprParser* exprStrategy_ = nullptr;  // ← 주입된 전략 (null이면 기본 구현)
    // ...
};
```

### Parser.cpp — 생성자 + 전략 위임 분기 추가

```cpp
// Parser.cpp (변경 후)

Parser::Parser(IExprParser* exprStrategy)
    : exprStrategy_(exprStrategy)
{
}

std::unique_ptr<Expr> Parser::parseExpression()
{
    if (exprStrategy_) return exprStrategy_->parseExpression(*this);  // ← 전략 위임
    return parseAssignment();  // 기본 재귀 하강 구현
}
```

---

## 두 가지 사용 방식 비교

### 방식 A — 합성 방식 (Strategy 패턴, FakeExprParser 포함)

```
ParserTest.cpp 의 FakeExprParser
───────────────────────────────
FakeExprParser : public IExprParser
  parseExpression(Parser& ctx) → ctx 의 커서 API 를 통해 토큰 소비 후 LiteralExpr 반환

FakeExprParser fake;
Parser p{&fake};   // 전략 주입 — Parser 를 상속하지 않음
```

### 방식 B — 신규 합성 방식 (Strategy 패턴, 실험적 구현)

```
새 코드 (예: Pratt 파서 실험)
───────────────────────────────────
class PrattExprParser : public IExprParser {
    std::unique_ptr<Expr> parseExpression(Parser& ctx) override {
        // ctx.advance(), ctx.peek() 등 커서 API 사용
        // ...
    }
};

PrattExprParser pratt;
Parser parser(&pratt);  // 전략 주입
parser.parse(tokens);   // 표현식 파싱 시 PrattExprParser 가 호출됨
```

`Parser` 를 상속하지 않고도 표현식 파싱 알고리즘을 교체할 수 있다.

---

## 커서 메서드 public 이동 이유

기존에는 `isAtEnd()`, `peek()`, `advance()` 등이 **protected** 였다.  
방식 A (상속)에서는 subclass 가 protected 메서드에 접근 가능했지만,  
방식 B (합성)에서는 `IExprParser` 구현체가 `Parser&` 를 통해 호출해야 하므로  
`public` 으로 이동이 필요했다.

Parser 는 토큰 커서 역할도 겸하므로 커서 API 가 공개되는 것은 설계 상 자연스럽다.

---

## 흐름 비교

### 변경 전 (Template Method)

```
Parser::parse()
  └─ parseDeclaration()
       └─ parseExpression()   ← virtual, subclass 가 override
            └─ FakeExprParser::parseExpression()
```

### 변경 후 (Strategy + Template Method 공존)

```
Parser::parse()
  └─ parseDeclaration()
       └─ parseExpression()   ← virtual
            ├─ [exprStrategy_ != null] exprStrategy_->parseExpression(*this)
            │       └─ IExprParser 구현체가 Parser& 의 커서 API 사용
            └─ [exprStrategy_ == null] parseAssignment()  ← 기본 구현
                    또는 subclass override (FakeExprParser 등)
```

---

## 장점

### 1. 상속 없이 전략 교체 가능
기존에는 표현식 파서를 바꾸려면 `Parser` 를 상속해야 했다.  
`IExprParser` 를 구현하면 `Parser` 와 무관한 독립 클래스로 교체 가능하다.

### 2. 단위 테스트 용이
`IExprParser` 구현체는 `Parser` 없이 독립적으로 테스트할 수 있다.  
반대로 `Parser` 의 Statement 파싱(C파트) 테스트 시 `FakeExprParser` 를  
`IExprParser` 로 만들어 Parser 에 주입하면 됨으로써 더 명확한 경계를 만든다.

### 3. 합성 vs 상속 원칙 준수
"상속보다 합성을 선호하라(Favor composition over inheritance)" 원칙에 따라
표현식 파싱 전략을 **has-a** 관계로 보유한다.  
Parser 가 표현식 파서의 동작 방식에 대해 알 필요가 없다.

### 4. 실험적 구현 격리
Pratt 파서, 메모이제이션 파서 등 실험적 표현식 파싱 알고리즘을
`Parser` 코드를 건드리지 않고 교체·비교할 수 있다.

---

## 변경 범위 요약

| 파일 | 변경 유형 | 핵심 내용 |
|------|-----------|-----------|
| `Parser.h` | 수정 | `IExprParser` 인터페이스 추가, `Parser(IExprParser*)` 생성자, 커서 메서드 `public` 이동·주석 강화, `exprStrategy_` 멤버, `virtual` 제거 |
| `Parser.cpp` | 수정 | 생성자 구현, `parseExpression()` 에 전략 위임 분기 추가 |
| `ParserTest.cpp` | 수정 | `FakeExprParser` 를 `IExprParser` 구현체로 리팩토링, 픽스처를 전략 주입 방식으로 변경 |
| `ExpressionParserTest.cpp` | 무변경 | `Parser parser` 기본 생성자 사용, 동작 동일 |
