#include "Executor.h"
#include <iostream>
#include <stdexcept>

static bool isTruthy(const Value& val)
{
    return std::visit([](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>)        return v;
        if constexpr (std::is_same_v<T, double>)      return v != 0.0;
        if constexpr (std::is_same_v<T, std::string>) return !v.empty();
        return false;
    }, val);
}

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

    if (auto* s = dynamic_cast<VarDeclareStmt*>(stmt))
    {
        Value val = s->initializer ? evaluateExpr(s->initializer.get(), env) : std::monostate{};
        env->define(s->name.lexeme, val);
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(stmt))
    {
        if (isTruthy(evaluateExpr(s->condition.get(), env)))
            executeStatement(s->thenBranch.get(), env);
        else if (s->elseBranch)
            executeStatement(s->elseBranch.get(), env);
        return;
    }

    if (auto* s = dynamic_cast<ForStmt*>(stmt))
    {
        if (s->init)
            executeStatement(s->init.get(), env);
        while (!s->condition || isTruthy(evaluateExpr(s->condition.get(), env)))
        {
            executeStatement(s->body.get(), env);
            if (s->increment)
                evaluateExpr(s->increment.get(), env);
        }
        return;
    }

    if (auto* s = dynamic_cast<BlockStmt*>(stmt))
    {
        Environment blockEnv;
        blockEnv.parent = env;
        for (const auto& st : s->statements)
            executeStatement(st.get(), &blockEnv);
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

    if (auto* e = dynamic_cast<VariableExpr*>(expr))
        return env->get(e->name.lexeme);

    if (auto* e = dynamic_cast<AssignExpr*>(expr))
    {
        Value val = evaluateExpr(e->value.get(), env);
        env->assign(e->name.lexeme, val);
        return val;
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(expr))
    {
        Value lv = evaluateExpr(e->left.get(), env);
        Value rv = evaluateExpr(e->right.get(), env);

        if (!std::holds_alternative<double>(lv) || !std::holds_alternative<double>(rv))
            throw std::runtime_error("Type error: operands must be numbers");

        double l = std::get<double>(lv);
        double r = std::get<double>(rv);

        switch (e->op.type)
        {
            case TokenType::PLUS:  return l + r;
            case TokenType::MINUS: return l - r;
            case TokenType::STAR:  return l * r;
            case TokenType::SLASH: return l / r;
            case TokenType::LESS:  return l < r;
            default:               return std::monostate{};
        }
    }

    return std::monostate{};
}
