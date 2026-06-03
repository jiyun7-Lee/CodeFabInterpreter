#pragma once
#include <vector>
#include <initializer_list>
#include "Token.h"
#include "Stmt.h"

class Parser
{
public:
    std::vector<Stmt*> parse(const std::vector<Token>& tokens);

private:
    std::vector<Token> m_tokens;
    int                m_current = 0;

    // --- Statement ---
    Stmt*  parseStatement();
    Stmt*  parseExpressionStatement();

    // --- Expression (ordered lowest to highest precedence) ---
    Expr*  parseExpression();
    Expr*  parseAssignment();   // =
    Expr*  parseOr();           // or
    Expr*  parseAnd();          // and
    Expr*  parseComparison();   // > <
    Expr*  parseTerm();         // + -
    Expr*  parseFactor();       // * /
    Expr*  parseUnary();        // unary -, !
    Expr*  parsePrimary();      // literals, variables, grouping

    // --- Node factory ---
    BinaryExpr* makeBinary(Expr* left, Token op, Expr* right);

    // --- Token helpers ---
    bool   match(std::initializer_list<TokenType> types);
    bool   check(TokenType type);
    bool   isAtEnd();
    Token  advance();
    Token  peek();
    Token  previous();
    Token  consume(TokenType type);
};
