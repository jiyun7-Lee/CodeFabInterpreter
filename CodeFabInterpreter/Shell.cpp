#include "Shell.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
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
    std::string accumulated;
    int braceDepth = 0;

    while (true)
    {
        std::cout << (accumulated.empty() ? "> " : "... ") << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;

        std::string trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
            trimmed.pop_back();

        if (accumulated.empty() && (toLower(trimmed) == "exit" || toLower(trimmed) == "quit")) break;

        for (char c : line)
        {
            if (c == '{') ++braceDepth;
            else if (c == '}') --braceDepth;
        }

        if (!accumulated.empty()) accumulated += '\n';
        accumulated += line;

        if (braceDepth <= 0)
        {
            braceDepth = 0;
            runLine(accumulated);
            accumulated.clear();
        }
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
// 공통 헬퍼 — 파일 읽기 / 줄 결합 (FileRunner / DebugShell 공유)
// -----------------------------------------------------------------------
// nullopt → 파일 없음, {} → 빈 파일 (정상), {...} → 내용 있는 파일
static std::optional<std::vector<std::string>> readFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cout << "Error: File Not Found '" << filepath << "'\n";
        return std::nullopt;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(std::move(line));
    }
    return lines;
}

// -----------------------------------------------------------------------
// FileRunner
// -----------------------------------------------------------------------

void FileRunner::run(const std::string& filepath)
{
    auto lines = readFile(filepath);
    if (!lines.has_value()) return;
    runSource(*lines);
}

void FileRunner::runSource(const std::vector<std::string>& lines)
{
    for (const std::string& line : lines)
    {
        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(line);

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
            return;
        }
    }
}

// -----------------------------------------------------------------------
// DebugShell
// -----------------------------------------------------------------------

void DebugShell::run(const std::string& filepath)
{
    auto lines = readFile(filepath);
    if (!lines.has_value()) return;

    std::string source;
    for (size_t i = 0; i < lines->size(); ++i)
    {
        if (i > 0) source += '\n';
        source += (*lines)[i];
    }

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
