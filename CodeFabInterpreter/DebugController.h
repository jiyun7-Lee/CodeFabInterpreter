#pragma once

class Stmt;
class Environment;

// beforeExecute() 호출 시 "정지할지 / 통과할지"를 결정하는 상태 머신
// STEP    : 매 Stmt마다 정지 — 사용자 입력 대기
// RUNNING : 자유 실행 — breakpoint 도달 시 PAUSED로 전환 (Phase 5)
// PAUSED  : breakpoint에 걸려 정지 — 사용자 입력 대기 (Phase 5)
// NEXT    : 블록 내부 진입 없이 현재 depth 이하 Stmt는 통과 (Phase 7)
enum class ExecutionState { RUNNING, PAUSED, STEP, NEXT };

// -----------------------------------------------------------------------
// DebugController — Executor hook 수신자 (Phase 4 뼈대)
// Phase 5~7 에서 Breakpoint / Watch / Step 기능 추가 예정
// -----------------------------------------------------------------------
class DebugController
{
public:
    virtual ~DebugController() = default;
    virtual void beforeExecute(Stmt* stmt, Environment* env);
protected:
    ExecutionState state_ = ExecutionState::STEP;
};
