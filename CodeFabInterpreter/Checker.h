#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "Stmt.h"

class Checker
{
public:
    // side-effect: AST 내 VariableExpr/AssignExpr 의 distance 필드를 기록한다.
    // const& 이지만 unique_ptr::get() 이 반환하는 non-const 포인터를 통해
    // 노드 내부 값을 수정한다. 호출자는 check() 후 AST 가 변경됨을 인지할 것.
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements);
    const std::vector<std::string>& getErrors() const;
    void reset();

private:
    std::vector<std::string> errors_;
    std::vector<std::unordered_set<std::string>>  scopes_ = {{}};
    std::unordered_map<std::string, size_t>        funcs_;  // 함수명 → 파라미터 수
};
