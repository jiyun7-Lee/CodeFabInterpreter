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
        evaluateExpr(s->expression.get(), env);
        return;
    }
}

Value Executor::evaluateExpr(Expr* expr, Environment* env)
{
    if (auto* e = dynamic_cast<LiteralExpr*>(expr))
        return e->value;

    return std::monostate{};
}
