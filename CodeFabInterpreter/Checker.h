#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "Stmt.h"

class Checker
{
public:
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements);
    const std::vector<std::string>& getErrors() const;
    void reset();

private:
    std::vector<std::string> errors_;
    std::vector<std::unordered_set<std::string>>  scopes_ = {{}};
    std::unordered_map<std::string, size_t>        funcs_;  // 함수명 → 파라미터 수
};
