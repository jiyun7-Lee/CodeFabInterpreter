#include "Parser.h"
#include "Expr.h"

// -----------------------------------------------------------------------
// public
// -----------------------------------------------------------------------

std::vector<Stmt*> Parser::parse(const std::vector<Token>& tokens)
{
    m_tokens  = tokens;
    m_current = 0;

    std::vector<Stmt*> statements;
    while (!isAtEnd())
        statements.push_back(parseStatement());

    return statements;
}

// -----------------------------------------------------------------------
// Statement
// -----------------------------------------------------------------------

Stmt* Parser::parseStatement()
{
    return parseExpressionStatement();
}

Stmt* Parser::parseExpressionStatement()
{
    Expr* expr = parseExpression();
    consume(TokenType::SEMICOLON);

    auto* stmt = new ExpressionStmt();
    stmt->expression = expr;
    return stmt;
}

// -----------------------------------------------------------------------
// Expression — 우선순위 낮은 순서대로 호출
// -----------------------------------------------------------------------

Expr* Parser::parseExpression()
{
    return parseAssignment();
}

// a = expr  (우결합)
Expr* Parser::parseAssignment()
{
    Expr* expr = parseOr();

    if (match({ TokenType::EQUAL }))
    {
        Expr* value = parseAssignment();

        if (auto* var = dynamic_cast<VariableExpr*>(expr))
        {
            auto* assign  = new AssignExpr();
            assign->name  = var->name;
            assign->value = value;
            delete expr;
            return assign;
        }
    }
    return expr;
}

// or
Expr* Parser::parseOr()
{
    Expr* expr = parseAnd();

    while (match({ TokenType::OR }))
    {
        Token  op    = previous();
        Expr*  right = parseAnd();
        auto*  bin   = new BinaryExpr();
        bin->left  = expr;
        bin->op    = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

// and
Expr* Parser::parseAnd()
{
    Expr* expr = parseComparison();

    while (match({ TokenType::AND }))
    {
        Token  op    = previous();
        Expr*  right = parseComparison();
        auto*  bin   = new BinaryExpr();
        bin->left  = expr;
        bin->op    = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

// > <
Expr* Parser::parseComparison()
{
    Expr* expr = parseTerm();

    while (match({ TokenType::GREATER, TokenType::LESS }))
    {
        Token  op    = previous();
        Expr*  right = parseTerm();
        auto*  bin   = new BinaryExpr();
        bin->left  = expr;
        bin->op    = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

// + -
Expr* Parser::parseTerm()
{
    Expr* expr = parseFactor();

    while (match({ TokenType::PLUS, TokenType::MINUS }))
    {
        Token  op    = previous();
        Expr*  right = parseFactor();
        auto*  bin   = new BinaryExpr();
        bin->left  = expr;
        bin->op    = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

// * /
Expr* Parser::parseFactor()
{
    Expr* expr = parseUnary();

    while (match({ TokenType::STAR, TokenType::SLASH }))
    {
        Token  op    = previous();
        Expr*  right = parseUnary();
        auto*  bin   = new BinaryExpr();
        bin->left  = expr;
        bin->op    = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

// 단항 -
Expr* Parser::parseUnary()
{
    if (match({ TokenType::MINUS }))
    {
        Token  op    = previous();
        Expr*  right = parseUnary();
        auto*  unary = new UnaryExpr();
        unary->op    = op;
        unary->right = right;
        return unary;
    }
    return parsePrimary();
}

// 숫자·문자열·bool 리터럴, 변수, 괄호 묶음
Expr* Parser::parsePrimary()
{
    if (match({ TokenType::NUMBER, TokenType::STRING }))
    {
        auto* lit  = new LiteralExpr();
        lit->value = previous().literal;
        return lit;
    }

    if (match({ TokenType::TRUE }))
    {
        auto* lit  = new LiteralExpr();
        lit->value = true;
        return lit;
    }

    if (match({ TokenType::FALSE }))
    {
        auto* lit  = new LiteralExpr();
        lit->value = false;
        return lit;
    }

    if (match({ TokenType::IDENTIFIER }))
    {
        auto* var  = new VariableExpr();
        var->name  = previous();
        return var;
    }

    if (match({ TokenType::LEFT_PAREN }))
    {
        Expr* expr = parseExpression();
        consume(TokenType::RIGHT_PAREN);
        auto* grp       = new GroupingExpr();
        grp->expression = expr;
        return grp;
    }

    return nullptr;
}

// -----------------------------------------------------------------------
// 토큰 헬퍼
// -----------------------------------------------------------------------

bool Parser::match(std::initializer_list<TokenType> types)
{
    for (TokenType type : types)
    {
        if (check(type))
        {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type)
{
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd()
{
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::advance()
{
    if (!isAtEnd()) m_current++;
    return previous();
}

Token Parser::peek()
{
    return m_tokens[m_current];
}

Token Parser::previous()
{
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type)
{
    if (check(type)) return advance();
    return peek();
}
