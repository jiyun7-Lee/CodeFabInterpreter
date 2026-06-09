#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Executor.h"
#include "Checker.h"

class DebugController;  // forward declaration — Shell.h 포함 시 DebugController.h 전파 제거

// -----------------------------------------------------------------------
// BaseRunner — Executor/Checker 공유 베이스, 파이프라인 실행 담당
// -----------------------------------------------------------------------
class BaseRunner
{
protected:
    Executor executor;
    Checker  checker;

    // tokenize → parse → check → execute 파이프라인
    // 성공 시 true, Checker 에러 / 런타임 에러 시 false
    bool executeSource(const std::string& source);
};

// -----------------------------------------------------------------------
// Shell — REPL 모드
// -----------------------------------------------------------------------
class Shell : public BaseRunner
{
public:
    void run();
    void runLine(const std::string& source);
};

// -----------------------------------------------------------------------
// FileRunner — 파일 실행 모드
// -----------------------------------------------------------------------
class FileRunner : public BaseRunner
{
public:
    void run(const std::string& filepath);
    void runSource(const std::vector<std::string>& lines);
};

// -----------------------------------------------------------------------
// DebugShell — 디버그 모드
// -----------------------------------------------------------------------
class DebugShell : public BaseRunner
{
public:
    void run(const std::string& filepath);
    void runSource(const std::vector<std::string>& lines, DebugController& ctrl);
};

// -----------------------------------------------------------------------
// IShellRunner — Adapter 공통 인터페이스
// 새 모드 추가 시 IShellRunner 를 구현하는 Adapter 클래스만 추가하면 됨
// -----------------------------------------------------------------------
class IShellRunner
{
public:
    virtual ~IShellRunner() = default;
    virtual void run() = 0;
};

// -----------------------------------------------------------------------
// Adapter 구현체
// -----------------------------------------------------------------------

class ReplAdapter : public IShellRunner
{
    Shell shell_;
public:
    void run() override { shell_.run(); }
};

class FileAdapter : public IShellRunner
{
    FileRunner  runner_;
    std::string filepath_;
public:
    explicit FileAdapter(std::string filepath) : filepath_(std::move(filepath)) {}
    void run() override { runner_.run(filepath_); }
};

class DebugAdapter : public IShellRunner
{
    DebugShell  shell_;
    std::string filepath_;
public:
    explicit DebugAdapter(std::string filepath) : filepath_(std::move(filepath)) {}
    void run() override { shell_.run(filepath_); }
};

// -----------------------------------------------------------------------
// FactoryShell — 실행 모드 분기
// -----------------------------------------------------------------------
enum class ShellMode { REPL, FILE, DEBUG };

class FactoryShell
{
public:
    ShellMode detectMode(int argc, char** argv) const;
    void      run(int argc, char** argv);

private:
    // 모드에 맞는 Adapter 생성 — 새 모드는 여기에 case 추가
    std::unique_ptr<IShellRunner> createRunner(int argc, char** argv) const;
};
