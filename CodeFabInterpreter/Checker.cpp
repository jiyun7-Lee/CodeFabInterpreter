#include "Checker.h"
#include <unordered_set>
#include <unordered_map>

using Scope   = std::unordered_set<std::string>;
using FuncMap = std::unordered_map<std::string, size_t>; // 함수명 → 파라미터 수

// -----------------------------------------------------------------------
// 전방 선언
// -----------------------------------------------------------------------
static void checkExpr(Expr* expr,
                      const std::string& declaringVarName,
                      std::vector<std::string>& errors,
                      bool& selfRefFound,
                      const FuncMap& funcs,
                      const std::vector<Scope>& scopes);

static void checkStmt(Stmt* stmt,
                      std::vector<Scope>& scopes,
                      std::vector<std::string>& errors,
                      FuncMap& funcs,
                      bool insideFunction);

// -----------------------------------------------------------------------
// checkExpr — non-const: VariableExpr/AssignExpr 에 distance 기록
// -----------------------------------------------------------------------
static void checkExpr(Expr* expr,
                      const std::string& declaringVarName,
                      std::vector<std::string>& errors,
                      bool& selfRefFound,
                      const FuncMap& funcs,
                      const std::vector<Scope>& scopes)
{
    if (!expr) return;

    if (auto* e = dynamic_cast<VariableExpr*>(expr))
    {
        if (!declaringVarName.empty() && e->name.lexeme == declaringVarName && !selfRefFound)
        {
            selfRefFound = true;
            errors.push_back("[" + std::to_string(e->name.line) + "번째 줄] "
                             "변수 '" + declaringVarName + "' 가 자기 자신을 초기화에 참조합니다.");
        }
        // 정적 바인딩: 가장 안쪽 스코프부터 바깥으로 탐색해 distance 기록
        for (int i = (int)scopes.size() - 1; i >= 0; i--)
        {
            if (scopes[i].count(e->name.lexeme))
            {
                e->distance = (int)scopes.size() - 1 - i;
                break;
            }
        }
    }
    else if (auto* e = dynamic_cast<BinaryExpr*>(expr))
    {
        checkExpr(e->left.get(),  declaringVarName, errors, selfRefFound, funcs, scopes);
        checkExpr(e->right.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
    }
    else if (auto* e = dynamic_cast<UnaryExpr*>(expr))
    {
        checkExpr(e->right.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
    }
    else if (auto* e = dynamic_cast<GroupingExpr*>(expr))
    {
        checkExpr(e->expression.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
    }
    else if (auto* e = dynamic_cast<AssignExpr*>(expr))
    {
        checkExpr(e->value.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
        // 정적 바인딩: 대입 대상 변수의 distance 기록
        for (int i = (int)scopes.size() - 1; i >= 0; i--)
        {
            if (scopes[i].count(e->name.lexeme))
            {
                e->distance = (int)scopes.size() - 1 - i;
                break;
            }
        }
    }
    else if (auto* e = dynamic_cast<ArrayAccessExpr*>(expr))
    {
        checkExpr(e->array.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
        checkExpr(e->index.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
    }
    else if (auto* e = dynamic_cast<ArrayWriteExpr*>(expr))
    {
        // arr[i] = val — ArrayAccessExpr(읽기)와 구분되는 쓰기 전용 노드.
        // 동일한 arr[i] 문법이 대입 좌변이면 ArrayWriteExpr, 우변이면 ArrayAccessExpr 로 파싱된다.
        checkExpr(e->array.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
        checkExpr(e->index.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
        checkExpr(e->value.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
    }
    else if (auto* e = dynamic_cast<FunctionCallExpr*>(expr))
    {
        const std::string& callee = e->callee.lexeme;
        int line = e->callee.line;

        bool isFunc = funcs.count(callee) > 0;

        // 함수가 아닌 대상 호출: callee 가 변수로 선언됐지만 함수가 아닌 경우
        bool isDeclaredVar = false;
        for (const auto& scope : scopes)
            if (scope.count(callee)) { isDeclaredVar = true; break; }

        if (!isFunc && isDeclaredVar)
            errors.push_back("[" + std::to_string(line) + "번째 줄] "
                             "'" + callee + "' 는 함수가 아닙니다.");

        // isFunc=false, isDeclaredVar=false 인 경우(아예 선언되지 않은 이름 호출)는
        // 의도적으로 Checker 에서 잡지 않는다. Executor 가 런타임에
        // "Undefined function '...'" 오류를 발생시킨다.

        // 인자 개수 불일치
        if (isFunc && e->args.size() != funcs.at(callee))
            errors.push_back("[" + std::to_string(line) + "번째 줄] "
                             "함수 '" + callee + "': 인자 개수 불일치 "
                             "(기대 " + std::to_string(funcs.at(callee)) + "개, "
                             "전달 " + std::to_string(e->args.size()) + "개).");

        // 인자 각각 체크
        for (const auto& arg : e->args)
        {
            bool unused = false;
            checkExpr(arg.get(), "", errors, unused, funcs, scopes);
        }
    }
}

// -----------------------------------------------------------------------
// checkStmt — non-const: 자식 expr 에 non-const Expr* 전달 가능
// -----------------------------------------------------------------------
static void checkStmt(Stmt* stmt,
                      std::vector<Scope>& scopes,
                      std::vector<std::string>& errors,
                      FuncMap& funcs,
                      bool insideFunction)
{
    if (!stmt) return;

    if (auto* s = dynamic_cast<VarDeclareStmt*>(stmt))
    {
        bool selfRefFound = false;
        checkExpr(s->initializer.get(), s->name.lexeme, errors, selfRefFound, funcs, scopes);

        if (scopes.back().count(s->name.lexeme))
            errors.push_back("[" + std::to_string(s->name.line) + "번째 줄] "
                             "변수 '" + s->name.lexeme + "' 가 이미 선언되어 있습니다.");
        else
            scopes.back().insert(s->name.lexeme);
    }
    else if (auto* s = dynamic_cast<FunctionDeclareStmt*>(stmt))
    {
        // 파라미터 이름 중복 검사
        std::unordered_set<std::string> paramSet;
        for (const auto& p : s->params)
        {
            if (paramSet.count(p.lexeme))
                errors.push_back("[" + std::to_string(p.line) + "번째 줄] "
                                 "파라미터 '" + p.lexeme + "' 이름이 중복됩니다.");
            paramSet.insert(p.lexeme);
        }

        // 함수 등록 (이름 → 파라미터 수)
        funcs[s->name.lexeme] = s->params.size();

        // 함수 본문 검사 — 파라미터를 스코프에 등록, insideFunction = true
        scopes.push_back(paramSet);
        checkStmt(s->body.get(), scopes, errors, funcs, true);
        scopes.pop_back();
    }
    else if (auto* s = dynamic_cast<ReturnStmt*>(stmt))
    {
        // 함수 외부 return 검사
        if (!insideFunction)
            errors.push_back("함수 외부에서 return 을 사용할 수 없습니다.");

        if (s->value)
        {
            bool unused = false;
            checkExpr(s->value.get(), "", errors, unused, funcs, scopes);
        }
    }
    else if (auto* s = dynamic_cast<BlockStmt*>(stmt))
    {
        scopes.push_back({});
        for (const auto& st : s->statements)
            checkStmt(st.get(), scopes, errors, funcs, insideFunction);
        scopes.pop_back();
    }
    else if (auto* s = dynamic_cast<IfStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(s->condition.get(), "", errors, unused, funcs, scopes);
        checkStmt(s->thenBranch.get(), scopes, errors, funcs, insideFunction);
        checkStmt(s->elseBranch.get(), scopes, errors, funcs, insideFunction);
    }
    else if (auto* s = dynamic_cast<ForStmt*>(stmt))
    {
        bool unused = false;
        scopes.push_back({});
        checkStmt(s->init.get(), scopes, errors, funcs, insideFunction);
        checkExpr(s->condition.get(), "", errors, unused, funcs, scopes);
        checkExpr(s->increment.get(), "", errors, unused, funcs, scopes);
        checkStmt(s->body.get(), scopes, errors, funcs, insideFunction);
        scopes.pop_back();
    }
    else if (auto* s = dynamic_cast<PrintStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(s->expression.get(), "", errors, unused, funcs, scopes);
    }
    else if (auto* s = dynamic_cast<ExpressionStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(s->expression.get(), "", errors, unused, funcs, scopes);
    }
}

// -----------------------------------------------------------------------
// public — check() 시그니처는 const& 유지
// const unique_ptr<Stmt>::get() 은 Stmt* 를 반환하므로 non-const checkStmt 호출 가능
// -----------------------------------------------------------------------
bool Checker::check(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    errors_.clear();
    // scopes_·funcs_ 는 초기화하지 않음 — REPL 세션 전반의 선언 정보를 누적하기 위해 유지
    // 스코프를 초기화해야 하는 경우 reset() 을 먼저 호출할 것

    for (const auto& s : statements)
        checkStmt(s.get(), scopes_, errors_, funcs_, false);

    return errors_.empty();
}

void Checker::reset()
{
    errors_.clear();
    scopes_ = {{}};
    funcs_.clear();
}

const std::vector<std::string>& Checker::getErrors() const
{
    return errors_;
}
