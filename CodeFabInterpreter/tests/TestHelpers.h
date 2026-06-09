#pragma once
#include <gtest/gtest.h>
#include <stdexcept>
#include <memory>
#include <vector>
#include <variant>
#include "../Token.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../Executor.h"
#include "../Expr.h"
#include "../Stmt.h"
#include "../Value.h"
#include "../DebugController.h"

// -----------------------------------------------------------------------
// Token 생성 헬퍼 (기존)
// -----------------------------------------------------------------------

inline Token tok(TokenType type, const std::string& lexeme = "")
{
    Token t; t.type = type; t.lexeme = lexeme; t.line = 1; t.literal = {};
    return t;
}

inline Token numTok(double val)
{
    Token t; t.type = TokenType::NUMBER; t.lexeme = std::to_string(val);
    t.line = 1; t.literal = val;
    return t;
}

inline Token eof() { return tok(TokenType::EOF_TOKEN); }

// -----------------------------------------------------------------------
// stdout 캡처 헬퍼
// -----------------------------------------------------------------------

template<typename Fn>
inline std::string captureOutput(Fn&& fn)
{
    testing::internal::CaptureStdout();
    fn();
    return testing::internal::GetCapturedStdout();
}

// -----------------------------------------------------------------------
// ExecutorFixture — Tokenizer / Parser / Executor 기반 테스트 공통 베이스
// ArrayFixture, FunctionFixture 등 실행기 테스트에서 상속하여 사용한다.
// -----------------------------------------------------------------------

class ExecutorFixture : public ::testing::Test
{
protected:
    std::string runSource(const std::string& source)
    {
        Tokenizer tz;
        Parser parser;
        auto stmts = parser.parse(tz.tokenize(source));
        Executor executor;
        return captureOutput([&]{ executor.execute(stmts); });
    }

    void runSourceExpectThrow(const std::string& source)
    {
        Tokenizer tz;
        Parser parser;
        auto stmts = parser.parse(tz.tokenize(source));
        Executor executor;
        ASSERT_THROW(executor.execute(stmts), std::runtime_error);
    }
};

// -----------------------------------------------------------------------
// AST 빌더 헬퍼 — Expr
// -----------------------------------------------------------------------

inline std::unique_ptr<Expr> makeLit(double v)
{
    auto e = std::make_unique<LiteralExpr>(); e->value = v; return e;
}

inline std::unique_ptr<Expr> makeLitStr(const std::string& s)
{
    auto e = std::make_unique<LiteralExpr>(); e->value = s; return e;
}

inline std::unique_ptr<Expr> makeLitBool(bool v)
{
    auto e = std::make_unique<LiteralExpr>(); e->value = v; return e;
}

inline std::unique_ptr<Expr> makeLitNull()
{
    auto e = std::make_unique<LiteralExpr>(); e->value = std::monostate{}; return e;
}

inline std::unique_ptr<Expr> makeVar(const Token& t)
{
    auto e = std::make_unique<VariableExpr>(); e->name = t; return e;
}

inline std::unique_ptr<Expr> makeBin(std::unique_ptr<Expr> l, TokenType op, std::unique_ptr<Expr> r)
{
    Token t; t.type = op;
    auto e = std::make_unique<BinaryExpr>();
    e->left = std::move(l); e->op = t; e->right = std::move(r);
    return e;
}

inline std::unique_ptr<Expr> makeUnary(TokenType op, std::unique_ptr<Expr> right)
{
    Token t; t.type = op;
    auto e = std::make_unique<UnaryExpr>();
    e->op = t; e->right = std::move(right);
    return e;
}

// -----------------------------------------------------------------------
// AST 빌더 헬퍼 — Stmt
// -----------------------------------------------------------------------

inline std::unique_ptr<PrintStmt> makePrint(std::unique_ptr<Expr> expr)
{
    auto s = std::make_unique<PrintStmt>(); s->expression = std::move(expr);
    return s;
}

inline std::unique_ptr<PrintStmt> makePrintLit(double v)   { return makePrint(makeLit(v)); }
inline std::unique_ptr<PrintStmt> makePrintStr(const std::string& s) { return makePrint(makeLitStr(s)); }
inline std::unique_ptr<PrintStmt> makePrintVar(const Token& t)       { return makePrint(makeVar(t)); }

