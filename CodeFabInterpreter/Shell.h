#pragma once
#include <string>

class Shell
{
public:
    void run();
    void runLine(const std::string& source);
};
