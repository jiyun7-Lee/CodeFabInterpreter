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

    // --- Expression (우선순위 낮은 순 → 높은 순) ---
    Expr*  parseExpression();
    Expr*  parseAssignment();   // =
    Expr*  parseOr();           // or
    Expr*  parseAnd();          // and
    Expr*  parseComparison();   // > <
    Expr*  parseTerm();         // + -
    Expr*  parseFactor();       // * /
    Expr*  parseUnary();        // - (단항)
    Expr*  parsePrimary();      // 리터럴, 변수, 괄호

    // --- 토큰 헬퍼 ---
    bool   match(std::initializer_list<TokenType> types);
    bool   check(TokenType type);
    bool   isAtEnd();
    Token  advance();
    Token  peek();
    Token  previous();
    Token  consume(TokenType type);
};
