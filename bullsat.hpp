#ifndef BULLSAT_HPP
#define BULLSAT_HPP
#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

namespace bullsat {

// definitions
enum class Status { Sat, Unsat, Unknown };
enum class LitBool { True, False, Undefine };
using Var = int;
struct Lit;
using Clause = std::vector<Lit>;
using CRef = std::shared_ptr<Clause>;

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

std::ostream &operator<<(std::ostream &os, const Clause &clause) {
  std::for_each(clause.begin(), clause.end(),
                [&](Lit lit) { os << lit << " "; });
  return os;
}

class Solver {
public:
  Solver(size_t variable_num) : que_head(0) {
    assings.resize(variable_num);
    watchers.resize(2 * variable_num);
    reasons.resize(variable_num);
    levels.resize(variable_num);
    que.clear();
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
  void enqueue(Lit lit, std::optional<CRef> reason = std::nullopt) {

    assert(!levels[lit.vidx()].has_value());
    levels[lit.vidx()] = decision_level();
    assings[lit.vidx()] = lit.pos() ? true : false;
    reasons[lit.vidx()] = reason;
    que.push_back(lit);
  }

  void new_var() {
    // literal index
    watchers.push_back(std::vector<CRef>());
    watchers.push_back(std::vector<CRef>());
    // variable index
    assings.push_back(false);
    reasons.push_back(std::nullopt);
    levels.push_back(std::nullopt);
  }

  void add_clause(const Clause &clause) {
    // grow the size
    std::for_each(clause.begin(), clause.end(), [&](Lit lit) {
      if (lit.vidx() >= assings.size()) {
        new_var();
      }
    });

    if (clause.size() == 1) {
      // Unit Clause
      enqueue(clause[0]);
    } else {
      CRef cr = std::make_shared<Clause>(clause);
      watchers[(~clause[0]).lidx()].push_back(cr);
      watchers[(~clause[1]).lidx()].push_back(cr);
      clauses.emplace_back(cr);
    }
  }
  std::optional<CRef> propagate() {
    while (que_head < que.size()) {
      const Lit lit = que[que_head++];
      const Lit nlit = ~lit;

      std::vector<CRef> &watcher = watchers[lit.lidx()];
      for (size_t i = 0; i < watcher.size(); i++) {
        CRef cr = watcher[i];
        Clause &clause = *cr;

        assert(clause[0] == nlit || clause[1] == nlit);
        // make sure that the clause[1] it false.
        if (clause[0] == nlit) {
          std::swap(clause[0], clause[1]);
        }
        assert(clause[1] == nlit && eval(clause[1]) == LitBool::False);

        Lit first = clause[0];
        // Already satisfied
        if (eval(first) == LitBool::True) {
          goto nextclause;
        }
        // clause[0] is False or Undefine
        // clause[1] is False
        // clause[2..] is False or True or Undefine.

        for (size_t k = 2; k < clause.size(); k++) {
          // Found a new lit to watch
          if (eval(clause[k]) != LitBool::False) {
            std::swap(clause[1], clause[k]);
            // Remove a value(swap the last one and pop back)
            watcher[i] = watcher.back();
            watcher.pop_back();
            // New watch
            watchers[(~clause[1]).lidx()].push_back(cr);
            i -= 1;
            goto nextclause;
          }
        }

        // clause[2..] is False

        if (eval(first) == LitBool::False) {
          // All literals are false
          // Conflict
          que_head = que.size();
          return std::move(cr);
        } else {
          // All literals excepting first are false
          // Unit Propagation
          assert(eval(first) == LitBool::Undefine);
          enqueue(first, cr);
        }
      nextclause:;
      }
    }

    return std::nullopt;
  }
  int decision_level() {
    if (que.empty()) {
      return 0;
    }
    Lit l = que.back();
    return levels[l.vidx()].value_or(0);
  }

  void analyze(CRef conflict) {}
  Status solve() {

    while (true) {
      if (std::optional<CRef> conflict = propagate()) {
        if (decision_level() == 0) {
          return Status::Unsat;
        }
        analyze(conflict.value());
      } else {
        // No Conflict
        std::optional<Lit> next = std::nullopt;
        for (size_t v = 0; v < assings.size(); v++) {
          if (!levels[v].has_value()) {
            // undefine
            next = Lit(static_cast<Var>(v), assings[v]);
            break;
          }
        }
        if (next) {
          Lit lit = next.value();
          enqueue(lit);
        } else {
          return Status::Sat;
        }
      }
    }
    return Status::Unknown;
  }

private:
  std::vector<CRef> clauses, learnts;
  std::vector<std::vector<CRef>> watchers;
  std::vector<std::optional<CRef>> reasons;
  std::vector<std::optional<int>> levels;
  std::deque<Lit> que;
  size_t que_head;
};
} // namespace bullsat

#endif