inline std::unique_ptr<VarDeclareStmt> makeVarDecl(const Token& t, std::unique_ptr<Expr> init = nullptr)
{
    auto d = std::make_unique<VarDeclareStmt>();
    d->name = t; d->initializer = std::move(init);
    return d;
}

inline std::unique_ptr<VarDeclareStmt> makeVarDeclLit(const Token& t, double v)
{
    return makeVarDecl(t, makeLit(v));
}

// 여러 Stmt를 vector로 묶는 헬퍼 (C++17 fold expression)
template<typename... Stmts>
inline std::vector<std::unique_ptr<Stmt>> makeStmts(Stmts&&... stmts)
{
    std::vector<std::unique_ptr<Stmt>> v;
    (v.push_back(std::forward<Stmts>(stmts)), ...);
    return v;
}

// -----------------------------------------------------------------------
// 테스트용 컨트롤러 클래스 (DebugShellTest / BreakpointTest / StepTest 공유)
// -----------------------------------------------------------------------

// stdin 블로킹 없이 자동 step 진행
class NonBlockingController : public DebugController
{
public:
    void beforeExecute(Stmt*, Environment*, int) override
    {
        state_ = ExecutionState::STEP;
    }
};

// 정지한 줄번호를 기록하며 자동 step
class RecordingController : public DebugController
{
public:
    std::vector<int> stoppedAtLines;
    void beforeExecute(Stmt* stmt, Environment*, int) override
    {
        int line = currentLineNo_ > 0 ? currentLineNo_ : stmt->line;
        stoppedAtLines.push_back(line);
        state_ = ExecutionState::STEP;
    }
};

// RUNNING 모드에서 effectiveLine 기반 breakpoint 체크 (stdin 블로킹 없음)
class EffectiveLineController : public DebugController
{
public:
    int capturedLine = -1;
    void setRunning() { state_ = ExecutionState::RUNNING; }
    void beforeExecute(Stmt* stmt, Environment*, int) override
    {
        int effectiveLine = currentLineNo_ > 0 ? currentLineNo_ : stmt->line;
        if (state_ == ExecutionState::RUNNING && breakpoints_.isBreakpoint(effectiveLine))
        {
            state_       = ExecutionState::PAUSED;
            capturedLine = effectiveLine;
        }
    }
};

// breakpoint 도달 시 PAUSED로 전환 (stdin 블로킹 없음)
class PauseTrackingController : public DebugController
{
public:
    int pausedAtLine = -1;
    void setRunning() { state_ = ExecutionState::RUNNING; }
    void beforeExecute(Stmt* stmt, Environment*, int) override
    {
        if (state_ == ExecutionState::RUNNING && breakpoints_.isBreakpoint(stmt->line))
        {
            state_       = ExecutionState::PAUSED;
            pausedAtLine = stmt->line;
        }
    }
};

// step/next/continue를 스크립트로 실행하는 테스트용 컨트롤러
class ScriptedController : public DebugController
{
public:
    std::vector<int> pausedLines;

    enum class Cmd { STEP, NEXT, CONT };
    std::vector<Cmd> script;

    void beforeExecute(Stmt* stmt, Environment*, int depth) override
    {
        if (state_ == ExecutionState::RUNNING)
        {
            if (!breakpoints_.isBreakpoint(stmt->line)) return;
            state_ = ExecutionState::PAUSED;
        }
        if (state_ == ExecutionState::NEXT)
        {
            if (depth > nextDepth_) return;
            state_ = ExecutionState::STEP;
        }

        pausedLines.push_back(stmt->line);

        size_t i   = pausedLines.size() - 1;
        Cmd    cmd = (i < script.size()) ? script[i] : Cmd::CONT;
        switch (cmd)
        {
            case Cmd::STEP: state_ = ExecutionState::STEP; break;
            case Cmd::NEXT: nextDepth_ = depth; state_ = ExecutionState::NEXT; break;
            case Cmd::CONT: state_ = ExecutionState::RUNNING; break;
        }
    }
};
