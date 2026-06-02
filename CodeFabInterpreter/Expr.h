#pragma once
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
    Token  op;
    Expr*  right;
};

class BinaryExpr : public Expr
{
public:
    Expr*  left;
    Token  op;
    Expr*  right;
};

class AssignExpr : public Expr
{
public:
    Token  name;
    Expr*  value;
};

class GroupingExpr : public Expr
{
public:
    Expr*  expression;
};
