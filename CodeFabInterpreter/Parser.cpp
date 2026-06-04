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
    m_tokens  = tokens;
    m_current = 0;

    // EOF 토큰을 만날 때까지 Declaration 을 반복 파싱하여 AST 목록을 구성한다.
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd())
        statements.push_back(parseDeclaration());

    return statements;
}

// -----------------------------------------------------------------------
// Statement  (C파트)
// -----------------------------------------------------------------------

std::unique_ptr<Stmt> Parser::parseDeclaration()
{
    // var 선언은 Statement 가 아닌 Declaration 계층에서 처리한다.
    // 블록 내부에서도 var 를 쓸 수 있도록 parseBlock() 도 이 함수를 호출한다.
    if (match({ TokenType::FUNC }))
        return parseFunctionDeclaration();
    if (match({ TokenType::VAR }))
        return parseVarDeclaration();
    return parseStatement();
}

std::unique_ptr<Stmt> Parser::parseStatement()
{
    // 첫 토큰으로 문장 종류를 판별하는 디스패치 테이블
    if (match({ TokenType::PRINT }))       return parsePrintStatement();
    if (match({ TokenType::IF }))          return parseIfStatement();
    if (match({ TokenType::FOR }))         return parseForStatement();
    if (match({ TokenType::LEFT_BRACE }))  return parseBlock();
    if (match({ TokenType::RETURN }))      return parseReturnStatement();
    return parseExpressionStatement();
}

