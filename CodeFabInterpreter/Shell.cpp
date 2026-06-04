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
        std::cout << ">>> ";
        if (!std::getline(std::cin, line)) break;
        runLine(line);
    }
}

void Shell::runLine(const std::string& source)
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
                std::cerr << "[Checker] " << err << "\n";
            return;
        }

        executor_.execute(stmts);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Error] " << e.what() << "\n";
    }
}
