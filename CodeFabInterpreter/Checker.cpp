#include "Checker.h"

bool Checker::check(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    return true;
}

const std::vector<std::string>& Checker::getErrors() const
{
    return errors_;
}
