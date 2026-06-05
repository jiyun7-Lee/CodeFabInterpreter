#include "DebugController.h"
#include "Stmt.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------
// BreakpointManager
// -----------------------------------------------------------------------

void BreakpointManager::add(int line)    { breakpoints_.insert(line); }
void BreakpointManager::remove(int line) { breakpoints_.erase(line); }
bool BreakpointManager::isBreakpoint(int line) const { return breakpoints_.count(line) > 0; }

void BreakpointManager::print() const
{
    if (breakpoints_.empty())
    {
        std::cout << "(설정된 breakpoint 없음)\n";
        return;
    }
    std::vector<int> sorted(breakpoints_.begin(), breakpoints_.end());
    std::sort(sorted.begin(), sorted.end());
    for (int line : sorted)
        std::cout << "  줄 " << line << "\n";
}

// -----------------------------------------------------------------------
// DebugController
// -----------------------------------------------------------------------

void DebugController::beforeExecute(Stmt* stmt, Environment* /*env*/)
{
    // RUNNING 중 breakpoint 도달 시 PAUSED로 전환
    if (state_ == ExecutionState::RUNNING)
    {
        if (breakpoints_.isBreakpoint(stmt->line))
            state_ = ExecutionState::PAUSED;
        else
            return;
    }

    // STEP 또는 PAUSED: 사용자 입력 대기
    std::cout << "[Debug] 줄 " << stmt->line << " 에서 정지\n";
    std::cout << "> " << std::flush;

    std::string cmd;
    while (std::getline(std::cin, cmd))
    {
        if (cmd == "step" || cmd == "s")
        {
            state_ = ExecutionState::STEP;
            return;
        }
        if (cmd == "continue" || cmd == "c")
        {
            state_ = ExecutionState::RUNNING;
            return;
        }
        if (cmd.rfind("break ", 0) == 0)
        {
            try
            {
                int line = std::stoi(cmd.substr(6));
                breakpoints_.add(line);
                std::cout << "[Debug] 줄 " << line << " 에 breakpoint 설정\n";
            }
            catch (...) { std::cout << "[Debug] 잘못된 줄번호\n"; }
        }
        else if (cmd.rfind("remove ", 0) == 0)
        {
            try
            {
                int line = std::stoi(cmd.substr(7));
                breakpoints_.remove(line);
                std::cout << "[Debug] 줄 " << line << " breakpoint 해제\n";
            }
            catch (...) { std::cout << "[Debug] 잘못된 줄번호\n"; }
        }
        else if (cmd == "breakpoints")
        {
            breakpoints_.print();
        }
        else
        {
            // Phase 6: watch / unwatch / watches / inspect
            // Phase 7: next
            std::cout << "[Debug] 알 수 없는 명령어."
                         " (step / continue / break N / remove N / breakpoints)\n";
        }
        std::cout << "> " << std::flush;
    }
}
