# Template Method 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Template Method |
| 대상 | `BaseRunner::executeSource()` — 공통 실행 파이프라인 |
| 관련 파일 | `Shell.h`, `Shell.cpp` |

---

## 패턴 구조

GoF Template Method 패턴은 알고리즘의 뼈대(순서)를 기반 클래스에서 고정하고, 세부 단계는 서브클래스가 구현하도록 한다.

```
BaseRunner::executeSource()   ← 파이프라인 순서 고정 (Template Method)
  1. Tokenizer::tokenize()
  2. Parser::parse()
  3. Checker::check()
  4. Executor::execute()

서브클래스가 변경하는 것 — 입력 획득 방식과 실행 루프:
  Shell      → REPL 루프, 한 줄씩 누적
  FileRunner → 파일 읽기, 전체 누적
  DebugShell → 파일 읽기, 줄별 DebugController 연동
```

---

## 코드 구조

### Shell.h — 베이스와 서브클래스 선언

```cpp
// 베이스 클래스 — 파이프라인 고정
class BaseRunner
{
protected:
    Executor executor;
    Checker  checker;

    // Template Method: tokenize → parse → check → execute 순서를 고정한다.
    bool executeSource(const std::string& source);
};

// 서브클래스 — 입력 방식만 다르게 구현
class Shell      : public BaseRunner { public: void run(); };
class FileRunner : public BaseRunner { public: void run(const std::string& filepath); };
class DebugShell : public BaseRunner { public: void run(const std::string& filepath); };
```

### Shell.cpp — Template Method 구현

```cpp
// 베이스 클래스가 파이프라인 순서를 고정한다.
// 서브클래스는 이 메서드를 직접 호출만 하면 된다.
bool BaseRunner::executeSource(const std::string& src)
{
    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(src);   // Step 1: 토크나이징

        Parser parser;
        auto stmts = parser.parse(tokens);       // Step 2: 파싱

        if (!checker.check(stmts))               // Step 3: 정적 검사
        {
            for (const auto& err : checker.getErrors())
                std::cout << "[Checker] " << err << "\n";
            return false;
        }

        executor.execute(stmts);                 // Step 4: 실행
        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "[Error] " << e.what() << "\n";
        return false;
    }
}
```

### Shell::run() — REPL 서브클래스 (입력 방식 구현)

```cpp
void Shell::run()
{
    BraceContext ctx;
    std::string  line;
    std::cout << "> " << std::flush;

    while (std::getline(std::cin, line))
    {
        if (isExitCommand(line)) break;

        // 멀티라인 누적 로직 (brace/paren 추적)
        accumulateBraces(ctx, line, ...);

        if (isComplete(ctx))
        {
            executeSource(ctx.accumulated);  // ← Template Method 호출
            ctx.reset();
        }
        // ...
    }
}
```

### FileRunner::run() — 파일 모드 서브클래스

```cpp
void FileRunner::run(const std::string& filepath)
{
    std::ifstream file(filepath);
    // ...
    BraceContext ctx;
    std::string  line;

    while (std::getline(file, line))
    {
        accumulateBraces(ctx, line, ...);
        if (isComplete(ctx))
        {
            executeSource(ctx.accumulated);  // ← 동일한 Template Method 호출
            ctx.reset();
        }
    }
}
```

---

## 파이프라인 흐름

```
[Shell / FileRunner / DebugShell]
  소스 문자열 준비 (방식이 각각 다름)
        │
        ▼
BaseRunner::executeSource(source)
  ┌─────────────────────────────┐
  │ 1. Tokenizer::tokenize()    │  소스 → Token[]
  │ 2. Parser::parse()          │  Token[] → AST
  │ 3. Checker::check()         │  AST 정적 검사
  │ 4. Executor::execute()      │  AST 실행
  └─────────────────────────────┘
        │
        ▼
   결과 출력 / 에러 메시지
```

---

## 공유 상태 — `executor` / `checker` 멤버

`BaseRunner` 는 `Executor` 와 `Checker` 를 멤버로 보유한다. 이를 통해 REPL 모드에서 세션 전체에 걸쳐 변수·함수 정의가 유지된다.

```cpp
// var x = 1; 을 첫 번째 executeSource()에서 실행하면
// 두 번째 executeSource()에서도 executor 가 x 를 기억한다.
```

---

## 장점

### 1. 코드 중복 제거
Tokenize → Parse → Check → Execute 순서는 세 모드 모두 동일하다. `executeSource()` 한 곳에만 있으므로, 파이프라인에 새 단계(예: 최적화 패스)를 추가해도 서브클래스를 수정할 필요가 없다.

### 2. 관심사 분리
- 서브클래스 — **어떻게 입력을 얻는가** (REPL 루프, 파일 읽기, 디버그 루프)
- 베이스 클래스 — **어떻게 실행하는가** (고정 파이프라인)

### 3. 새 실행 모드 추가 용이
새 모드(예: `NetworkRunner`)를 만들 때 `BaseRunner` 를 상속하고, 입력 획득 방식(`run()`)만 구현하면 된다. 파이프라인 로직은 자동으로 상속된다.
