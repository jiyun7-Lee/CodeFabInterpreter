#pragma once
#include <unordered_map>
#include <string>
#include "Value.h"

class Environment
{
public:
    virtual ~Environment() = default;

    std::unordered_map<std::string, Value> values;
    Environment* parent = nullptr;

    void define(const std::string& name, Value value);
    virtual Value get(const std::string& name, int line = 0) const;
    const std::unordered_map<std::string, Value>& getAll() const { return values; }
    virtual Value getAt(int distance, const std::string& name) const;
    void assign(const std::string& name, Value value, int line = 0);
    void assignAt(int distance, const std::string& name, Value value);
};
