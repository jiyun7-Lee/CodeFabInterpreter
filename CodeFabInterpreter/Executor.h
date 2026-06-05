#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "Stmt.h"
#include "Environment.h"

// return 문 처리용 신호 — std::exception 계층 밖으로 throw/catch
struct ReturnSignal { Value value; };

struct FunctionValue
{
    std::vector<Token>     params;
    std::unique_ptr<Stmt>  body;
};

class Executor
{
public:
    void execute(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    Environment                                  globalEnv;
    std::unordered_map<std::string, FunctionValue> functions_;

    void  executeStatement(Stmt* stmt, Environment* env);
    Value evaluateExpr(Expr* expr, Environment* env);
    void  printValue(const Value& val);
};
