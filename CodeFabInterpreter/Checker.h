#pragma once
#include <vector>
#include <memory>
#include "Stmt.h"

class Checker
{
public:
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements);
};
