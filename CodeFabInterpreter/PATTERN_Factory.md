# Factory Method 패턴 적용 문서

## 개요

| 항목 | 내용 |
|------|------|
| 패턴 | GoF Factory Method |
| 대상 | `FactoryShell` — 실행 모드별 Runner 객체 생성 |
| 관련 파일 | `Shell.h`, `Shell.cpp` |

---

## 패턴 구조

GoF Factory Method 패턴은 객체 생성 로직을 별도 메서드(팩토리)로 캡슐화하여, 클라이언트 코드가 구체 클래스에 직접 의존하지 않도록 한다.

```
FactoryShell
  ├─ detectMode()   ← 모드 판별 (Factory Method)
  └─ run()          ← 모드별 객체 생성 + 실행

생성 대상:
  ShellMode::REPL  → Shell
  ShellMode::FILE  → FileRunner
  ShellMode::DEBUG → DebugShell
```

---

## 코드 구조

### Shell.h — 제품 계층과 팩토리 선언

```cpp
// 공통 베이스 (제품 인터페이스 역할)
class BaseRunner
{
protected:
    Executor executor;
    Checker  checker;
    bool executeSource(const std::string& source);
};

// 구체 제품 3종
class Shell      : public BaseRunner { public: void run(); };
class FileRunner : public BaseRunner { public: void run(const std::string& filepath); };
class DebugShell : public BaseRunner { public: void run(const std::string& filepath); };

// 실행 모드 열거형
enum class ShellMode { REPL, FILE, DEBUG };

// 팩토리
class FactoryShell
{
public:
    ShellMode detectMode(int argc, char** argv) const;  // ← 모드 판별
    void      run(int argc, char** argv);               // ← 생성 + 실행
};
```

### Shell.cpp — 팩토리 구현

```cpp
// 모드 판별: 명령행 인자로 실행 모드를 결정한다.
ShellMode FactoryShell::detectMode(int argc, char** argv) const
{
    if (argc >= 2)
    {
        std::string cmd(argv[1]);
        if (cmd == "run")   return ShellMode::FILE;
        if (cmd == "debug") return ShellMode::DEBUG;
    }
    return ShellMode::REPL;   // 인자 없으면 대화형 모드
}

// 객체 생성 + 실행: 모드에 따라 적합한 구체 클래스를 생성한다.
void FactoryShell::run(int argc, char** argv)
{
    switch (detectMode(argc, argv))
    {
        case ShellMode::REPL:
        {
            Shell shell;
            shell.run();
            break;
        }
        case ShellMode::FILE:
        {
            FileRunner runner;
            runner.run(argv[2]);
            break;
        }
        case ShellMode::DEBUG:
        {
            DebugShell debugShell;
            debugShell.run(argv[2]);
            break;
        }
    }
}
```

### main.cpp — 클라이언트 (구체 클래스 비의존)

```cpp
int main(int argc, char** argv)
{
    FactoryShell factory;
    factory.run(argc, argv);   // ← Shell / FileRunner / DebugShell 중 무엇이 생성되는지 몰라도 됨
    return 0;
}
```

---

## 객체 생성 흐름

```
main()
  └─ FactoryShell::run(argc, argv)
       ├─ detectMode(argc, argv)
       │    ├─ argv[1] == "run"   → ShellMode::FILE
       │    ├─ argv[1] == "debug" → ShellMode::DEBUG
       │    └─ (없음)             → ShellMode::REPL
       │
       └─ switch(mode)
            ├─ REPL  → Shell shell;          shell.run();
            ├─ FILE  → FileRunner runner;    runner.run(argv[2]);
            └─ DEBUG → DebugShell debugShell; debugShell.run(argv[2]);
```

---

## 장점

### 1. 클라이언트와 구체 클래스 분리
`main()` 은 `FactoryShell::run()` 만 호출하면 된다. `Shell`, `FileRunner`, `DebugShell` 이라는 구체 타입을 직접 알 필요가 없다. 새 실행 모드를 추가할 때 `main()` 은 변경하지 않아도 된다.

### 2. 모드 판별 로직 캡슐화
`detectMode()` 가 명령행 인자 해석을 전담한다. 인자 규칙이 바뀌어도 이 메서드만 수정하면 된다.

### 3. 확장에 열려 있음
새 실행 모드(예: `ProfileRunner`)를 추가하려면:
1. `BaseRunner` 를 상속하는 새 클래스 작성
2. `ShellMode` 에 값 추가
3. `detectMode()` / `run()` 에 분기 추가

기존 `Shell`, `FileRunner`, `DebugShell` 코드는 변경하지 않아도 된다.
