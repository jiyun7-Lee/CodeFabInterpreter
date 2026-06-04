#include "Checker.h"
#include <unordered_set>

using Scope = std::unordered_set<std::string>;

static void checkExpr(const Expr* expr,
                      const std::string& forbiddenName,
                      std::vector<std::string>& errors)
{
    if (!expr) return;

    if (const auto* v = dynamic_cast<const VariableExpr*>(expr))
    {
        if (!forbiddenName.empty() && v->name.lexeme == forbiddenName)
            errors.push_back("[" + std::to_string(v->name.line) + "번째 줄] "
                             "변수 '" + forbiddenName + "' 가 자기 자신을 초기화에 참조합니다.");
    }
    else if (const auto* b = dynamic_cast<const BinaryExpr*>(expr))
    {
        checkExpr(b->left.get(),  forbiddenName, errors);
        checkExpr(b->right.get(), forbiddenName, errors);
    }
    else if (const auto* u = dynamic_cast<const UnaryExpr*>(expr))
    {
        checkExpr(u->right.get(), forbiddenName, errors);
    }
    else if (const auto* g = dynamic_cast<const GroupingExpr*>(expr))
    {
        checkExpr(g->expression.get(), forbiddenName, errors);
    }
    else if (const auto* a = dynamic_cast<const AssignExpr*>(expr))
    {
        checkExpr(a->value.get(), forbiddenName, errors);
    }
}

static void checkStmt(const Stmt* stmt,
                      std::vector<Scope>& scopes,
                      std::vector<std::string>& errors)
{
    if (!stmt) return;

    if (const auto* v = dynamic_cast<const VarDeclareStmt*>(stmt))
    {
        // initializer를 먼저 검사 (선언 전이므로 자기 참조 감지)
        checkExpr(v->initializer.get(), v->name.lexeme, errors);

        // 현재 스코프에서 중복 선언 검사
        if (scopes.back().count(v->name.lexeme))
            errors.push_back("[" + std::to_string(v->name.line) + "번째 줄] "
                             "변수 '" + v->name.lexeme + "' 가 이미 선언되어 있습니다.");
        else
            scopes.back().insert(v->name.lexeme);
    }
    else if (const auto* bl = dynamic_cast<const BlockStmt*>(stmt))
    {
        scopes.push_back({});
        for (const auto& s : bl->statements)
            checkStmt(s.get(), scopes, errors);
        scopes.pop_back();
    }
    else if (const auto* i = dynamic_cast<const IfStmt*>(stmt))
    {
        checkExpr(i->condition.get(), "", errors);
        checkStmt(i->thenBranch.get(), scopes, errors);
        checkStmt(i->elseBranch.get(), scopes, errors);
    }
    else if (const auto* f = dynamic_cast<const ForStmt*>(stmt))
    {
        scopes.push_back({});
        checkStmt(f->init.get(), scopes, errors);
        checkExpr(f->condition.get(), "", errors);
        checkExpr(f->increment.get(), "", errors);
        checkStmt(f->body.get(), scopes, errors);
        scopes.pop_back();
    }
    else if (const auto* p = dynamic_cast<const PrintStmt*>(stmt))
    {
        checkExpr(p->expression.get(), "", errors);
    }
    else if (const auto* e = dynamic_cast<const ExpressionStmt*>(stmt))
    {
        checkExpr(e->expression.get(), "", errors);
    }
}

bool Checker::check(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    errors_.clear();
    std::vector<Scope> scopes;
    scopes.push_back({}); // global scope

    for (const auto& s : statements)
        checkStmt(s.get(), scopes, errors_);

    return errors_.empty();
}

const std::vector<std::string>& Checker::getErrors() const
{
    return errors_;
}
