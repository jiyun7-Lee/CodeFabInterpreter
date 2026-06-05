#include "Environment.h"
#include <stdexcept>

void Environment::define(const std::string& name, Value value)
{
    values[name] = value;
}

Value Environment::get(const std::string& name, int line) const
{
    auto it = values.find(name);
    if (it != values.end())
        return it->second;
    if (parent)
        return parent->get(name, line);
    std::string prefix = line > 0 ? "[" + std::to_string(line) + "번째 줄] " : "";
    throw std::runtime_error(prefix + "미정의된 변수 '" + name + "'");
}

void Environment::assign(const std::string& name, Value value, int line)
{
    auto it = values.find(name);
    if (it != values.end())
    {
        it->second = value;
        return;
    }
    if (parent)
    {
        parent->assign(name, value, line);
        return;
    }
    std::string prefix = line > 0 ? "[" + std::to_string(line) + "번째 줄] " : "";
    throw std::runtime_error(prefix + "미정의된 변수 '" + name + "'");
}
