#pragma once
#include <variant>
#include <string>
#include <vector>
#include <memory>

struct ArrayValue;  // forward declaration — enables recursive Value
using Value = std::variant<std::monostate, double, std::string, bool,
                           std::shared_ptr<ArrayValue>>;
struct ArrayValue { std::vector<Value> elements; };
