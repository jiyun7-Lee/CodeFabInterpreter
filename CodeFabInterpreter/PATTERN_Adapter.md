# Adapter 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Adapter (Factory Method 와 조합) |
| 대상 | `FactoryShell` — 모드별 Runner의 서로 다른 시그니처를 `IShellRunner::run()`으로 통일 |
| 적용 브랜치 | `refactor/UnitTestShell` |
| 변경 파일 | `Shell.h`, `Shell.cpp` |

---

## 적용 전 구조

`FactoryShell::run()`이 세 클래스의 서로 다른 `run()` 시그니처를 직접 알고 있어야 했습니다.

```cpp
void FactoryShell::run(int argc, char** argv)
{
    switch (detectMode(argc, argv))
    {
        case ShellMode::REPL:
            Shell shell; shell.run();                    // 인수 없음
            break;
        case ShellMode::FILE:
            FileRunner runner; runner.run(argv[2]);      // filepath 필요
            break;
        case ShellMode::DEBUG:
            DebugShell debug; debug.run(argv[2]);        // filepath 필요
            break;
    }
}
```

**문제점**
- `FactoryShell`이 각 클래스의 시그니처 차이를 직접 처리
- 새 모드 추가 시 `run()` 내부 switch 문을 수정해야 함
- 인터페이스 불일치(`run()` vs `run(filepath)`)로 인해 다형적 처리 불가

---

## 적용 후 구조

### Target 인터페이스

```cpp
class IShellRunner
{
public:
    virtual ~IShellRunner() = default;
    virtual void run() = 0;
};
```

### Adapter 구현체

```cpp
// Shell: run() 시그니처가 이미 일치 → Adapter로 감쌈
class ReplAdapter : public IShellRunner
{
    Shell shell_;
public:
    void run() override { shell_.run(); }
};

// FileRunner: run(filepath) → run() 으로 적용
class FileAdapter : public IShellRunner
{
    FileRunner  runner_;
    std::string filepath_;
public:
    explicit FileAdapter(std::string filepath) : filepath_(std::move(filepath)) {}
    void run() override { runner_.run(filepath_); }
};

// DebugShell: run(filepath) → run() 으로 적응
class DebugAdapter : public IShellRunner
{
    DebugShell  shell_;
    std::string filepath_;
public:
    explicit DebugAdapter(std::string filepath) : filepath_(std::move(filepath)) {}
    void run() override { shell_.run(filepath_); }
};
```

### Factory Method (`createRunner`) + 단순화된 `run()`

```cpp
// createRunner: Factory Method — ShellMode에 따라 적합한 Adapter 생성
std::unique_ptr<IShellRunner> FactoryShell::createRunner(int argc, char** argv) const
{
    switch (detectMode(argc, argv))
    {
        case ShellMode::REPL:  return std::make_unique<ReplAdapter>();
        case ShellMode::FILE:  return std::make_unique<FileAdapter>(argv[2]);
        case ShellMode::DEBUG: return std::make_unique<DebugAdapter>(argv[2]);
    }
    return nullptr;
}

// run: Adapter의 공통 인터페이스만 사용 — argv 세부 구조 불필요
void FactoryShell::run(int argc, char** argv)
{
    auto runner = createRunner(argc, argv);
    if (runner) runner->run();
}
```

---

## 패턴 조합 구조

```
FactoryShell::createRunner()   ← Factory Method
    │ ShellMode::REPL  → ReplAdapter  ┐
    │ ShellMode::FILE  → FileAdapter  ├── Adapter (IShellRunner 구현)
    │ ShellMode::DEBUG → DebugAdapter ┘
    ↓
FactoryShell::run()
    runner->run()              ← IShellRunner (Target 인터페이스)
```

---

## 새 모드 추가 방법

1. `ShellMode` enum에 새 값 추가
2. 새 Runner 클래스 작성 (또는 기존 클래스 재사용)
3. `IShellRunner`를 구현하는 Adapter 클래스 작성
4. `createRunner()`에 `case` 추가

`FactoryShell::run()` 자체는 **수정 불필요**합니다.

---

## 효과

| 항목 | 변경 전 | 변경 후 |
|------|---------|---------|
| 모드 추가 시 수정 범위 | `run()` switch 문 | `createRunner()` case + Adapter 클래스 |
| `FactoryShell::run()` | argv 세부 구조 직접 처리 | `runner->run()` 한 줄 |
| 인터페이스 통일 | 없음 | `IShellRunner::run()` |
| 다형 처리 | 불가 | 가능 (IShellRunner 주입) |
