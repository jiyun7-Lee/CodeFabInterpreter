#pragma once
#include <unordered_map>
#include <string>
#include "Value.h"

class Environment
{
public:
    std::unordered_map<std::string, Value> values;
    Environment* parent = nullptr;

    void define(const std::string& name, Value value);
    Value get(const std::string& name) const;
    Value getAt(int distance, const std::string& name) const;
    void assign(const std::string& name, Value value);
    void assignAt(int distance, const std::string& name, Value value);
};
