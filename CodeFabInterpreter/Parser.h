#pragma once
#include <vector>
#include <memory>
#include <initializer_list>
#include "Token.h"
#include "Stmt.h"

class Parser
{
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens);

private:
    std::vector<Token> m_tokens;
    int                m_current = 0;

    // --- Statement ---
    std::unique_ptr<Stmt>  parseStatement();
    std::unique_ptr<Stmt>  parseExpressionStatement();

    // --- Expression (우선순위 낮은 순 → 높은 순) ---
    std::unique_ptr<Expr>  parseExpression();
    std::unique_ptr<Expr>  parseAssignment();   // =
    std::unique_ptr<Expr>  parseOr();           // or
    std::unique_ptr<Expr>  parseAnd();          // and
    std::unique_ptr<Expr>  parseComparison();   // > <
    std::unique_ptr<Expr>  parseTerm();         // + -
    std::unique_ptr<Expr>  parseFactor();       // * /
    std::unique_ptr<Expr>  parseUnary();        // unary -, !
    std::unique_ptr<Expr>  parsePrimary();      // 리터럴, 변수, 괄호

    // --- 노드 생성 헬퍼 ---
    std::unique_ptr<BinaryExpr> makeBinary(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right);

    // --- 토큰 헬퍼 ---
    bool   match(std::initializer_list<TokenType> types);
    bool   check(TokenType type);
    bool   isAtEnd();
    Token  advance();
    Token  peek();
    Token  previous();
    Token  consume(TokenType type);
};
