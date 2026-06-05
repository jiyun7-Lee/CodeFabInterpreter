#include "Shell.h"
#include <iostream>
#include <string>
#include "Tokenizer.h"
#include "Parser.h"
#include "Checker.h"
#include "Executor.h"

void Shell::run()
{
    std::string line;
    while (true)
    {
        std::cout << ">>> " << std::flush;
        if (!std::getline(std::cin, line)) break;

        // exit / quit 입력 시 루프 종료
        std::string trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
            trimmed.pop_back();
        if (trimmed == "exit" || trimmed == "quit") break;

        runLine(line);
    }
}

void Shell::runLine(const std::string& source)
{
    // Trim trailing whitespace and auto-append ';' for REPL convenience.
    // Blocks ending with '}' and already-terminated lines are left as-is.
    std::string src = source;
    while (!src.empty() && std::isspace(static_cast<unsigned char>(src.back())))
        src.pop_back();

    // exit / quit: REPL 종료 신호 — 에러 없이 조용히 반환
    if (src == "exit" || src == "quit") return;

    if (!src.empty() && src.back() != ';' && src.back() != '}')
        src += ';';

    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(src);

        Parser parser;
        auto stmts = parser.parse(tokens);

        Checker checker;
        if (!checker.check(stmts))
        {
            for (const auto& err : checker.getErrors())
                std::cout << "[Checker] " << err << "\n";
            return;
        }

        executor_.execute(stmts);
    }
    catch (const std::exception& e)
    {
        std::cout << "[Error] " << e.what() << "\n";
    }
}
