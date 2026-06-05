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
    bool check(std::vector<std::unique_ptr<Stmt>>& statements);
    // rvalue 오버로드 — check(S(...)) 형태 허용
    bool check(std::vector<std::unique_ptr<Stmt>>&& statements) { return check(statements); }
    // const& 오버로드 — const 변수로 받은 stmts 에서도 호출 가능.
    // 벡터 요소 수는 변경하지 않으므로 const_cast 가 안전하다.
    bool check(const std::vector<std::unique_ptr<Stmt>>& statements)
    { return check(const_cast<std::vector<std::unique_ptr<Stmt>>&>(statements)); }
    const std::vector<std::string>& getErrors() const;
    void reset();

private:
    std::vector<std::string> errors_;
    std::vector<std::unordered_set<std::string>>  scopes_ = {{}};
    std::unordered_map<std::string, size_t>        funcs_;  // 함수명 → 파라미터 수
};
