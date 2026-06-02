#pragma once
#include <vector>
#include "Expr.h"

class Stmt
{
public:
    virtual ~Stmt() = default;
};

class ExpressionStmt : public Stmt
{
public:
    Expr* expression;
};

class PrintStmt : public Stmt
{
public:
    Expr* expression;
};

class VarDeclareStmt : public Stmt
{
public:
    Token name;
    Expr* initializer;
};

class BlockStmt : public Stmt
{
public:
    std::vector<Stmt*> statements;
};

class IfStmt : public Stmt
{
public:
    Expr* condition;
    Stmt* thenBranch;
    Stmt* elseBranch;
};

class ForStmt : public Stmt
{
public:
    Stmt* init;
    Expr* condition;
    Expr* increment;
    Stmt* body;
};
