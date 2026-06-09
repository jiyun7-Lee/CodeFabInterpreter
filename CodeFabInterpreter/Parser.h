#pragma once
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <initializer_list>
#include "Token.h"
#include "Stmt.h"

class Parser; // IExprParser 선언에서 Parser& 참조 타입으로 사용하기 위해 필요

// -----------------------------------------------------------------------
// IExprParser — Strategy 패턴 인터페이스
// 표현식 파싱 전략을 Parser 상속 없이 교체할 수 있도록 한다.
// 전략 구현체는 Parser& ctx 를 통해 토큰 커서 메서드를 사용한다.
// -----------------------------------------------------------------------
class IExprParser
{
public:
    virtual ~IExprParser() = default;
    virtual std::unique_ptr<Expr> parseExpression(Parser& ctx) = 0;
};

class Parser
{
public:
    Parser() = default;
    // 표현식 파싱 전략을 주입한다. null 이면 기본 재귀 하강 구현을 사용한다.
    explicit Parser(IExprParser* exprStrategy);

    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens);

    // 토큰 커서 메서드 — IExprParser 구현체 전용.
    // IExprParser 구현체 외에서 직접 호출하면 토큰 스트림 위치가 어긋날 수 있다.
    bool  isAtEnd()  const;
    Token peek()     const;
    Token previous() const;
    Token advance();
    bool  check(TokenType type) const;
    bool  match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& msg = "");

protected:
    // B파트 진입점. 주입된 전략이 있으면 위임하고, 없으면 기본 구현(parseAssignment)을 사용한다.
    std::unique_ptr<Expr> parseExpression();

    std::vector<Token> m_tokens;
    int m_current = 0;

private:
    IExprParser* exprStrategy_ = nullptr;

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

    // Expression (B파트) — 기본 재귀 하강 구현
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