std::unique_ptr<Stmt> Parser::parseVarDeclaration()
{
    // VAR 는 이미 소비된 상태로 진입. 문법: var IDENTIFIER = expr ;
    Token name = consume(TokenType::IDENTIFIER, "변수 이름이 필요합니다.");
    consume(TokenType::EQUAL, "'=' 가 필요합니다.");
    auto initializer = parseExpression();
    consume(TokenType::SEMICOLON, "';' 가 필요합니다.");

    auto stmt         = std::make_unique<VarDeclareStmt>();
    stmt->name        = name;
    stmt->initializer = std::move(initializer);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parsePrintStatement()
{
    // PRINT 는 이미 소비된 상태로 진입. 문법: print expr ;
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "';' 가 필요합니다.");

    auto stmt        = std::make_unique<PrintStmt>();
    stmt->expression = std::move(expr);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseIfStatement()
{
    // IF 는 이미 소비된 상태로 진입. 문법: if ( expr ) stmt ( else stmt )?
    consume(TokenType::LEFT_PAREN, "'(' 가 필요합니다.");
    auto condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "')' 가 필요합니다.");

    auto thenBranch = parseStatement();

    // else 는 발견 즉시 소비한다 — 가장 가까운 if 에 결합 (dangling-else 해소)
    std::unique_ptr<Stmt> elseBranch;
    if (match({ TokenType::ELSE }))
        elseBranch = parseStatement();

    auto stmt        = std::make_unique<IfStmt>();
    stmt->condition  = std::move(condition);
    stmt->thenBranch = std::move(thenBranch);
    stmt->elseBranch = std::move(elseBranch);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseForStatement()
{
    // FOR 는 이미 소비된 상태로 진입. 문법: for ( init cond ; incr ) stmt
    // init 은 VarDeclareStmt 또는 ExpressionStmt — 각 함수가 세미콜론까지 소비한다.
    consume(TokenType::LEFT_PAREN, "'(' 가 필요합니다.");

    std::unique_ptr<Stmt> init;
    if (match({ TokenType::VAR }))
        init = parseVarDeclaration();       // var i = 0;  (세미콜론 포함)
    else
        init = parseExpressionStatement();  // i = 0;      (세미콜론 포함)

    auto condition = parseExpression();
    consume(TokenType::SEMICOLON, "';' 가 필요합니다.");

    auto increment = parseExpression();
    consume(TokenType::RIGHT_PAREN, "')' 가 필요합니다.");

    auto body = parseStatement();

    auto stmt       = std::make_unique<ForStmt>();
    stmt->init      = std::move(init);
    stmt->condition = std::move(condition);
    stmt->increment = std::move(increment);
    stmt->body      = std::move(body);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseBlock()
{
    // LEFT_BRACE 는 이미 소비된 상태로 진입.
    // isAtEnd() 검사는 닫는 중괄호 없이 EOF 에 도달했을 때 무한루프를 방지한다.
    // 실제 오류는 consume(RIGHT_BRACE) 에서 던진다.
    auto stmt = std::make_unique<BlockStmt>();
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        stmt->statements.push_back(parseDeclaration());
    consume(TokenType::RIGHT_BRACE, "'}' 가 필요합니다.");
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseExpressionStatement()
{
    // Expression 을 파싱한 뒤 반드시 세미콜론(;) 으로 닫혀야 한다.
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "';' 가 필요합니다.");

    auto stmt        = std::make_unique<ExpressionStmt>();
    stmt->expression = std::move(expr);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseFunctionDeclaration()
{
    // FUNC 는 이미 소비된 상태로 진입. 문법: func name(p1, p2) { body }
    Token name = consume(TokenType::IDENTIFIER, "함수 이름이 필요합니다.");
    consume(TokenType::LEFT_PAREN, "'(' 가 필요합니다.");

    std::vector<Token> params;
    if (!check(TokenType::RIGHT_PAREN))
    {
        do { params.push_back(consume(TokenType::IDENTIFIER, "매개변수 이름이 필요합니다.")); }
        while (match({ TokenType::COMMA }));
    }
    consume(TokenType::RIGHT_PAREN, "')' 가 필요합니다.");
    consume(TokenType::LEFT_BRACE,  "'{' 가 필요합니다.");

    auto body         = parseBlock(); // LEFT_BRACE 이미 소비된 상태로 호출
    auto stmt         = std::make_unique<FunctionDeclareStmt>();
    stmt->name        = name;
    stmt->params      = std::move(params);
    stmt->body        = std::move(body);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseReturnStatement()
{
    // RETURN 은 이미 소비된 상태로 진입. 문법: return expr ;
    auto value = parseExpression();
    consume(TokenType::SEMICOLON, "';' 가 필요합니다.");

    auto stmt   = std::make_unique<ReturnStmt>();
    stmt->value = std::move(value);
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
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix()
{
    // 배열 접근: expr[index] (좌결합, 중첩 가능)
    auto expr = parsePrimary();
    while (match({ TokenType::LEFT_BRACKET }))
    {
        auto index  = parseExpression();
        consume(TokenType::RIGHT_BRACKET, "']' 가 필요합니다.");
        auto access  = std::make_unique<ArrayAccessExpr>();
        access->array = std::move(expr);
        access->index = std::move(index);
        expr = std::move(access);
    }
    return expr;
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
        Token name = previous();
        if (match({ TokenType::LEFT_PAREN }))
        {
            // 함수 호출: name(arg1, arg2, ...)
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RIGHT_PAREN))
            {
                do { args.push_back(parseExpression()); }
                while (match({ TokenType::COMMA }));
            }
            consume(TokenType::RIGHT_PAREN, "')' 가 필요합니다.");
            auto call    = std::make_unique<FunctionCallExpr>();
            call->callee = name;
            call->args   = std::move(args);
            return call;
        }
        auto var  = std::make_unique<VariableExpr>();
        var->name = name;
        return var;
    }

    if (match({ TokenType::LEFT_BRACKET }))
    {
        // 배열 리터럴: [elem1, elem2, ...]
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RIGHT_BRACKET))
        {
            do { elements.push_back(parseExpression()); }
            while (match({ TokenType::COMMA }));
        }
        consume(TokenType::RIGHT_BRACKET, "']' 가 필요합니다.");
        auto arr      = std::make_unique<ArrayLiteralExpr>();
        arr->elements = std::move(elements);
        return arr;
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
    if (!isAtEnd()) m_current++;
    return previous();
}

Token Parser::peek() const
{
    return m_tokens[m_current];
}

Token Parser::previous() const
{
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type, const std::string& msg)
{
    if (check(type)) return advance();
    throw std::runtime_error("[" + std::to_string(peek().line) + "번째 줄] " + msg);
}
