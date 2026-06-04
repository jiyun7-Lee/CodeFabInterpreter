#include "Parser.h"
#include "Expr.h"
#include <stdexcept>

// -----------------------------------------------------------------------
// [재귀 하강 파서 (Recursive Descent Parser) 개요]
//
// 재귀 하강 파서는 문법 규칙을 함수 호출 계층으로 표현한다.
// 우선순위가 낮은 규칙일수록 상위 함수가 되고,
// 우선순위가 높은 규칙일수록 하위 함수가 된다.
//
// 호출 체인 (위로 갈수록 우선순위 낮음 / 아래로 갈수록 우선순위 높음):
//
//   parseExpression
//     parseAssignment   (=)
//       parseOr         (or)
//         parseAnd      (and)
//           parseComparison  (> <)
//             parseTerm      (+ -)
//               parseFactor  (* /)
//                 parseUnary   (단항 -, !)
//                   parsePrimary  (리터럴, 변수, 괄호)
//
// 예) 1 + 2 * 3 을 파싱하면:
//   parseTerm 이 parseFactor 를 두 번 호출하는데,
//   두 번째 parseFactor 안에서 * 가 먼저 묶이므로
//   결과 트리의 루트는 + 가 되고 * 는 오른쪽 자식이 된다.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// public
// -----------------------------------------------------------------------

std::vector<std::unique_ptr<Stmt>> Parser::parse(const std::vector<Token>& tokens)
{
    tokens_  = tokens;
    current_ = 0;

    // EOF 토큰을 만날 때까지 Statement 를 반복 파싱하여 AST 목록을 구성한다.
    // C파트 구현 시 parseStatement() → parseDeclaration() 으로 교체 예정
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd())
        statements.push_back(parseStatement());

    return statements;
}

// -----------------------------------------------------------------------
// Statement  (C파트 구현 예정 — 현재는 ExpressionStmt 만 처리)
// -----------------------------------------------------------------------

std::unique_ptr<Stmt> Parser::parseDeclaration()
{
    return parseStatement();
}

std::unique_ptr<Stmt> Parser::parseStatement()
{
    // C파트 구현 예정: VAR / PRINT / IF / FOR / BLOCK 분기 추가
    return parseExpressionStatement();
}

std::unique_ptr<Stmt> Parser::parseVarDeclaration()   { return nullptr; }
std::unique_ptr<Stmt> Parser::parsePrintStatement()   { return nullptr; }
std::unique_ptr<Stmt> Parser::parseIfStatement()      { return nullptr; }
std::unique_ptr<Stmt> Parser::parseForStatement()     { return nullptr; }
std::unique_ptr<Stmt> Parser::parseBlock()            { return nullptr; }

std::unique_ptr<Stmt> Parser::parseExpressionStatement()
{
    // Expression 을 파싱한 뒤 반드시 세미콜론(;) 으로 닫혀야 한다.
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "';' 가 필요합니다.");

    auto stmt        = std::make_unique<ExpressionStmt>();
    stmt->expression = std::move(expr);
    return stmt;
}

// -----------------------------------------------------------------------
// Expression — 우선순위 낮은 순서대로 호출  (B파트 구현)
// -----------------------------------------------------------------------

