#include "Environment.h"
#include <stdexcept>

void Environment::define(const std::string& name, Value value)
{
    values[name] = value;
}

Value Environment::get(const std::string& name) const
{
    auto it = values.find(name);
    if (it != values.end())
        return it->second;
    if (parent)
        return parent->get(name);
    throw std::runtime_error("Undefined variable '" + name + "'");
}

Value Environment::getAt(int distance, const std::string& name) const
{
    const Environment* env = this;
    for (int i = 0; i < distance; i++)
        env = env->parent;
    return env->values.at(name);
}

void Environment::assignAt(int distance, const std::string& name, Value value)
{
    Environment* env = this;
    for (int i = 0; i < distance; i++)
        env = env->parent;
    env->values.at(name) = value;
}

void Environment::assign(const std::string& name, Value value)
{
    auto it = values.find(name);
    if (it != values.end())
    {
        it->second = value;
        return;
    }
    if (parent)
    {
        parent->assign(name, value);
        return;
    }
    throw std::runtime_error("Undefined variable '" + name + "'");
}
