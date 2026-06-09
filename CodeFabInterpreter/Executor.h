#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "Stmt.h"
#include "Expr.h"
#include "Environment.h"

// return 문 처리용 신호 — std::exception 계층 밖으로 throw/catch
struct ReturnSignal { Value value; };

struct FunctionValue
{
    std::vector<Token>     params;
    std::unique_ptr<Stmt>  body;
};

class DebugController; // forward declaration

class Executor : public ExprVisitor
{
public:
    void execute(const std::vector<std::unique_ptr<Stmt>>& statements);
    void execute(const std::vector<std::unique_ptr<Stmt>>& statements, Environment& env);
    void setDebugController(DebugController* ctrl) { debug_ = ctrl; }

private:
    Environment                                    globalEnv;
    std::unordered_map<std::string, FunctionValue> functions_;
    DebugController*                               debug_ = nullptr;
    int                                            currentLine_ = 0;
    int                                            depth_ = 0;
    Environment*                                   env_ = nullptr;  // 현재 평가 환경

    void  executeStatement(Stmt* stmt, Environment* env);
    Value evaluateExpr(Expr* expr, Environment* env);
    void  printValue(const Value& val);

    // ExprVisitor 구현 — evaluateExpr 이 env_ 를 설정한 뒤 accept() 를 통해 호출된다.
    Value visitLiteral      (LiteralExpr&       e) override;
    Value visitVariable     (VariableExpr&      e) override;
    Value visitUnary        (UnaryExpr&         e) override;
    Value visitBinary       (BinaryExpr&        e) override;
    Value visitAssign       (AssignExpr&        e) override;
    Value visitGrouping     (GroupingExpr&      e) override;
    Value visitFunctionCall (FunctionCallExpr&  e) override;
    Value visitArrayLiteral (ArrayLiteralExpr&  e) override;
    Value visitArrayAccess  (ArrayAccessExpr&   e) override;
    Value visitArrayWrite   (ArrayWriteExpr&    e) override;
};
