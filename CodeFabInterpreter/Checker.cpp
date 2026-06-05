#include "Checker.h"
#include <unordered_set>
#include <unordered_map>

using Scope   = std::unordered_set<std::string>;
using FuncMap = std::unordered_map<std::string, size_t>; // 함수명 → 파라미터 수

// -----------------------------------------------------------------------
// 전방 선언
// -----------------------------------------------------------------------
static void foldExpr(std::unique_ptr<Expr>& exprRef);

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
// isPure — 사이드 이펙트가 없는 표현식인지 판별한다.
// LiteralExpr / VariableExpr 만 순수하다고 본다.
// 함수 호출(FunctionCallExpr), 대입(AssignExpr) 등은 false.
static bool isPure(const Expr* e)
{
    return dynamic_cast<const LiteralExpr*>(e) || dynamic_cast<const VariableExpr*>(e);
}

// -----------------------------------------------------------------------
// foldExpr — 상수 폴딩: 컴파일 타임에 계산 가능한 노드를 LiteralExpr 로 교체한다.
//   패턴 A : BinaryExpr(Lit, op, Lit) → Lit
//   패턴 B : UnaryExpr(op, Lit)       → Lit
//   패턴 C : GroupingExpr(Lit)        → Lit  (괄호 껍데기 제거)
//   패턴 D : 대수 항등식 (x±0, x*1, 1*x, x/1, x*0, 0*x, 0+x, x+0)
// checkExpr 로 distance 기록을 완료한 뒤 호출해야 한다.
// -----------------------------------------------------------------------
static void foldExpr(std::unique_ptr<Expr>& exprRef)
{
    if (!exprRef) return;

    if (auto* e = dynamic_cast<BinaryExpr*>(exprRef.get()))
    {
        foldExpr(e->left);
        foldExpr(e->right);

        auto* litL = dynamic_cast<LiteralExpr*>(e->left.get());
        auto* litR = dynamic_cast<LiteralExpr*>(e->right.get());

        // 패턴 A: 양쪽 모두 숫자 리터럴 → 연산 결과 리터럴로 교체
        if (litL && litR &&
            std::holds_alternative<double>(litL->value) &&
            std::holds_alternative<double>(litR->value))
        {
            double l = std::get<double>(litL->value);
            double r = std::get<double>(litR->value);
            bool foldable = false;
            Value result;
            switch (e->op.type)
            {
                case TokenType::PLUS:    result = Value{l + r};  foldable = true; break;
                case TokenType::MINUS:   result = Value{l - r};  foldable = true; break;
                case TokenType::STAR:    result = Value{l * r};  foldable = true; break;
                case TokenType::SLASH:   if (r != 0.0) { result = Value{l / r}; foldable = true; } break;
                case TokenType::LESS:    result = Value{l < r};  foldable = true; break;
                case TokenType::GREATER: result = Value{l > r};  foldable = true; break;
                default: break;
            }
            if (foldable)
            {
                auto lit = std::make_unique<LiteralExpr>();
                lit->value = result;
                exprRef = std::move(lit);
                return;
            }
        }

        // 패턴 D: 대수 항등식 — 한쪽이 숫자 리터럴인 경우만 처리
        if (litR && std::holds_alternative<double>(litR->value))
        {
            double r   = std::get<double>(litR->value);
            TokenType op = e->op.type;
            if ((op == TokenType::PLUS || op == TokenType::MINUS) && r == 0.0)
            { exprRef = std::move(e->left); return; }
            if ((op == TokenType::STAR || op == TokenType::SLASH) && r == 1.0)
            { exprRef = std::move(e->left); return; }
            if (op == TokenType::STAR && r == 0.0 && isPure(e->left.get()))
            { auto z = std::make_unique<LiteralExpr>(); z->value = 0.0; exprRef = std::move(z); return; }
        }
        if (litL && std::holds_alternative<double>(litL->value))
        {
            double l   = std::get<double>(litL->value);
            TokenType op = e->op.type;
            if (op == TokenType::PLUS && l == 0.0)
            { exprRef = std::move(e->right); return; }
            if (op == TokenType::STAR && l == 1.0)
            { exprRef = std::move(e->right); return; }
            if (op == TokenType::STAR && l == 0.0 && isPure(e->right.get()))
            { auto z = std::make_unique<LiteralExpr>(); z->value = 0.0; exprRef = std::move(z); return; }
        }
        return;
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(exprRef.get()))
    {
        foldExpr(e->right);
        auto* lit = dynamic_cast<LiteralExpr*>(e->right.get());
        if (!lit) return;

        // 패턴 B: -숫자
        if (e->op.type == TokenType::MINUS && std::holds_alternative<double>(lit->value))
        {
            auto folded = std::make_unique<LiteralExpr>();
            folded->value = Value{-std::get<double>(lit->value)};
            exprRef = std::move(folded);
        }
        // 패턴 B: !bool
        else if (e->op.type == TokenType::BANG && std::holds_alternative<bool>(lit->value))
        {
            auto folded = std::make_unique<LiteralExpr>();
            folded->value = Value{!std::get<bool>(lit->value)};
            exprRef = std::move(folded);
        }
        return;
    }

    if (auto* e = dynamic_cast<GroupingExpr*>(exprRef.get()))
    {
        foldExpr(e->expression);
        // 패턴 C: (리터럴) → 리터럴
        if (dynamic_cast<LiteralExpr*>(e->expression.get()))
            exprRef = std::move(e->expression);
        return;
    }

    if (auto* e = dynamic_cast<AssignExpr*>(exprRef.get()))
    {
        foldExpr(e->value);
        return;
    }

    if (auto* e = dynamic_cast<ArrayLiteralExpr*>(exprRef.get()))
    {
        for (auto& elem : e->elements) foldExpr(elem);
        return;
    }

    if (auto* e = dynamic_cast<ArrayAccessExpr*>(exprRef.get()))
    {
        foldExpr(e->array);
        foldExpr(e->index);
        return;
    }

    if (auto* e = dynamic_cast<ArrayWriteExpr*>(exprRef.get()))
    {
        foldExpr(e->array);
        foldExpr(e->index);
        foldExpr(e->value);
        return;
    }

    if (auto* e = dynamic_cast<FunctionCallExpr*>(exprRef.get()))
    {
        for (auto& arg : e->args) foldExpr(arg);
        return;
    }
    // LiteralExpr / VariableExpr: 리프 노드, 처리 불필요
}

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
    else if (auto* e = dynamic_cast<ArrayLiteralExpr*>(expr))
    {
        for (const auto& elem : e->elements)
            checkExpr(elem.get(), declaringVarName, errors, selfRefFound, funcs, scopes);
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
        foldExpr(s->initializer);

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
            foldExpr(s->value);
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
        foldExpr(s->condition);
        checkStmt(s->thenBranch.get(), scopes, errors, funcs, insideFunction);
        checkStmt(s->elseBranch.get(), scopes, errors, funcs, insideFunction);
    }
    else if (auto* s = dynamic_cast<ForStmt*>(stmt))
    {
        bool unused = false;
        scopes.push_back({});
        checkStmt(s->init.get(), scopes, errors, funcs, insideFunction);
        checkExpr(s->condition.get(), "", errors, unused, funcs, scopes);
        foldExpr(s->condition);
        checkExpr(s->increment.get(), "", errors, unused, funcs, scopes);
        foldExpr(s->increment);
        checkStmt(s->body.get(), scopes, errors, funcs, insideFunction);
        scopes.pop_back();
    }
    else if (auto* s = dynamic_cast<PrintStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(s->expression.get(), "", errors, unused, funcs, scopes);
        foldExpr(s->expression);
    }
    else if (auto* s = dynamic_cast<ExpressionStmt*>(stmt))
    {
        bool unused = false;
        checkExpr(s->expression.get(), "", errors, unused, funcs, scopes);
        foldExpr(s->expression);
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
