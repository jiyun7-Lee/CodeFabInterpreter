#include "DebugController.h"
#include "Stmt.h"
#include "Environment.h"
#include "Value.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------
// 내부 헬퍼
// -----------------------------------------------------------------------
static std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}


static std::string valueToString(const Value& val)
{
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>)
        {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        }
        if constexpr (std::is_same_v<T, std::string>)  return "\"" + v + "\"";
        if constexpr (std::is_same_v<T, bool>)         return v ? "true" : "false";
        if constexpr (std::is_same_v<T, std::monostate>) return "null";
        if constexpr (std::is_same_v<T, std::shared_ptr<ArrayValue>>) return "[array]";
        return "?";
    }, val);
}

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
// WatchManager
// -----------------------------------------------------------------------

void WatchManager::add(const std::string& name)    { watches_.insert(name); }
void WatchManager::remove(const std::string& name) { watches_.erase(name); }
bool WatchManager::empty() const                   { return watches_.empty(); }

void WatchManager::printWatches(const Environment* env) const
{
    if (watches_.empty())
    {
        std::cout << "(감시 중인 변수 없음)\n";
        return;
    }
    std::vector<std::string> sorted(watches_.begin(), watches_.end());
    std::sort(sorted.begin(), sorted.end());
    for (const auto& name : sorted)
    {
        try   { std::cout << "  " << name << " = " << valueToString(env->get(name)) << "\n"; }
        catch (...) { std::cout << "  " << name << " = (스코프 밖)\n"; }
    }
}

void WatchManager::printInspect(const Environment* env) const
{
    if (!env || env->values.empty())
    {
        std::cout << "(현재 스코프에 변수 없음)\n";
        return;
    }
    std::vector<std::string> names;
    for (const auto& kv : env->values)
        names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    for (const auto& name : names)
        std::cout << "  " << name << " = " << valueToString(env->values.at(name)) << "\n";
}

// -----------------------------------------------------------------------
// DebugController
// -----------------------------------------------------------------------

void DebugController::beforeExecute(Stmt* stmt, Environment* env)
{
    // RUNNING 중 breakpoint 도달 시 PAUSED로 전환
    if (state_ == ExecutionState::RUNNING)
    {
        if (breakpoints_.isBreakpoint(stmt->line))
            state_ = ExecutionState::PAUSED;
        else
            return;
    }

    // 정지 시 감시 변수 자동 출력
    if (!watches_.empty())
        watches_.printWatches(env);

    std::cout << "[Debug] 줄 " << stmt->line << " 에서 정지\n";
    std::cout << "> " << std::flush;

    std::string cmd;
    while (std::getline(std::cin, cmd))
    {
        const std::string low = toLower(cmd);

        if (low == "step" || low == "s")      { state_ = ExecutionState::STEP;    return; }
        if (low == "continue" || low == "c")  { state_ = ExecutionState::RUNNING; return; }

        if (low.rfind("break ", 0) == 0)
        {
            try
            {
                int line = std::stoi(cmd.substr(6));
                breakpoints_.add(line);
                std::cout << "[Debug] 줄 " << line << " 에 breakpoint 설정\n";
            }
            catch (...) { std::cout << "[Debug] 잘못된 줄번호\n"; }
        }
        else if (low.rfind("remove ", 0) == 0)
        {
            try
            {
                int line = std::stoi(cmd.substr(7));
                breakpoints_.remove(line);
                std::cout << "[Debug] 줄 " << line << " breakpoint 해제\n";
            }
            catch (...) { std::cout << "[Debug] 잘못된 줄번호\n"; }
        }
        else if (low == "breakpoints") { breakpoints_.print(); }
        else if (low.rfind("watch ", 0) == 0)
        {
            std::string name = cmd.substr(6);
            watches_.add(name);
            std::cout << "[Debug] '" << name << "' 감시 시작\n";
        }
        else if (low.rfind("unwatch ", 0) == 0)
        {
            std::string name = cmd.substr(8);
            watches_.remove(name);
            std::cout << "[Debug] '" << name << "' 감시 해제\n";
        }
        else if (low == "watches") { watches_.printWatches(env); }
        else if (low == "inspect") { watches_.printInspect(env); }
        else
        {
            // Phase 7: next
            std::cout << "[Debug] 알 수 없는 명령어."
                         " (step/continue/break N/remove N/breakpoints"
                         "/watch V/unwatch V/watches/inspect)\n";
        }
        std::cout << "> " << std::flush;
    }
}
