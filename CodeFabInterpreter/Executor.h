#pragma once
#include <vector>
#include "Stmt.h"
#include "Environment.h"

class Executor
{
public:
    void execute(const std::vector<Stmt*>& statements);

private:
    Environment globalEnv;
};
