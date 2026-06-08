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
#include "DebugController.h"

static std::string toLower(std::string s)
{
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool isExitCommand(const std::string& s)
{
    return toLower(s) == "exit" || toLower(s) == "quit";
}

// -----------------------------------------------------------------------
// 공통 헬퍼 — brace 누적 (Shell / FileRunner / DebugShell 공유)
// -----------------------------------------------------------------------
// AccumState: accumulateBraces 호출 사이에 유지되는 누적 상태 전체
//   braceDepth     — 열린 { 깊이
//   pendingControl — 최상위에서 바디를 아직 받지 못한 if/for/else 개수
//   parenDepth     — for(;;) 안의 ; 를 보호하기 위한 ( 깊이
//   pendingElse    — if 블록 바디가 닫힌 뒤 else 를 기다리는 플래그 (0/1)
//   nextElseBody   — 다음에 열리는 최상위 { 가 else 바디임을 표시
//   blockIsIf      — 현재 열린 최상위 블록이 if 바디 (닫힐 때 else 대기 여부)
//   blockIsElse    — 현재 열린 최상위 블록이 else 바디 (닫혀도 else 대기 없음)
struct AccumState
{
    int  braceDepth         = 0;
    int  pendingControl     = 0;
    int  parenDepth         = 0;
    int  pendingElse        = 0;
    bool nextElseBody       = false;
    bool blockIsIf          = false;
    bool blockIsElse        = false;
    bool lastControlWasIf   = false;
};

// BraceContext: accumulated 문자열 + AccumState 를 하나로 묶어 선언·리셋 중복 제거
struct BraceContext
{
    std::string accumulated;
    AccumState  state;

    void reset()
    {
        accumulated.clear();
        state = AccumState{};
    }
};

static bool accumulateBraces(const std::string& line, BraceContext& ctx)
{
    AccumState& s = ctx.state;

    try
    {
        // 빈 줄(공백만 있는 줄)은 else 없이 구문을 끝냈다는 신호
        // → pendingElse 를 해제해 REPL 에서 무한 대기에 빠지지 않도록 함
        if (s.pendingElse > 0 && line.find_first_not_of(" \t\r\n") == std::string::npos)
            s.pendingElse = 0;

        int thisLineControls = 0;
        Tokenizer tok;
        for (const auto& t : tok.tokenize(line))
        {
            // else 대기 중에 else·EOF 가 아닌 토큰이 최상위에 오면 대기 포기
            if (s.pendingElse > 0 && t.type != TokenType::ELSE
                && t.type != TokenType::EOF_TOKEN && s.braceDepth == 0)
                s.pendingElse = 0;

            if (t.type == TokenType::LEFT_PAREN)
                ++s.parenDepth;
            else if (t.type == TokenType::RIGHT_PAREN)
                --s.parenDepth;
            else if (t.type == TokenType::LEFT_BRACE)
            {
                if (s.braceDepth == 0)
                {
                    if (s.pendingControl > 0)
                    {
                        s.blockIsElse      = s.nextElseBody;
                        s.blockIsIf        = !s.nextElseBody && s.lastControlWasIf;
                        s.nextElseBody     = false;
                        s.lastControlWasIf = false;
                        --s.pendingControl;
                    }
                    else
                    {
                        s.blockIsElse      = false;
                        s.blockIsIf        = false;
                        s.lastControlWasIf = false;
                    }
                }
                ++s.braceDepth;
            }
            else if (t.type == TokenType::RIGHT_BRACE)
            {
                --s.braceDepth;
                if (s.braceDepth == 0)
                {
                    if (s.blockIsElse)
                    {
                        s.pendingElse    = 0;
                        s.pendingControl = 0;
                    }
                    else if (s.blockIsIf)
                        s.pendingElse = 1;
                    else
                        s.pendingElse = 0;
                    s.blockIsElse = false;
                    s.blockIsIf   = false;
                }
            }
            else if (s.braceDepth == 0 && s.parenDepth == 0
                     && (t.type == TokenType::IF || t.type == TokenType::FOR))
            {
                ++s.pendingControl;
                ++thisLineControls;
                s.lastControlWasIf = (t.type == TokenType::IF);
            }
            else if (t.type == TokenType::ELSE
                     && s.braceDepth == 0 && s.parenDepth == 0)
            {
                if (s.pendingElse > 0)
                {
                    s.pendingElse = 0;
                    ++s.pendingControl;
                    s.nextElseBody = true;
                }
                else if (s.pendingControl == 0)
                {
                    ++s.pendingControl;
                    s.nextElseBody = true;
                }
            }
            else if (t.type == TokenType::SEMICOLON
                     && s.braceDepth == 0 && s.parenDepth == 0
                     && s.pendingControl > 0)
            {
                int toClose = std::max(1, thisLineControls);
                s.pendingControl = std::max(0, s.pendingControl - toClose);
                s.pendingElse    = 0;
                thisLineControls = 0;
            }
        }
    }
    catch (...) {}

    if (!ctx.accumulated.empty()) ctx.accumulated += '\n';
    ctx.accumulated += line;

    if (s.braceDepth <= 0 && s.pendingControl <= 0 && s.pendingElse <= 0)
    {
        s.braceDepth = 0;
        return true;
    }
    return false;
}

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

// pendingElse > 0 이면서 입력이 끝난 경우 (else 없이 if 블록만 존재) 실행을 flush
static bool needsPendingElseFlush(const BraceContext& ctx)
{
    return !ctx.accumulated.empty()
        && ctx.state.braceDepth     == 0
        && ctx.state.pendingControl == 0
        && ctx.state.pendingElse    > 0;
}

// -----------------------------------------------------------------------
// BaseRunner — 파이프라인 실행 (tokenize → parse → check → execute)
// -----------------------------------------------------------------------

bool BaseRunner::executeSource(const std::string& src)
{
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
            return false;
        }

        executor.execute(stmts);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "[Error] " << e.what() << "\n";
        return false;
    }
}

