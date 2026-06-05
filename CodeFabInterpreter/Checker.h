#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include "Stmt.h"

class Checker
{
public:
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements);
    const std::vector<std::string>& getErrors() const;

private:
    std::vector<std::string> errors_;
    std::vector<std::unordered_set<std::string>> scopes_ = {{}};
};
