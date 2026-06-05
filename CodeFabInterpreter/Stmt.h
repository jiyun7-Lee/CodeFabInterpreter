#pragma once
#include <vector>
#include <memory>
#include "Expr.h"

class Stmt
{
public:
    int line = 0;
    virtual ~Stmt() = default;
};

class ExpressionStmt : public Stmt
{
public:
    std::unique_ptr<Expr>  expression;
};

class PrintStmt : public Stmt
{
public:
    std::unique_ptr<Expr>  expression;
};

class VarDeclareStmt : public Stmt
{
public:
    Token                  name;
    std::unique_ptr<Expr>  initializer;
};

class BlockStmt : public Stmt
{
public:
    std::vector<std::unique_ptr<Stmt>>  statements;
};

class IfStmt : public Stmt
{
public:
    std::unique_ptr<Expr>  condition;
    std::unique_ptr<Stmt>  thenBranch;
    std::unique_ptr<Stmt>  elseBranch;
};

class ForStmt : public Stmt
{
public:
    std::unique_ptr<Stmt>  init;
    std::unique_ptr<Expr>  condition;
    std::unique_ptr<Expr>  increment;
    std::unique_ptr<Stmt>  body;
};

class FunctionDeclareStmt : public Stmt
{
public:
    Token                  name;
    std::vector<Token>     params;
    std::unique_ptr<Stmt>  body;
};

class ReturnStmt : public Stmt
{
public:
    Token                  keyword;
    std::unique_ptr<Expr>  value;
};
