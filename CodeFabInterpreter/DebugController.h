#pragma once
#include <unordered_set>
#include <string>

class Stmt;
class Environment;

// beforeExecute() 호출 시 "정지할지 / 통과할지"를 결정하는 상태 머신
// STEP    : 매 Stmt마다 정지 — 사용자 입력 대기
// RUNNING : 자유 실행 — breakpoint 도달 시 PAUSED로 전환 (Phase 5)
// PAUSED  : breakpoint에 걸려 정지 — 사용자 입력 대기 (Phase 5)
// NEXT    : 블록 내부 진입 없이 현재 depth 이하 Stmt는 통과 (Phase 7)
enum class ExecutionState { RUNNING, PAUSED, STEP, NEXT };

// -----------------------------------------------------------------------
// BreakpointManager — 줄번호 기반 breakpoint 집합 관리 (Phase 5)
// -----------------------------------------------------------------------
class BreakpointManager
{
public:
    void add(int line);
    void remove(int line);
    bool isBreakpoint(int line) const;
    void print() const;
private:
    std::unordered_set<int> breakpoints_;
};

// -----------------------------------------------------------------------
// WatchManager — 감시 변수 집합 관리 (Phase 6)
// -----------------------------------------------------------------------
class WatchManager
{
public:
    void add(const std::string& name);
    void remove(const std::string& name);
    bool empty() const;
    void printWatches(const Environment* env) const; // 감시 변수 값 출력
    void printInspect(const Environment* env) const; // 현재 스코프 전체 출력
private:
    std::unordered_set<std::string> watches_;
};

// -----------------------------------------------------------------------
// DebugController — Executor hook 수신자
// -----------------------------------------------------------------------
class DebugController
{
public:
    virtual ~DebugController() = default;
    virtual void beforeExecute(Stmt* stmt, Environment* env);
    void addBreakpoint(int line)          { breakpoints_.add(line); }
    void removeBreakpoint(int line)       { breakpoints_.remove(line); }
    void addWatch(const std::string& n)   { watches_.add(n); }
    void removeWatch(const std::string& n){ watches_.remove(n); }
protected:
    ExecutionState    state_       = ExecutionState::STEP;
    BreakpointManager breakpoints_;
    WatchManager      watches_;
};
