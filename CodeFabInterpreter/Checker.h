#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Stmt.h"

class Checker
{
public:
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements);
    const std::vector<std::string>& getErrors() const;

private:
    std::vector<std::string> errors_;
};
