#pragma once
#include <memory>
#include "Token.h"
#include "Value.h"

class Expr
{
public:
    virtual ~Expr() = default;
};

class LiteralExpr : public Expr
{
public:
    Value value;
};

class VariableExpr : public Expr
{
public:
    Token name;
};

class UnaryExpr : public Expr
{
public:
    Token                  op;
    std::unique_ptr<Expr>  right;
};

class BinaryExpr : public Expr
{
public:
    std::unique_ptr<Expr>  left;
    Token                  op;
    std::unique_ptr<Expr>  right;
};

class AssignExpr : public Expr
{
public:
    Token                  name;
    std::unique_ptr<Expr>  value;
};

class GroupingExpr : public Expr
{
public:
    std::unique_ptr<Expr>  expression;
};

class FunctionCallExpr : public Expr
{
public:
    Token                                callee;
    std::vector<std::unique_ptr<Expr>>   args;
};

class ArrayLiteralExpr : public Expr
{
public:
    std::vector<std::unique_ptr<Expr>>   elements;
};

class ArrayAccessExpr : public Expr
{
public:
    std::unique_ptr<Expr>  array;
    std::unique_ptr<Expr>  index;
};
