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
    // side-effect: distance 기록(정적 바인딩) + 상수 폴딩으로 AST 를 변경한다.
    // const& 이지만 unique_ptr::get() 이 반환하는 non-const Stmt* 를 통해
    // 내부 Expr 노드를 교체한다. 호출자는 check() 후 AST 가 변경됨을 인지할 것.
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements);
    // rvalue 오버로드 — check(S(...)) 형태 허용
    bool check(std::vector<std::unique_ptr<Stmt>>&& statements) { return check(statements); }
    const std::vector<std::string>& getErrors() const;
    void reset();

private:
    std::vector<std::string> errors_;
    std::vector<std::unordered_set<std::string>>  scopes_ = {{}};
    std::unordered_map<std::string, size_t>        funcs_;  // 함수명 → 파라미터 수
};
