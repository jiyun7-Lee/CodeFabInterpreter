#include "Executor.h"
#include "DebugController.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

// ReturnSignal 등 예외 발생 시에도 depth_ 가 복원되도록 보장
struct DepthGuard
{
    int& depth;
    DepthGuard(int& d) : depth(d) { ++depth; }
    ~DepthGuard()                 { --depth; }
};

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
    {
        try { executeStatement(stmt.get(), &globalEnv); }
        catch (const std::runtime_error& e)
        {
            throw std::runtime_error(
                "[" + std::to_string(currentLine_) + "번째 줄] " + e.what());
        }
    }
}

void Executor::executeStatement(Stmt* stmt, Environment* env)
{
    if (stmt->line != 0) currentLine_ = stmt->line;
    if (debug_) debug_->beforeExecute(stmt, env, depth_);

    if (auto* s = dynamic_cast<ExpressionStmt*>(stmt))
    {
        evaluateExpr(s->expression.get(), env);
        return;
    }

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
        DepthGuard g(depth_);
        if (isTruthy(evaluateExpr(s->condition.get(), env)))
            executeStatement(s->thenBranch.get(), env);
        else if (s->elseBranch)
            executeStatement(s->elseBranch.get(), env);
        return;
    }

    if (auto* s = dynamic_cast<ForStmt*>(stmt))
    {
        // Checker가 for-init 변수를 별도 스코프에 등록하므로
        // Executor도 대응하는 Environment를 생성해 distance 값과 환경 체인을 일치시킨다.
        Environment forEnv;
        forEnv.parent = env;
        DepthGuard g(depth_);
        if (s->init)
            executeStatement(s->init.get(), &forEnv);
        while (!s->condition || isTruthy(evaluateExpr(s->condition.get(), &forEnv)))
        {
            executeStatement(s->body.get(), &forEnv);
            if (s->increment)
                evaluateExpr(s->increment.get(), &forEnv);
        }
        return;
    }

    if (auto* s = dynamic_cast<BlockStmt*>(stmt))
    {
        Environment blockEnv;
        blockEnv.parent = env;
        DepthGuard g(depth_);
        for (const auto& st : s->statements)
            executeStatement(st.get(), &blockEnv);
        return;
    }

    if (auto* s = dynamic_cast<FunctionDeclareStmt*>(stmt))
    {
        functions_[s->name.lexeme] = {s->params, std::move(s->body)};
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(stmt))
    {
        Value val = s->value ? evaluateExpr(s->value.get(), env) : std::monostate{};
        throw ReturnSignal{val};
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
        else if constexpr (std::is_same_v<T, std::monostate>)
            std::cout << "null\n";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ArrayValue>>)
        {
            std::cout << "[";
            for (size_t i = 0; i < v->elements.size(); ++i)
            {
                if (i > 0) std::cout << ", ";
                std::visit([](const auto& elem) {
                    using ET = std::decay_t<decltype(elem)>;
                    if constexpr (std::is_same_v<ET, double> || std::is_same_v<ET, std::string>)
                        std::cout << elem;
                    else if constexpr (std::is_same_v<ET, bool>)
                        std::cout << (elem ? "true" : "false");
                    else if constexpr (std::is_same_v<ET, std::monostate>)
                        std::cout << "null";
                }, v->elements[i]);
            }
            std::cout << "]\n";
        }
    }, val);
}

