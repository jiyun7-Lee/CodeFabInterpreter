#include "Checker.h"
#include <unordered_set>

using Scope = std::unordered_set<std::string>;

static void checkExpr(const Expr* expr,
                      const std::string& declaringVarName,
                      std::vector<std::string>& errors,
                      bool& selfRefFound)
{
    if (!expr) return;

    if (const auto* varExpr = dynamic_cast<const VariableExpr*>(expr))
    {
        if (!declaringVarName.empty() && varExpr->name.lexeme == declaringVarName && !selfRefFound)
        {
            selfRefFound = true;
            errors.push_back("[" + std::to_string(varExpr->name.line) + "번째 줄] "
                             "변수 '" + declaringVarName + "' 가 자기 자신을 초기화에 참조합니다.");
        }
    }
    else if (const auto* binExpr = dynamic_cast<const BinaryExpr*>(expr))
    {
        checkExpr(binExpr->left.get(),  declaringVarName, errors, selfRefFound);
        checkExpr(binExpr->right.get(), declaringVarName, errors, selfRefFound);
    }
    else if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr))
    {
        checkExpr(unaryExpr->right.get(), declaringVarName, errors, selfRefFound);
    }
    else if (const auto* groupExpr = dynamic_cast<const GroupingExpr*>(expr))
    {
        checkExpr(groupExpr->expression.get(), declaringVarName, errors, selfRefFound);
    }
    else if (const auto* assignExpr = dynamic_cast<const AssignExpr*>(expr))
    {
        checkExpr(assignExpr->value.get(), declaringVarName, errors, selfRefFound);
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
        // selfRefFound 플래그로 같은 변수 자기참조 에러를 1개만 추가
        bool selfRefFound = false;
        checkExpr(varDecl->initializer.get(), varDecl->name.lexeme, errors, selfRefFound);

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
        bool unused = false;
        checkExpr(ifStmt->condition.get(), "", errors, unused);
        checkStmt(ifStmt->thenBranch.get(), scopes, errors);
        checkStmt(ifStmt->elseBranch.get(), scopes, errors);
    }
    else if (const auto* forStmt = dynamic_cast<const ForStmt*>(stmt))
    {
        bool unused = false;
        scopes.push_back({});
        checkStmt(forStmt->init.get(), scopes, errors);
        checkExpr(forStmt->condition.get(), "", errors, unused);
        checkExpr(forStmt->increment.get(), "", errors, unused);
        checkStmt(forStmt->body.get(), scopes, errors);
        scopes.pop_back();
    }
    else if (const auto* printStmt = dynamic_cast<const PrintStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(printStmt->expression.get(), "", errors, unused);
    }
    else if (const auto* exprStmt = dynamic_cast<const ExpressionStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(exprStmt->expression.get(), "", errors, unused);
    }
}

bool Checker::check(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    errors_.clear();

    for (const auto& s : statements)
        checkStmt(s.get(), scopes_, errors_);

    return errors_.empty();
}

const std::vector<std::string>& Checker::getErrors() const
{
    return errors_;
}
