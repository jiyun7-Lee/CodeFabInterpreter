#pragma once
#include <vector>
#include <memory>
#include "Stmt.h"
#include "Environment.h"

class Executor
{
public:
    void execute(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    Environment globalEnv;
};
