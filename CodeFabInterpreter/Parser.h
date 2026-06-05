#pragma once
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <initializer_list>
#include "Token.h"
#include "Stmt.h"

class Parser
{
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens);

protected:
    // B파트 구현. 테스트에서 FakeExprParser로 override 가능
    virtual std::unique_ptr<Expr> parseExpression();

    std::vector<Token> m_tokens;
    int m_current = 0;

    bool  isAtEnd()  const;
    Token peek()     const;
    Token previous() const;
    Token advance();
    bool  check(TokenType type) const;
    bool  match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& msg = "");

private:
    // Statement (C파트)
    std::unique_ptr<Stmt> parseDeclaration();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseVarDeclaration();
    std::unique_ptr<Stmt> parsePrintStatement();
    std::unique_ptr<Stmt> parseIfStatement();
    std::unique_ptr<Stmt> parseForStatement();
    std::unique_ptr<Stmt> parseBlock();
    std::unique_ptr<Stmt> parseExpressionStatement();
    std::unique_ptr<Stmt> parseFunctionDeclaration();
    std::unique_ptr<Stmt> parseReturnStatement();

    // Expression (B파트)
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseOr();
    std::unique_ptr<Expr> parseAnd();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();

    std::unique_ptr<BinaryExpr> makeBinary(
        std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right);
};
