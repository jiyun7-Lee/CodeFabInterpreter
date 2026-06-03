#include "Parser.h"
#include "Expr.h"

// -----------------------------------------------------------------------
// [Recursive Descent Parser]
//
// Each grammar rule is a function. Lower-precedence rules call
// higher-precedence rules, forming a call hierarchy that naturally
// encodes operator precedence.
//
// Call chain (top = lowest precedence, bottom = highest):
//
//   parseExpression
//     parseAssignment   (=)
//       parseOr         (or)
//         parseAnd      (and)
//           parseComparison  (> <)
//             parseTerm      (+ -)
//               parseFactor  (* /)
//                 parseUnary   (unary -)
//                   parsePrimary  (literals, variables, grouping)
//
// Example: "1 + 2 * 3"
//   parseTerm calls parseFactor twice.
//   The second parseFactor consumes "2 * 3" as a subtree,
//   so the tree root becomes '+' with '*' as its right child.
// -----------------------------------------------------------------------

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
    // Only ExpressionStmt is handled here.
    // PrintStmt, VarDeclareStmt, IfStmt, ForStmt -> Stmt Parser (separate owner).
    return parseExpressionStatement();
}

Stmt* Parser::parseExpressionStatement()
{
    // Parse an expression and require a trailing semicolon.
    // ExpressionStmt is a wrapper that promotes an Expr to a Stmt.
    Expr* expr = parseExpression();
    consume(TokenType::SEMICOLON);

    auto* stmt = new ExpressionStmt();
    stmt->expression = expr;
    return stmt;
}

// -----------------------------------------------------------------------
// Expression -- ordered from lowest to highest precedence
// -----------------------------------------------------------------------

Expr* Parser::parseExpression()
{
    // Entry point for all expression parsing.
    // Delegates immediately to the lowest-precedence rule.
    return parseAssignment();
}

Expr* Parser::parseAssignment()
{
    // Parse the right-hand side first via parseOr, then check for '='.
    //
    // [Right-associative]
    // The RHS recurses back into parseAssignment so that
    // "a = b = 3" parses as "a = (b = 3)".
    Expr* expr = parseOr();

    if (match({ TokenType::EQUAL }))
    {
        Expr* value = parseAssignment(); // right-recursive for right-associativity

        // LHS of assignment must be a variable.
        // e.g. "3 = 5" is invalid -- further checked in Checker unit.
        if (auto* var = dynamic_cast<VariableExpr*>(expr))
        {
            auto* assign  = new AssignExpr();
            assign->name  = var->name;
            assign->value = value;
            delete expr; // VariableExpr absorbed into AssignExpr
            return assign;
        }
    }
    return expr;
}

Expr* Parser::parseOr()
{
    // Left-associative: "a or b or c" -> BinaryExpr(or, BinaryExpr(or, a, b), c)
    Expr* expr = parseAnd();
    while (match({ TokenType::OR }))
    {
        Token op = previous(); // capture operator before recursive call
        expr = makeBinary(expr, op, parseAnd());
    }
    return expr;
}

Expr* Parser::parseAnd()
{
    Expr* expr = parseComparison();
    while (match({ TokenType::AND }))
    {
        Token op = previous();
        expr = makeBinary(expr, op, parseComparison());
    }
    return expr;
}

Expr* Parser::parseComparison()
{
    // Handles >, < operators.
    // e.g. "a > b" -> BinaryExpr(>, VariableExpr(a), VariableExpr(b))
    Expr* expr = parseTerm();
    while (match({ TokenType::GREATER, TokenType::LESS }))
    {
        Token op = previous();
        expr = makeBinary(expr, op, parseTerm());
    }
    return expr;
}

Expr* Parser::parseTerm()
{
    // Handles +, - operators.
    // parseFactor is called first, so * / bind tighter than + -.
    Expr* expr = parseFactor();
    while (match({ TokenType::PLUS, TokenType::MINUS }))
    {
        Token op = previous();
        expr = makeBinary(expr, op, parseFactor());
    }
    return expr;
}

Expr* Parser::parseFactor()
{
    // Handles *, / operators.
    // parseUnary is called first, so unary minus binds tighter than * /.
    Expr* expr = parseUnary();
    while (match({ TokenType::STAR, TokenType::SLASH }))
    {
        Token op = previous();
        expr = makeBinary(expr, op, parseUnary());
    }
    return expr;
}

Expr* Parser::parseUnary()
{
    // Handles unary minus.
    // Recursive so that "--a" -> UnaryExpr(-, UnaryExpr(-, a)).
    if (match({ TokenType::MINUS }))
    {
        Token op    = previous();
        Expr* right = parseUnary();
        auto* unary = new UnaryExpr();
        unary->op    = op;
        unary->right = right;
        return unary;
    }
    return parsePrimary();
}

Expr* Parser::parsePrimary()
{
    // Parses the smallest indivisible unit (atom).

    // Number and string literals: value is stored in the token's literal field.
    if (match({ TokenType::NUMBER, TokenType::STRING }))
    {
        auto* lit  = new LiteralExpr();
        lit->value = previous().literal;
        return lit;
    }

    // Boolean literals
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

    // Identifier (variable name): lexeme stored as-is; Executor resolves the value at runtime.
    if (match({ TokenType::IDENTIFIER }))
    {
        auto* var = new VariableExpr();
        var->name = previous();
        return var;
    }

    // Grouped expression: wraps inner expr in GroupingExpr to override precedence.
    // e.g. "(1 + 2) * 3" -- parseFactor treats GroupingExpr as a single atom.
    if (match({ TokenType::LEFT_PAREN }))
    {
        Expr* expr = parseExpression(); // re-enter at the top of the precedence chain
        consume(TokenType::RIGHT_PAREN);
        auto* grp       = new GroupingExpr();
        grp->expression = expr;
        return grp;
    }

    return nullptr; // no matching token -- error handling to be added later
}

// -----------------------------------------------------------------------
// Node factory helper
// -----------------------------------------------------------------------

BinaryExpr* Parser::makeBinary(Expr* left, Token op, Expr* right)
{
    // Shared BinaryExpr construction used by parseOr/And/Comparison/Term/Factor.
    auto* bin  = new BinaryExpr();
    bin->left  = left;
    bin->op    = op;
    bin->right = right;
    return bin;
}

// -----------------------------------------------------------------------
// Token helpers
// -----------------------------------------------------------------------

bool Parser::match(std::initializer_list<TokenType> types)
{
    // If the current token matches any of the given types, consume it and return true.
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
    // Peek at current token type without consuming.
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd()
{
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::advance()
{
    // Consume the current token and return it.
    if (!isAtEnd()) m_current++;
    return previous();
}

Token Parser::peek()
{
    // Return current token without consuming.
    return m_tokens[m_current];
}

Token Parser::previous()
{
    // Return the most recently consumed token.
    // Typically called right after match() to retrieve the matched operator.
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type)
{
    // Consume and return the current token if it matches the expected type.
    // Silent no-op on mismatch -- error handling to be added later.
    if (check(type)) return advance();
    return peek();
}
