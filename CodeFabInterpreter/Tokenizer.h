#pragma once
#include <vector>
#include <string>
#include "Token.h"

class Tokenizer
{
public:
    std::vector<Token> tokenize(const std::string& source);
};