std::unique_ptr<Expr> Parser::parseExpression()
{
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment()
{
    // 우선 하위 우선순위 표현식(parseOr) 을 파싱한다.
    // 이후 = 토큰이 나오면 대입식으로 확정한다.
    //
    // [우결합 처리]
    // 우변에 재귀적으로 parseAssignment 를 호출하므로
    // a = b = 3 은 a = (b = 3) 으로 파싱된다.
    auto expr = parseOr();

    if (match({ TokenType::EQUAL }))
    {
        Token op    = previous(); // 연산자를 재귀 호출 이전에 캡처 (인자 평가 순서 보장)
        auto  value = parseAssignment(); // 우결합: 재귀 호출

        // 대입의 좌변은 반드시 변수(VariableExpr) 여야 한다.
        if (auto* var = dynamic_cast<VariableExpr*>(expr.get()))
        {
            auto assign   = std::make_unique<AssignExpr>();
            assign->name  = var->name;
            assign->value = std::move(value);
            return assign;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseOr()
{
    // or 연산자는 좌결합이므로 while 로 반복 처리한다.
    auto expr = parseAnd();
    while (match({ TokenType::OR }))
    {
        Token op = previous();
        expr = makeBinary(std::move(expr), op, parseAnd());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAnd()
{
    auto expr = parseComparison();
    while (match({ TokenType::AND }))
    {
        Token op = previous();
        expr = makeBinary(std::move(expr), op, parseComparison());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison()
{
    auto expr = parseTerm();
    while (match({ TokenType::GREATER, TokenType::LESS }))
    {
        Token op = previous();
        expr = makeBinary(std::move(expr), op, parseTerm());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm()
{
    auto expr = parseFactor();
    while (match({ TokenType::PLUS, TokenType::MINUS }))
    {
        Token op = previous();
        expr = makeBinary(std::move(expr), op, parseFactor());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor()
{
    auto expr = parseUnary();
    while (match({ TokenType::STAR, TokenType::SLASH }))
    {
        Token op = previous();
        expr = makeBinary(std::move(expr), op, parseUnary());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary()
{
    if (match({ TokenType::MINUS, TokenType::BANG }))
    {
        Token op    = previous();
        auto  right = parseUnary();
        auto  unary = std::make_unique<UnaryExpr>();
        unary->op    = op;
        unary->right = std::move(right);
        return unary;
    }
    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary()
{
    if (match({ TokenType::NUMBER, TokenType::STRING }))
    {
        auto lit   = std::make_unique<LiteralExpr>();
        lit->value = previous().literal;
        return lit;
    }

    if (match({ TokenType::TRUE }))
    {
        auto lit   = std::make_unique<LiteralExpr>();
        lit->value = true;
        return lit;
    }

    if (match({ TokenType::FALSE }))
    {
        auto lit   = std::make_unique<LiteralExpr>();
        lit->value = false;
        return lit;
    }

    if (match({ TokenType::IDENTIFIER }))
    {
        auto var  = std::make_unique<VariableExpr>();
        var->name = previous();
        return var;
    }

    if (match({ TokenType::LEFT_PAREN }))
    {
        auto expr       = parseExpression();
        consume(TokenType::RIGHT_PAREN, "')' 가 필요합니다.");
        auto grp        = std::make_unique<GroupingExpr>();
        grp->expression = std::move(expr);
        return grp;
    }

    throw std::runtime_error(
        "[" + std::to_string(peek().line) + "번째 줄] "
        "예상치 못한 토큰: '" + peek().lexeme + "'");
}

// -----------------------------------------------------------------------
// 노드 생성 헬퍼
// -----------------------------------------------------------------------

std::unique_ptr<BinaryExpr> Parser::makeBinary(
    std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
{
    if (!left || !right)
        throw std::runtime_error(
            "[" + std::to_string(op.line) + "번째 줄] "
            "'" + op.lexeme + "' 연산자의 피연산자가 없습니다.");

    auto bin   = std::make_unique<BinaryExpr>();
    bin->left  = std::move(left);
    bin->op    = op;
    bin->right = std::move(right);
    return bin;
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

bool Parser::check(TokenType type) const
{
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd() const
{
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::advance()
{
    if (!isAtEnd()) current_++;
    return previous();
}

Token Parser::peek() const
{
    return tokens_[current_];
}

Token Parser::previous() const
{
    return tokens_[current_ - 1];
}

Token Parser::consume(TokenType type, const std::string& msg)
{
    if (check(type)) return advance();
    throw std::runtime_error(msg);
}
