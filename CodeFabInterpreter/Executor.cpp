#include "Executor.h"
#include <iostream>

void Executor::execute(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    for (const auto& stmt : statements)
        executeStatement(stmt.get(), &globalEnv);
}

void Executor::executeStatement(Stmt* stmt, Environment* env)
{
    if (auto* s = dynamic_cast<PrintStmt*>(stmt))
    {
        printValue(evaluateExpr(s->expression.get(), env));
        return;
    }
}

void Executor::printValue(const Value& val)
{
    std::visit([](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double> || std::is_same_v<T, std::string>)
            std::cout << v << "\n";
        else if constexpr (std::is_same_v<T, bool>)
            std::cout << (v ? "true" : "false") << "\n";
    }, val);
}

Value Executor::evaluateExpr(Expr* expr, Environment* env)
{
    if (auto* e = dynamic_cast<LiteralExpr*>(expr))
        return e->value;

    return std::monostate{};
}
