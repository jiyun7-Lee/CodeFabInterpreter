#pragma once
#include <memory>
#include <vector>
#include "Token.h"
#include "Value.h"

// -----------------------------------------------------------------------
// 전방 선언 — ExprVisitor 인터페이스에서 참조 타입으로 사용하기 위해 필요
// -----------------------------------------------------------------------
class LiteralExpr;
class VariableExpr;
class UnaryExpr;
class BinaryExpr;
class AssignExpr;
class GroupingExpr;
class FunctionCallExpr;
class ArrayLiteralExpr;
class ArrayAccessExpr;
class ArrayWriteExpr;

// -----------------------------------------------------------------------
// ExprVisitor — Visitor 패턴 인터페이스 (GoF Style A: 구체 노드별 visit 메서드)
// Executor 가 구현하여 dynamic_cast 없이 노드 타입별 평가를 수행한다.
// -----------------------------------------------------------------------
class ExprVisitor
{
public:
    virtual ~ExprVisitor() = default;
    virtual Value visitLiteral      (LiteralExpr&       e) = 0;
    virtual Value visitVariable     (VariableExpr&      e) = 0;
    virtual Value visitUnary        (UnaryExpr&         e) = 0;
    virtual Value visitBinary       (BinaryExpr&        e) = 0;
    virtual Value visitAssign       (AssignExpr&        e) = 0;
    virtual Value visitGrouping     (GroupingExpr&      e) = 0;
    virtual Value visitFunctionCall (FunctionCallExpr&  e) = 0;
    virtual Value visitArrayLiteral (ArrayLiteralExpr&  e) = 0;
    virtual Value visitArrayAccess  (ArrayAccessExpr&   e) = 0;
    virtual Value visitArrayWrite   (ArrayWriteExpr&    e) = 0;
};

// -----------------------------------------------------------------------
// Expr — 모든 표현식 노드의 추상 기반 클래스
// -----------------------------------------------------------------------
class Expr
{
public:
    virtual ~Expr() = default;
    virtual Value accept(ExprVisitor& v) = 0;
};

// -----------------------------------------------------------------------
// 구체 노드 — accept() 는 대응하는 visit 메서드로 이중 디스패치
// -----------------------------------------------------------------------

class LiteralExpr : public Expr
{
public:
    Value value;
    Value accept(ExprVisitor& v) override { return v.visitLiteral(*this); }
};

class VariableExpr : public Expr
{
public:
    Token name;
    int   distance = -1;
    Value accept(ExprVisitor& v) override { return v.visitVariable(*this); }
};

class UnaryExpr : public Expr
{
public:
    Token                  op;
    std::unique_ptr<Expr>  right;
    Value accept(ExprVisitor& v) override { return v.visitUnary(*this); }
};

class BinaryExpr : public Expr
{
public:
    std::unique_ptr<Expr>  left;
    Token                  op;
    std::unique_ptr<Expr>  right;
    Value accept(ExprVisitor& v) override { return v.visitBinary(*this); }
};

class AssignExpr : public Expr
{
public:
    Token                  name;
    std::unique_ptr<Expr>  value;
    int                    distance = -1;
    Value accept(ExprVisitor& v) override { return v.visitAssign(*this); }
};

class GroupingExpr : public Expr
{
public:
    std::unique_ptr<Expr>  expression;
    Value accept(ExprVisitor& v) override { return v.visitGrouping(*this); }
};

class FunctionCallExpr : public Expr
{
public:
    Token                                callee;
    std::vector<std::unique_ptr<Expr>>   args;
    Value accept(ExprVisitor& v) override { return v.visitFunctionCall(*this); }
};

class ArrayLiteralExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>>   elements;
    Value accept(ExprVisitor& v) override { return v.visitArrayLiteral(*this); }
};

class ArrayAccessExpr : public Expr
{
public:
    std::unique_ptr<Expr>  array;
    std::unique_ptr<Expr>  index;
    Value accept(ExprVisitor& v) override { return v.visitArrayAccess(*this); }
};

class ArrayWriteExpr : public Expr
{
public:
    std::unique_ptr<Expr>  array;
    std::unique_ptr<Expr>  index;
    std::unique_ptr<Expr>  value;
    Value accept(ExprVisitor& v) override { return v.visitArrayWrite(*this); }
};
