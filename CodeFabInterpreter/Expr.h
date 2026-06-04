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
