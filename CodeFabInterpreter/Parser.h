#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "Token.h"
#include "Stmt.h"

class Parser
{
public:
    std::vector<Stmt*> parse(const std::vector<Token>& tokens);

protected:
    // B파트 구현 예정. 테스트에서 FakeExprParser로 override 가능
    virtual Expr* parseExpression();

    std::vector<Token> tokens_;
    int current_ = 0;

    bool  isAtEnd()  const;
    Token peek()     const;
    Token previous() const;
    Token advance();
    bool  check(TokenType type) const;
    bool  match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& msg);

private:
    Stmt* parseDeclaration();
    Stmt* parseStatement();
    Stmt* parseVarDeclaration();
    Stmt* parsePrintStatement();
    Stmt* parseIfStatement();
    Stmt* parseForStatement();
    Stmt* parseBlock();
    Stmt* parseExpressionStatement();
};