// -----------------------------------------------------------------------
// Shell
// -----------------------------------------------------------------------

void Shell::run()
{
    BraceContext ctx;

    while (true)
    {
        std::cout << (ctx.accumulated.empty() ? "> " : "... ") << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;

        std::string trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
            trimmed.pop_back();

        if (ctx.accumulated.empty() && isExitCommand(trimmed)) break;

        if (accumulateBraces(line, ctx))
        {
            runLine(ctx.accumulated);
            ctx.reset();
        }
    }

    // EOF 직전에 else 없는 if-블록이 pending 으로 남은 경우 flush
    if (needsPendingElseFlush(ctx))
        runLine(ctx.accumulated);
}

void Shell::runLine(const std::string& source)
{
    std::string src = source;
    while (!src.empty() && std::isspace(static_cast<unsigned char>(src.back())))
        src.pop_back();

    if (isExitCommand(src)) return;

    executeSource(src);
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
    BraceContext ctx;

    for (const std::string& srcLine : lines)
    {
        if (!accumulateBraces(srcLine, ctx)) continue;
        if (!executeSource(ctx.accumulated)) return;
        ctx.reset();
    }

    // else 없이 if-블록이 파일 끝에서 pending 으로 남은 경우 flush
    if (needsPendingElseFlush(ctx))
        executeSource(ctx.accumulated);
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

    BraceContext ctx;
    int startLineNo = 0;
    std::string startSrcLine;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string& srcLine = lines[i];
        if (srcLine.empty()) continue;

        // 블록 시작 줄 기억 (디버그 출력용)
        if (ctx.accumulated.empty())
        {
            startLineNo  = static_cast<int>(i + 1);
            startSrcLine = srcLine;
        }

        if (!accumulateBraces(srcLine, ctx)) continue;

        ctrl.setLineContext(startLineNo, startSrcLine);
        if (!executeSource(ctx.accumulated)) return;
        ctx.reset();
    }

    // else 없이 if-블록이 파일 끝에서 pending 으로 남은 경우 flush
    if (needsPendingElseFlush(ctx))
    {
        ctrl.setLineContext(startLineNo, startSrcLine);
        executeSource(ctx.accumulated);
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
