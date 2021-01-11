#ifndef BULLSAT_HPP
#define BULLSAT_HPP
#include <cassert>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

namespace bullsat {

// definitions
enum class Status { Sat, Unsat, Unknown };
enum class LitBool { True, False, Undefine };
using Var = int;
struct Lit;
using ClauseIdx = size_t;
using Clause = std::vector<Lit>;

// x is
// even: positive x0 (0 -> x0, 2 -> x1)
// odd: negative !x0 (1 -> !x0, 3 -> !x1)
struct Lit {
  Lit() = default;
  Lit(const Lit &lit) = default;
  // 0-index
  // Lit(0, true) means x0
  // Lit(0, false) means !x0
  Lit(Var v, bool positive) {
    assert(v >= 0);
    x = positive ? 2 * v : 2 * v + 1;
  }
  bool operator==(Lit lit) const { return x == lit.x; }
  bool operator!=(Lit lit) const { return x != lit.x; }
  bool operator<(Lit lit) const { return x < lit.x; }
  bool pos() const { return !neg(); }
  bool neg() const { return x & 1; }
  Var var() const { return x >> 1; }
  size_t vidx() const { return static_cast<size_t>(var()); }
  size_t lidx() const { return static_cast<size_t>(x); }
  int x;
};

// ~x0 = !x0
inline Lit operator~(Lit p) {
  Lit q(p);
  q.x ^= 1;
  return q;
}
std::ostream &operator<<(std::ostream &os, const Lit &lit) {
  os << (lit.neg() ? "!x" : "x") << lit.var();
  return os;
}

class Solver {
public:
  Solver(size_t variable_num) : que_head(0) {
    assings.resize(variable_num);
    watchers.resize(2 * variable_num);
    reasons.resize(variable_num);
    levels.resize(variable_num);
    que.resize(variable_num);
  }
  std::vector<bool> assings;
  std::optional<Status> status;
  LitBool eval(Lit lit) {
    if (!levels[lit.vidx()].has_value()) {
      return LitBool::Undefine;
    }
    if (lit.neg()) {
      return assings[lit.vidx()] ? LitBool::False : LitBool::True;
    }
    return assings[lit.vidx()] ? LitBool::True : LitBool::False;
  }
  void enqueue(Lit lit, std::optional<ClauseIdx> reason = std::nullopt) {

    assert(!levels[lit.vidx()].has_value());
    levels[lit.vidx()] = [&]() {
      if (que.empty()) {
        return 0;
      }
      Lit last = que.back();
      return levels[last.vidx()].value_or(0);
    }();
    assings[lit.vidx()] = lit.pos() ? true : false;
    reasons[lit.vidx()] = reason;
    que.push_back(lit);
  }

private:
  std::vector<Clause> clauses;
  std::vector<std::vector<ClauseIdx>> watchers;
  std::vector<std::optional<ClauseIdx>> reasons;
  std::vector<std::optional<int>> levels;
  std::vector<Lit> que;
  int que_head;
};
} // namespace bullsat

#endif
