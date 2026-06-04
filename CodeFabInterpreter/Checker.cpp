#include "Checker.h"
#include <unordered_set>

using Scope = std::unordered_set<std::string>;

static void checkExpr(const Expr* expr,
                      const std::string& declaringVarName,
                      std::vector<std::string>& errors)
{
    if (!expr) return;

    if (const auto* varExpr = dynamic_cast<const VariableExpr*>(expr))
    {
        if (!declaringVarName.empty() && varExpr->name.lexeme == declaringVarName)
            errors.push_back("[" + std::to_string(varExpr->name.line) + "번째 줄] "
                             "변수 '" + declaringVarName + "' 가 자기 자신을 초기화에 참조합니다.");
    }
    else if (const auto* binExpr = dynamic_cast<const BinaryExpr*>(expr))
    {
        checkExpr(binExpr->left.get(),  declaringVarName, errors);
        checkExpr(binExpr->right.get(), declaringVarName, errors);
    }
    else if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr))
    {
        checkExpr(unaryExpr->right.get(), declaringVarName, errors);
    }
    else if (const auto* groupExpr = dynamic_cast<const GroupingExpr*>(expr))
    {
        checkExpr(groupExpr->expression.get(), declaringVarName, errors);
    }
    else if (const auto* assignExpr = dynamic_cast<const AssignExpr*>(expr))
    {
        checkExpr(assignExpr->value.get(), declaringVarName, errors);
    }
}

static void checkStmt(const Stmt* stmt,
                      std::vector<Scope>& scopes,
                      std::vector<std::string>& errors)
{
    if (!stmt) return;

    if (const auto* varDecl = dynamic_cast<const VarDeclareStmt*>(stmt))
    {
        // initializer를 먼저 검사 (선언 전이므로 자기 참조 감지)
        // 같은 변수가 initializer에 여러 번 등장해도 에러는 1개만 유지
        const size_t errorsBefore = errors.size();
        checkExpr(varDecl->initializer.get(), varDecl->name.lexeme, errors);
        if (errors.size() > errorsBefore + 1)
            errors.resize(errorsBefore + 1);

        // 현재 스코프에서 중복 선언 검사
        if (scopes.back().count(varDecl->name.lexeme))
            errors.push_back("[" + std::to_string(varDecl->name.line) + "번째 줄] "
                             "변수 '" + varDecl->name.lexeme + "' 가 이미 선언되어 있습니다.");
        else
            scopes.back().insert(varDecl->name.lexeme);
    }
    else if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt))
    {
        scopes.push_back({});
        for (const auto& s : blockStmt->statements)
            checkStmt(s.get(), scopes, errors);
        scopes.pop_back();
    }
    else if (const auto* ifStmt = dynamic_cast<const IfStmt*>(stmt))
    {
        checkExpr(ifStmt->condition.get(), "", errors);
        checkStmt(ifStmt->thenBranch.get(), scopes, errors);
        checkStmt(ifStmt->elseBranch.get(), scopes, errors);
    }
    else if (const auto* forStmt = dynamic_cast<const ForStmt*>(stmt))
    {
        scopes.push_back({});
        checkStmt(forStmt->init.get(), scopes, errors);
        checkExpr(forStmt->condition.get(), "", errors);
        checkExpr(forStmt->increment.get(), "", errors);
        checkStmt(forStmt->body.get(), scopes, errors);
        scopes.pop_back();
    }
    else if (const auto* printStmt = dynamic_cast<const PrintStmt*>(stmt))
    {
        checkExpr(printStmt->expression.get(), "", errors);
    }
    else if (const auto* exprStmt = dynamic_cast<const ExpressionStmt*>(stmt))
    {
        checkExpr(exprStmt->expression.get(), "", errors);
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
