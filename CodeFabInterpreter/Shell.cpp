#include "Shell.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include "Tokenizer.h"
#include "Parser.h"
#include "Checker.h"
#include "Executor.h"

static std::string toLower(std::string s)
{
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// -----------------------------------------------------------------------
// Shell
// -----------------------------------------------------------------------

void Shell::run()
{
    std::string line;
    while (true)
    {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;

        std::string trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
            trimmed.pop_back();
        if (toLower(trimmed) == "exit" || toLower(trimmed) == "quit") break;

        runLine(line);
    }
}

void Shell::runLine(const std::string& source)
{
    std::string src = source;
    while (!src.empty() && std::isspace(static_cast<unsigned char>(src.back())))
        src.pop_back();

    if (toLower(src) == "exit" || toLower(src) == "quit") return;

    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(src);

        Parser parser;
        auto stmts = parser.parse(tokens);

        if (!checker.check(stmts))
        {
            for (const auto& err : checker.getErrors())
                std::cout << "[Checker] " << err << "\n";
            return;
        }

        executor.execute(stmts);
    }
    catch (const std::exception& e)
    {
        std::cout << "[Error] " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------
// 공통 헬퍼 — 파일 읽기 (FileRunner / DebugShell 공유)
// -----------------------------------------------------------------------
static std::string readFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cout << "Error: File Not Found '" << filepath << "'\n";
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}

// -----------------------------------------------------------------------
// FileRunner
// -----------------------------------------------------------------------

void FileRunner::run(const std::string& filepath)
{
    std::string source = readFile(filepath);
    if (source.empty()) return;
    runSource(source);
}

void FileRunner::runSource(const std::string& source)
{
    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(source);

        Parser parser;
        auto stmts = parser.parse(tokens);

        Checker checker;
        if (!checker.check(stmts))
        {
            for (const auto& err : checker.getErrors())
                std::cout << "[Checker] " << err << "\n";
            return;
        }

        Executor executor;
        executor.execute(stmts);
    }
    catch (const std::exception& e)
    {
        std::cout << "[Error] " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------
// DebugShell
// -----------------------------------------------------------------------

void DebugShell::run(const std::string& filepath)
{
    std::string source = readFile(filepath);
    if (source.empty()) return;

    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(source);

        Parser parser;
        auto stmts = parser.parse(tokens);

        Checker checker;
        if (!checker.check(stmts))
        {
            for (const auto& err : checker.getErrors())
                std::cout << "[Checker] " << err << "\n";
            return;
        }

        DebugController controller;
        Executor executor;
        executor.setDebugController(&controller);
        executor.execute(stmts);
    }
    catch (const std::exception& e)
    {
        std::cout << "[Error] " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------
// FactoryShell
// -----------------------------------------------------------------------

ShellMode FactoryShell::detectMode(int argc, char** argv) const
{
    if (argc >= 2)
    {
        std::string cmd(argv[1]);
        if (cmd == "run")   return ShellMode::FILE;
        if (cmd == "debug") return ShellMode::DEBUG;
    }
    return ShellMode::REPL;
}

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
            if (argc < 3)
            {
                std::cout << "Error: 파일 경로가 필요합니다. 사용법: run <파일경로>\n";
                break;
            }
            FileRunner runner;
            runner.run(argv[2]);
            break;
        }
        case ShellMode::DEBUG:
        {
            if (argc < 3)
            {
                std::cout << "Error: 파일 경로가 필요합니다. 사용법: debug <파일경로>\n";
                break;
            }
            DebugShell debugShell;
            debugShell.run(argv[2]);
            break;
        }
    }
}
