#include "Parser.h"
#include "Expr.h"
#include <stdexcept>
#include <iostream>

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
    m_tokens  = tokens;
    m_current = 0;

    // EOF 토큰을 만날 때까지 Statement 를 반복 파싱하여 AST 목록을 구성한다.
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd())
        statements.push_back(parseStatement());

    return statements;
}

// -----------------------------------------------------------------------
// Statement
// -----------------------------------------------------------------------

std::unique_ptr<Stmt> Parser::parseStatement()
{
    // 현재는 ExpressionStmt 만 지원한다.
    // PrintStmt, VarDeclareStmt, IfStmt, ForStmt 등은 심민구 담당 (Statement Parser).
    return parseExpressionStatement();
}

std::unique_ptr<Stmt> Parser::parseExpressionStatement()
{
    // Expression 을 파싱한 뒤 반드시 세미콜론(;) 으로 닫혀야 한다.
    // ExpressionStmt 는 Expr 을 Stmt 로 감싸는 래퍼 역할을 한다.
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON);

    auto stmt = std::make_unique<ExpressionStmt>();
    stmt->expression = std::move(expr);
    return stmt;
}

// -----------------------------------------------------------------------
// Expression — 우선순위 낮은 순서대로 호출
// -----------------------------------------------------------------------

std::unique_ptr<Expr> Parser::parseExpression()
{
    // 모든 Expression 파싱의 진입점.
    // 가장 우선순위가 낮은 parseAssignment 부터 시작한다.
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
        Token op = previous(); // 연산자를 재귀 호출 이전에 캡처 (인자 평가 순서 보장)
        auto value = parseAssignment(); // 우결합: 재귀 호출

        // 대입의 좌변은 반드시 변수(VariableExpr) 여야 한다.
        // 예) 3 = 5 는 문법 오류 — Checker 단계에서 추가 검증 예정.
        if (auto* var = dynamic_cast<VariableExpr*>(expr.get()))
        {
            auto assign   = std::make_unique<AssignExpr>();
            assign->name  = var->name;
            assign->value = std::move(value);
            // expr(unique_ptr) 는 스코프 종료 시 자동 해제됨
            return assign;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseOr()
{
    // or 연산자는 좌결합이므로 while 로 반복 처리한다.
    // 예) a or b or c → BinaryExpr(or, BinaryExpr(or, a, b), c)
    auto expr = parseAnd();
    while (match({ TokenType::OR }))
    {
        Token op = previous(); // 연산자를 재귀 호출 이전에 캡처
        expr = makeBinary(std::move(expr), op, parseAnd());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAnd()
{
    // and 연산자도 좌결합.
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
    // 비교 연산자 > < 처리.
    // 예) a > b → BinaryExpr(>, VariableExpr(a), VariableExpr(b))
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
    // 덧셈·뺄셈 처리.
    // parseFactor 를 먼저 호출하므로 * / 가 + - 보다 높은 우선순위를 갖는다.
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
    // 곱셈·나눗셈 처리.
    // parseUnary 를 먼저 호출하므로 단항 연산자가 * / 보다 높은 우선순위를 갖는다.
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
    // 단항 연산자 처리: - (음수), ! (논리 부정)
    // 재귀 호출로 !!a 같은 중첩 단항 연산도 처리된다.
    if (match({ TokenType::MINUS, TokenType::BANG }))
    {
        Token op    = previous();
        auto  right = parseUnary(); // 재귀: --a → UnaryExpr(-, UnaryExpr(-, a))
        auto  unary = std::make_unique<UnaryExpr>();
        unary->op    = op;
        unary->right = std::move(right);
        return unary;
    }
    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary()
{
    // 가장 기본 단위(원자) 파싱. 더 이상 분해되지 않는 토큰들을 처리한다.

    // 숫자·문자열 리터럴: 토큰의 literal 필드에 값이 담겨있다.
    if (match({ TokenType::NUMBER, TokenType::STRING }))
    {
        auto lit  = std::make_unique<LiteralExpr>();
        lit->value = previous().literal;
        return lit;
    }

    // 불리언 리터럴: TRUE/FALSE 토큰을 bool 값으로 변환
    if (match({ TokenType::TRUE }))
    {
        auto lit  = std::make_unique<LiteralExpr>();
        lit->value = true;
        return lit;
    }

    if (match({ TokenType::FALSE }))
    {
        auto lit  = std::make_unique<LiteralExpr>();
        lit->value = false;
        return lit;
    }

    // 식별자(변수명): lexeme 을 그대로 보관하고 Executor 가 실행 시 값을 조회한다.
    if (match({ TokenType::IDENTIFIER }))
    {
        auto var  = std::make_unique<VariableExpr>();
        var->name = previous();
        return var;
    }

    // 괄호 묶음: 내부 표현식을 GroupingExpr 로 감싸서 우선순위를 명시적으로 높인다.
    // 예) (1 + 2) * 3 → parseFactor 가 GroupingExpr 를 하나의 단위로 인식
    if (match({ TokenType::LEFT_PAREN }))
    {
        auto expr = parseExpression(); // 괄호 안은 다시 최상위부터 파싱
        consume(TokenType::RIGHT_PAREN);
        auto grp        = std::make_unique<GroupingExpr>();
        grp->expression = std::move(expr);
        return grp;
    }

    // 매칭되는 토큰 없음 — 파싱 불가능한 토큰이므로 예외를 던진다.
    // nullptr 을 반환하면 상위 호출자(parseUnary, makeBinary 등)에서
    // 역참조 크래시가 발생하므로 즉시 중단한다.
    throw std::runtime_error(
        "[" + std::to_string(peek().line) + "번째 줄] "
        "예상치 못한 토큰: '" + peek().lexeme + "'");
}

// -----------------------------------------------------------------------
// 노드 생성 헬퍼
// -----------------------------------------------------------------------

std::unique_ptr<BinaryExpr> Parser::makeBinary(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
{
    // parseOr/And/Comparison/Term/Factor 에서 공통으로 사용하는 BinaryExpr 생성.
    // parsePrimary 가 throw 로 막혀있더라도, 향후 호출 경로가 추가될 경우를 대비해
    // nullptr 피연산자가 Executor 에 전달되지 않도록 진입 시점에서 한 번 더 검사한다.
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
    // 현재 토큰이 주어진 타입 중 하나이면 소비(advance) 하고 true 를 반환한다.
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
    // 현재 토큰 타입을 확인만 하고 소비하지 않는다 (peek 와 같은 역할).
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd()
{
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::advance()
{
    // 현재 토큰을 소비하고 이전 토큰을 반환한다.
    if (!isAtEnd()) m_current++;
    return previous();
}

Token Parser::peek()
{
    // 현재 위치의 토큰을 소비하지 않고 반환한다.
    return m_tokens[m_current];
}

Token Parser::previous()
{
    // 가장 최근에 소비한 토큰을 반환한다. match() 직후 연산자 확인에 사용.
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type)
{
    // 기대하는 토큰이 맞으면 소비하고 반환한다.
    // 틀린 경우 stderr 에 경고를 출력하고 현재 토큰을 반환한다.
    if (check(type)) return advance();
    std::cerr << "[" << peek().line << "번째 줄] "
              << "예상치 못한 토큰: '" << peek().lexeme << "'\n";
    return peek();
}
