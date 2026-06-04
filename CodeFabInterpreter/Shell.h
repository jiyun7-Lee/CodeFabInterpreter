#pragma once
#include <string>
#include "Executor.h"

class Shell
{
public:
    void run();
    void runLine(const std::string& source);
private:
    Executor executor_;
};
