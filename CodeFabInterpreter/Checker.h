#pragma once
#include <vector>
#include "Stmt.h"

class Checker
{
public:
    bool check(const std::vector<Stmt*>& statements);
};
