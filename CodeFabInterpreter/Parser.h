#pragma once
#include <vector>
#include "Token.h"
#include "Stmt.h"

class Parser
{
public:
    std::vector<Stmt*> parse(const std::vector<Token>& tokens);
};
