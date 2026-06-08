#pragma once
#include <string>
#include "Executor.h"
#include "Checker.h"
#include "DebugController.h"

// -----------------------------------------------------------------------
// Shell — REPL 모드
// -----------------------------------------------------------------------
class Shell
{
public:
    void run();
    void runLine(const std::string& source);
private:
    Executor executor;
    Checker  checker;
};

// -----------------------------------------------------------------------
// FileRunner — 파일 실행 모드
// -----------------------------------------------------------------------
class FileRunner
{
public:
    void run(const std::string& filepath);
    void runSource(const std::string& source);
};

// -----------------------------------------------------------------------
// DebugShell — 디버그 모드
// -----------------------------------------------------------------------
class DebugShell
{
public:
    void run(const std::string& filepath);
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
};
