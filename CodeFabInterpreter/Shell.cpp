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
// 공통 헬퍼 — brace 누적 (Shell / FileRunner / DebugShell 공유)
// -----------------------------------------------------------------------
// braceDepth <= 0 && pendingControl <= 0 이면 true (실행 가능한 완성 문장)
// pendingControl: 최상위에서 아직 바디를 받지 못한 if/for 개수
//   - if/for 키워드(최상위) 등장 시 ++
//   - 최상위에서 첫 { 등장 시 -- (블록 바디 수신)
//   - 최상위에서 ; 등장 시 -- (인라인 바디 수신)
//   - else 는 카운트 변화 없음 (직전 if 의 pending 을 그대로 승계)
// parenDepth: for(;;) 안의 ; 가 pendingControl 을 줄이지 않도록 보호
static bool accumulateBraces(const std::string& line,
                              std::string& accumulated,
                              int& braceDepth,
                              int& pendingControl,
                              int& parenDepth)
{
    try
    {
        Tokenizer tok;
        for (const auto& t : tok.tokenize(line))
        {
            if (t.type == TokenType::LEFT_PAREN)
                ++parenDepth;
            else if (t.type == TokenType::RIGHT_PAREN)
                --parenDepth;
            else if (t.type == TokenType::LEFT_BRACE)
            {
                // 최상위에서 { 가 열릴 때 pending 중인 제어구조에 블록 바디 할당
                if (braceDepth == 0 && pendingControl > 0)
                    --pendingControl;
                ++braceDepth;
            }
            else if (t.type == TokenType::RIGHT_BRACE)
                --braceDepth;
            else if ((t.type == TokenType::IF || t.type == TokenType::FOR)
                     && braceDepth == 0 && parenDepth == 0)
                ++pendingControl;
            else if (t.type == TokenType::SEMICOLON
                     && braceDepth == 0 && parenDepth == 0 && pendingControl > 0)
                --pendingControl;
        }
    }
    catch (...) {}

    if (!accumulated.empty()) accumulated += '\n';
    accumulated += line;

    if (braceDepth <= 0 && pendingControl <= 0)
    {
        braceDepth = 0;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------
// Shell
// -----------------------------------------------------------------------

void Shell::run()
{
    std::string accumulated;
    int braceDepth     = 0;
    int pendingControl = 0;
    int parenDepth     = 0;

    while (true)
    {
        std::cout << (accumulated.empty() ? "> " : "... ") << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;

        std::string trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
            trimmed.pop_back();

        if (accumulated.empty() && (toLower(trimmed) == "exit" || toLower(trimmed) == "quit")) break;

        if (accumulateBraces(line, accumulated, braceDepth, pendingControl, parenDepth))
        {
            runLine(accumulated);
            accumulated.clear();
            braceDepth     = 0;
            pendingControl = 0;
            parenDepth     = 0;
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
// 공통 헬퍼 — 파일 읽기 (FileRunner / DebugShell 공유)
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
    std::string accumulated;
    int braceDepth     = 0;
    int pendingControl = 0;
    int parenDepth     = 0;

    for (const std::string& srcLine : lines)
    {
        if (!accumulateBraces(srcLine, accumulated, braceDepth, pendingControl, parenDepth)) continue;

        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(accumulated);

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
        accumulated.clear();
        braceDepth     = 0;
        pendingControl = 0;
        parenDepth     = 0;
    }
}

// -----------------------------------------------------------------------
// DebugShell
// -----------------------------------------------------------------------

void DebugShell::run(const std::string& filepath)
{
    auto lines = readFile(filepath);
    if (!lines.has_value()) return;

    std::cout << "[DEBUG] 소스코드 로딩: " << filepath << "\n";
    DebugController ctrl;
    runSource(*lines, ctrl);
}

void DebugShell::runSource(const std::vector<std::string>& lines,
                            DebugController& ctrl)
{
    executor.setDebugController(&ctrl);

    // 정상 종료 / 조기 return / 예외 모든 경로에서 nullptr 복원 보장
    struct CtrlGuard
    {
        Executor& ex;
        ~CtrlGuard() { ex.setDebugController(nullptr); }
    } guard{ executor };

    std::string accumulated;
    int braceDepth     = 0;
    int pendingControl = 0;
    int parenDepth     = 0;
    int startLineNo    = 0;
    std::string startSrcLine;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string& srcLine = lines[i];
        if (srcLine.empty()) continue;

        // 블록 시작 줄 기억 (디버그 출력용)
        if (accumulated.empty())
        {
            startLineNo  = static_cast<int>(i + 1);
            startSrcLine = srcLine;
        }

        if (!accumulateBraces(srcLine, accumulated, braceDepth, pendingControl, parenDepth)) continue;

        ctrl.setLineContext(startLineNo, startSrcLine);

        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(accumulated);

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
        accumulated.clear();
        braceDepth     = 0;
        pendingControl = 0;
        parenDepth     = 0;
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
