#include "DebugController.h"
#include "Stmt.h"
#include <iostream>
#include <string>

void DebugController::beforeExecute(Stmt* stmt, Environment* /*env*/)
{
    if (state_ == ExecutionState::RUNNING) return;

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
        // Phase 5: break / remove / breakpoints
        // Phase 6: watch / unwatch / watches / inspect
        // Phase 7: next
        std::cout << "[Debug] 알 수 없는 명령어. (step / continue)\n";
        std::cout << "> " << std::flush;
    }
}
