#pragma once
#include <vector>
#include <string>
#include "Stmt.h"

class Checker
{
public:
    bool check(const std::vector<Stmt*>& statements);
    const std::vector<std::string>& getErrors() const;

private:
    std::vector<std::string> errors_;
};
