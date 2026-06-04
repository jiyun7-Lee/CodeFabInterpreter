#pragma once
#include <vector>
#include <string>
#include "Token.h"

class Tokenizer
{
public:
    std::vector<Token> tokenize(const std::string& source);

private:
    Token scanString(const std::string& source, size_t& pos, int line);
    Token scanNumber(const std::string& source, size_t& pos, int line);
    Token scanWord  (const std::string& source, size_t& pos, int line);
};