Value Executor::evaluateExpr(Expr* expr, Environment* env)
{
    if (auto* e = dynamic_cast<LiteralExpr*>(expr))
        return e->value;

    if (auto* e = dynamic_cast<VariableExpr*>(expr))
        return e->distance >= 0 ? env->getAt(e->distance, e->name.lexeme)
                                : env->get(e->name.lexeme, e->name.line);

    if (auto* e = dynamic_cast<GroupingExpr*>(expr))
        return evaluateExpr(e->expression.get(), env);

    if (auto* e = dynamic_cast<UnaryExpr*>(expr))
    {
        Value val = evaluateExpr(e->right.get(), env);
        if (e->op.type == TokenType::MINUS)
        {
            if (!std::holds_alternative<double>(val))
                throw std::runtime_error("피연산자는 반드시 숫자여야 한다.");
            return -std::get<double>(val);
        }
        if (e->op.type == TokenType::BANG)
            return !isTruthy(val);
        throw std::runtime_error("Executor error: unknown unary operator '" + e->op.lexeme + "'");
    }

    if (auto* e = dynamic_cast<AssignExpr*>(expr))
    {
        Value val = evaluateExpr(e->value.get(), env);
        if (e->distance >= 0)
            env->assignAt(e->distance, e->name.lexeme, val);
        else
            env->assign(e->name.lexeme, val, e->name.line);
        return val;
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(expr))
    {
        // AND는 단락 평가: 왼쪽이 falsy면 오른쪽 평가 없이 반환
        if (e->op.type == TokenType::AND)
        {
            Value lv = evaluateExpr(e->left.get(), env);
            if (!isTruthy(lv)) return lv;
            return evaluateExpr(e->right.get(), env);
        }

        // OR는 단락 평가: 왼쪽이 truthy면 오른쪽 평가 없이 반환
        if (e->op.type == TokenType::OR)
        {
            Value lv = evaluateExpr(e->left.get(), env);
            if (isTruthy(lv)) return lv;
            return evaluateExpr(e->right.get(), env);
        }

        Value lv = evaluateExpr(e->left.get(), env);
        Value rv = evaluateExpr(e->right.get(), env);

        switch (e->op.type)
        {
            case TokenType::PLUS:
            {
                if (std::holds_alternative<std::string>(lv) && std::holds_alternative<std::string>(rv))
                    return std::get<std::string>(lv) + std::get<std::string>(rv);
                if (std::holds_alternative<double>(lv) && std::holds_alternative<double>(rv))
                    return std::get<double>(lv) + std::get<double>(rv);
                throw std::runtime_error("'+' 연산자는 숫자+숫자 또는 문자열+문자열만 지원합니다.");
            }
            case TokenType::MINUS:
            case TokenType::STAR:
            case TokenType::SLASH:
            case TokenType::PERCENT:
            case TokenType::LESS:
            case TokenType::GREATER:
            {
                if (!std::holds_alternative<double>(lv) || !std::holds_alternative<double>(rv))
                    throw std::runtime_error("피연산자는 반드시 숫자여야 한다.");
                double l = std::get<double>(lv);
                double r = std::get<double>(rv);
                if (e->op.type == TokenType::MINUS)   return l - r;
                if (e->op.type == TokenType::STAR)    return l * r;
                if (e->op.type == TokenType::SLASH || e->op.type == TokenType::PERCENT)
                {
                    if (r == 0.0) throw std::runtime_error("0으로 나눈 오류");
                    return e->op.type == TokenType::SLASH ? l / r : std::fmod(l, r);
                }
                if (e->op.type == TokenType::LESS)    return l < r;
                if (e->op.type == TokenType::GREATER) return l > r;
            }
            default: return std::monostate{};
        }
    }

    if (auto* e = dynamic_cast<FunctionCallExpr*>(expr))
    {
        // 빌트인 Array(n) — 고정 크기 배열 생성, 초기값 null
        // 사용자가 `func Array(n) { ... }` 를 선언하면 functions_ 에 등록되어
        // 해당 조건이 false 가 되므로 사용자 정의 함수가 빌트인을 오버라이드한다.
        // 이는 의도된 동작이다 — 사용자 정의가 항상 빌트인보다 우선한다.
        if (e->callee.lexeme == "Array" && functions_.find("Array") == functions_.end())
        {
            if (e->args.size() != 1)
                throw std::runtime_error("Array(): 인자가 1개 필요합니다.");
            Value sizeVal = evaluateExpr(e->args[0].get(), env);
            if (!std::holds_alternative<double>(sizeVal))
                throw std::runtime_error("Array(): 배열 크기는 숫자여야 합니다.");
            int size = static_cast<int>(std::get<double>(sizeVal));
            if (size < 0)
                throw std::runtime_error("Array(): 배열 크기는 0 이상이어야 합니다.");
            auto arr = std::make_shared<ArrayValue>();
            arr->elements.resize(static_cast<size_t>(size), std::monostate{});
            return arr;
        }

        auto it = functions_.find(e->callee.lexeme);
        if (it == functions_.end())
            throw std::runtime_error("Undefined function '" + e->callee.lexeme + "'");
        const auto& fn = it->second;
        if (e->args.size() != fn.params.size())
            throw std::runtime_error("'" + e->callee.lexeme + "': wrong number of arguments");

        Environment fnEnv;
        fnEnv.parent = &globalEnv;
        for (size_t i = 0; i < fn.params.size(); ++i)
            fnEnv.define(fn.params[i].lexeme, evaluateExpr(e->args[i].get(), env));

        DepthGuard g(depth_);  // 추가
        try { executeStatement(fn.body.get(), &fnEnv); }
        catch (const ReturnSignal& ret) { return ret.value; }
        return std::monostate{};
    }

    if (auto* e = dynamic_cast<ArrayLiteralExpr*>(expr))
    {
        auto arr = std::make_shared<ArrayValue>();
        for (const auto& elem : e->elements)
            arr->elements.push_back(evaluateExpr(elem.get(), env));
        return arr;
    }

    if (auto* e = dynamic_cast<ArrayAccessExpr*>(expr))
    {
        Value arrVal = evaluateExpr(e->array.get(), env);
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal))
            throw std::runtime_error("Type error: not an array");
        auto arr     = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        Value idxVal = evaluateExpr(e->index.get(), env);
        if (!std::holds_alternative<double>(idxVal))
            throw std::runtime_error("Type error: array index must be a number");
        int idx = static_cast<int>(std::get<double>(idxVal));
        if (idx < 0 || idx >= static_cast<int>(arr->elements.size()))
            throw std::runtime_error("Array index out of bounds");
        return arr->elements[idx];
    }

    if (auto* e = dynamic_cast<ArrayWriteExpr*>(expr))
    {
        Value arrVal = evaluateExpr(e->array.get(), env);
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal))
            throw std::runtime_error("Type error: not an array");
        auto arr     = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        Value idxVal = evaluateExpr(e->index.get(), env);
        if (!std::holds_alternative<double>(idxVal))
            throw std::runtime_error("Type error: array index must be a number");
        int idx = static_cast<int>(std::get<double>(idxVal));
        if (idx < 0 || idx >= static_cast<int>(arr->elements.size()))
            throw std::runtime_error("Array index out of bounds");
        Value newVal = evaluateExpr(e->value.get(), env);
        arr->elements[idx] = newVal;
        return newVal;
    }

    return std::monostate{};
}
