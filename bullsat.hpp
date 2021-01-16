#ifndef BULLSAT_HPP_
#define BULLSAT_HPP_
#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bullsat {

// definitions
enum class Status { Sat, Unsat, Unknown };
enum class LitBool { True, False, Undefine };
using Var = int;
struct Lit;
using Clause = std::vector<Lit>;
using CRef = std::shared_ptr<Clause>;
using CWRef = std::weak_ptr<Clause>;

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

struct Heap {
  std::vector<Var> heap;
  std::vector<std::optional<size_t>> indices;
  std::vector<double> activity;
  Heap() = default;

  std::optional<Var> top() {
    if (heap.empty()) {
      return {};
    }
    return heap[0];
  }
  bool gt(Var left, Var right) {
    size_t l = static_cast<size_t>(left);
    size_t r = static_cast<size_t>(right);
    return activity[l] > activity[r];
  }
  void heap_up(size_t i) {
    if (i == 0) {
      return;
    }
    Var x = heap[i];
    size_t p = (i - 1) >> 1;

    while (i != 0) {
      if (!gt(x, heap[p])) {
        break;
      }
      heap[i] = heap[p];
      indices[static_cast<size_t>(heap[p])] = i;
      i = p;
      p = (p - 1) >> 1;
    }
    heap[i] = x;
    indices[static_cast<size_t>(x)] = i;
  }
  void heap_down(size_t i) {
    Var x = heap[i];
    while (2 * i + 1 < heap.size()) {
      size_t left = 2 * i + 1;
      size_t right = 2 * i + 2;
      size_t child =
          right < heap.size() && gt(heap[right], heap[left]) ? right : left;
      if (gt(heap[child], x)) {
        heap[i] = heap[child];
        indices[static_cast<size_t>(heap[i])] = i;
        i = child;
      } else {
        break;
      }
    }
    heap[i] = x;
    indices[static_cast<size_t>(x)] = i;
  }
  std::optional<Var> pop() {

    if (heap.empty()) {
      return {};
    }
    Var x = heap[0];
    indices[static_cast<size_t>(x)] = std::nullopt;
    if (heap.size() > 1) {
      heap[0] = heap.back();
      indices[static_cast<size_t>(heap[0])] = 0;
    }

    heap.pop_back();
    if (heap.size() > 1) {
      heap_down(0);
    }

    assert(!in_heap(x));
    return x;
  }
  void push(Var v) {
    if (in_heap(v)) {
      return;
    }
    while (static_cast<size_t>(v) >= indices.size()) {
      indices.push_back(std::nullopt);
      activity.push_back(0.0);
    }
    assert(!in_heap(v));
    indices[static_cast<size_t>(v)] = heap.size();
    heap.push_back(v);
    heap_up(indices[static_cast<size_t>(v)].value());
  }
  size_t size() const { return heap.size(); }
  bool empty() const { return heap.empty(); }
  bool in_heap(Var x) {
    return static_cast<size_t>(x) < indices.size() &&
           indices[static_cast<size_t>(x)].has_value();
  }
  void increase(Var n) {
    assert(in_heap(n));
    heap_up(indices[static_cast<size_t>(n)].value());
  }
  void decrease(Var n) {
    assert(in_heap(n));
    heap_down(indices[static_cast<size_t>(n)].value());
  }
  void update(Var n) {
    if (!in_heap(n)) {
      push(n);
    } else {
      const size_t idx = static_cast<size_t>(n);
      heap_up(indices[idx].value());
      heap_down(indices[idx].value());
    }
  }
};
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
  Solver() = default;
  explicit Solver(size_t variable_num)
      : skip_simplify(false), que_head(0), var_bump_inc(1.0) {
    assings.resize(variable_num);
    watchers.resize(2 * variable_num);
    reasons.resize(variable_num);
    levels.resize(variable_num);
    seen.resize(variable_num);
    que.clear();
    for (size_t v = 0; v < variable_num; v++) {
      order_heap.push(Var(v));
    }
  }
  [[nodiscard]] LitBool eval(Lit lit) const {
    if (!levels[lit.vidx()].has_value()) {
      return LitBool::Undefine;
    }
    if (lit.neg()) {
      return assings[lit.vidx()] ? LitBool::False : LitBool::True;
    }
    return assings[lit.vidx()] ? LitBool::True : LitBool::False;
  }
  [[nodiscard]] int decision_level() const {
    if (que.empty()) {
      return 0;
    }

    Lit l = que.back();
    return levels[l.vidx()].value_or(0);
  }
  void new_decision(Lit lit, std::optional<CRef> reason = std::nullopt) {
    enqueue(lit, reason);
    levels[lit.vidx()].value()++;
  }

  void enqueue(Lit lit, std::optional<CRef> reason = std::nullopt) {
    assert(!levels[lit.vidx()].has_value());
    levels[lit.vidx()] = decision_level();
    assings[lit.vidx()] = lit.pos() ? true : false;
    reasons[lit.vidx()] = reason;
    que.push_back(lit);
  }

  void pop_queue_until(int until_level) {
    assert(!que.empty());

    while (!que.empty()) {
      Lit lit = que.back();
      if (levels[lit.vidx()] > until_level) {
        if (!order_heap.in_heap(lit.var())) {
          order_heap.push(lit.var());
        }
        reasons[lit.vidx()] = std::nullopt;
        levels[lit.vidx()] = std::nullopt;
        que.pop_back();
      } else {
        break;
      }
    }
    if (que.size() > 0) {
      que_head = que.size() - 1;
    } else {
      que_head = 0;
    }
  }
  void var_bump_activity(Var v, double inc) {
    const size_t idx = static_cast<size_t>(v);
    order_heap.activity[idx] += inc;
    if (order_heap.activity[idx] > 1e100) {
      // rescale
      for (size_t i = 0; i < assings.size(); i++) {
        order_heap.activity[i] *= 1e-100;
      }
      var_bump_inc *= 1e-100;
    }
    if (order_heap.in_heap(v)) {
      order_heap.update(v);
    }
  }
  void new_var() {
    // literal index
    Var v = Var(assings.size());
    watchers.push_back(std::vector<CRef>());
    watchers.push_back(std::vector<CRef>());
    // variable index
    assings.push_back(false);
    seen.push_back(false);
    reasons.push_back(std::nullopt);
    levels.push_back(std::nullopt);
    order_heap.push(v);
  }
  void unwatch_clause(const CRef &cr) {
    const Clause &clause = *cr;
    assert(clause.size() > 1);
    for (const auto idx : {0, 1}) {
      auto &watcher = watchers[(~clause[static_cast<size_t>(idx)]).lidx()];
      for (size_t i = 0; i < watcher.size(); i++) {
        if (watcher[i] == cr) {
          watcher[i] = watcher.back();
          watcher.pop_back();
          return;
        }
      }
    }
    // not found
    assert(false);
    return;
  }
  void watch_clause(const CRef &cr) {
    const Clause &clause = *cr;
    assert(clause.size() > 1);
    watchers[(~clause[0]).lidx()].push_back(cr);
    watchers[(~clause[1]).lidx()].push_back(cr);
  }
  void attach_clause(const CRef &cr, bool learnt = false) {

    assert((*cr).size() > 1);
    watch_clause(cr);
    if (learnt) {
      learnts.push_back(cr);
    } else {
      clauses.push_back(cr);
    }
  }

  void add_clause(const Clause &clause) {
    assert(decision_level() == 0);
    // grow the size
    std::for_each(clause.begin(), clause.end(), [&](Lit lit) {
      if (lit.vidx() >= assings.size()) {
        new_var();
      }
    });
    Clause ps = clause;
    size_t new_len = 0;
    std::sort(ps.begin(), ps.end());
    for (size_t i = 0; i < ps.size(); i++) {
      if (eval(ps[i]) == LitBool::True) {
        return;
      }
      if (i >= 1) {
        if (ps[i] == ~ps[i - 1]) {
          return;
        }
        if (ps[i] == ps[i - 1]) {
          continue;
        }
      }
      if (eval(ps[i]) != LitBool::False) {
        ps[new_len++] = ps[i];
      }
    }
    ps.resize(new_len);
    if (ps.empty()) {
      status = Status::Unsat;
    } else if (ps.size() == 1) {
      // Unit Clause
      enqueue(ps[0]);
    } else {
      CRef cr = std::make_shared<Clause>(ps);
      attach_clause(cr);
    }
  }
  [[nodiscard]] std::optional<CRef> propagate() {
    while (que_head < que.size()) {
      assert(que_head >= 0);
      const Lit lit = que[que_head++];
      const Lit nlit = ~lit;

      std::vector<CRef> &watcher = watchers[lit.lidx()];
      for (size_t i = 0; i < watcher.size();) {
        CRef cr = watcher[i];
        const size_t next_idx = i + 1;
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
          i = next_idx;
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
            goto nextclause;
          }
        }

        // clause[2..] is False
        if (eval(first) == LitBool::False) {
          // All literals are false
          // Conflict
          return std::move(cr);
        } else {
          // All literals excepting first are false
          // Unit Propagation
          assert(eval(first) == LitBool::Undefine);
          enqueue(first, cr);
          i = next_idx;
        }
      nextclause:;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::pair<Clause, int> analyze(CRef conflict) {
    Clause learnt_clause;
    assert([&]() {
      bool ok = false;
      for (const bool b : seen) {
        ok |= b;
      }
      return !ok;
    }());
    const int conflicted_decision_level = decision_level();

    int counter = 0;
    {
      const Clause &clause = *conflict;

      // variables that are used to traverse by a conflicted clause
      for (const Lit &lit : clause) {
        assert(eval(lit) == LitBool::False);
        seen[lit.vidx()] = true;
        var_bump_activity(lit.var(), var_bump_inc);
        if (levels[lit.vidx()] < conflicted_decision_level) {
          learnt_clause.emplace_back(lit);
        } else {
          counter += 1;
        }
      }
      assert(counter >= 1);
    }

    // traverse a implication graph to a 1-UIP(first-uinque-implication-point)
    std::optional<Lit> first_uip = std::nullopt;
    for (size_t i = que.size() - 1; true; i--) {
      Lit lit = que[i];
      // Skip a variable that isn't checked.
      if (!seen[lit.vidx()]) {
        continue;
      }
      counter--;
      if (counter <= 0) {
        // 1-UIP
        first_uip = lit;
        break;
      }
      seen[lit.vidx()] = false;

      assert(reasons[lit.vidx()].has_value());
      CRef reason = reasons[lit.vidx()].value();
      const Clause clause = *reason;
      assert(clause[0] == lit);
      for (size_t j = 1; j < clause.size(); j++) {
        Lit clit = clause[j];
        // Already checked
        if (seen[clit.vidx()]) {
          continue;
        }
        seen[clit.vidx()] = true;
        var_bump_activity(lit.var(), var_bump_inc);
        if (levels[clit.vidx()] < conflicted_decision_level) {
          learnt_clause.push_back(clit);
        } else {
          counter += 1;
        }
      }
    }
    assert(first_uip.has_value());
    // learnt_clause[0] = !first_uip
    learnt_clause.push_back(~(first_uip.value()));
    std::swap(learnt_clause[0], learnt_clause.back());

    // Back Jump
    int back_jump_level = 0;
    for (size_t i = 1; i < learnt_clause.size(); i++) {
      assert(levels[learnt_clause[i].vidx()].has_value());
      back_jump_level =
          std::max(back_jump_level, levels[learnt_clause[i].vidx()].value());
    }

    for (const Lit &lit : learnt_clause) {
      seen[lit.vidx()] = false;
    }

    return std::make_pair(learnt_clause, back_jump_level);
  }
  bool locked(const CRef &cr) {
    const Clause &clause = *cr;

    if (eval(clause[0]) == LitBool::True &&
        reasons[clause[0].vidx()].has_value()) {
      // A clause is being propagated.
      return reasons[clause[0].vidx()].value() == cr;
    }
    return false;
  }
  void reduce_learnts() {
    std::sort(learnts.begin(), learnts.end(),
              [](const auto &left, const auto &right) {
                return left->size() < right->size();
              });
    size_t new_size = learnts.size() / 2;
    std::unordered_set<CRef> crs;
    for (size_t i = new_size; i < learnts.size(); i++) {
      if (learnts[i]->size() > 2 && !locked(learnts[i])) {
        unwatch_clause(learnts[i]);
        crs.insert(std::move(learnts[i]));
      } else {
        learnts[new_size] = std::move(learnts[i]);
        new_size++;
      }
    }
    learnts.resize(new_size);
  }

  void simplify() {
    assert(decision_level() == 0);
    auto remove_satisfied = [&](std::vector<CRef> &cls) {
      // learnts
      size_t new_cls_size = 0;
      for (size_t i = 0; i < cls.size(); i++) {
        CRef cr = cls[i];
        const Clause &clause = *cr;
        bool satisfied = false;
        for (size_t j = 0; j < clause.size(); j++) {
          LitBool lb = eval(clause[j]);
          if (lb == LitBool::True) {
            unwatch_clause(cr);
            satisfied = true;
            break;
          }
        }

        if (!satisfied) {
          cls[new_cls_size] = cr;
          new_cls_size++;
        }
      }
      cls.resize(new_cls_size);
    };

    remove_satisfied(learnts);
    remove_satisfied(clauses);
  }
  Status solve() {
    if (status) {
      return status.value();
    }
    double max_limit_learnts = static_cast<double>(clauses.size()) * 0.3;
    size_t conflict_cnt = 0;
    double restart_limit = 100;
    while (true) {
      if (std::optional<CRef> conflict = propagate()) {
        // Conflict
        conflict_cnt++;
        if (decision_level() == 0) {
          status = Status::Unsat;
          return Status::Unsat;
        }
        auto [learnt_clause, back_jump_level] = analyze(conflict.value());
        pop_queue_until(back_jump_level);
        if (learnt_clause.size() == 1) {
          enqueue(learnt_clause[0]);
          // a unit clause can simplify clauses
          // (!x1)
          // Delete: (!x1 v x2 v x3)
          skip_simplify = false;
        } else {
          CRef cr = std::make_shared<Clause>(learnt_clause);
          attach_clause(cr, true);
          enqueue(learnt_clause[0], cr);
        }

        var_bump_inc *= (1.0 / 0.95);
      } else {
        // No Conflict
        if (conflict_cnt >= static_cast<size_t>(restart_limit)) {
          restart_limit *= 1.1;
          // pop_queue_until(0);
        }

        if (!skip_simplify && decision_level() == 0) {
          // simplify clauses at the top level.
          simplify();
          skip_simplify = true;
        }

        if (learnts.size() >= static_cast<size_t>(max_limit_learnts)) {
          // Reduce the set of learnt clauses
          max_limit_learnts *= 1.1;
          reduce_learnts();
        }
        while (true) {
          // std::cout << std::endl;
          if (std::optional<Var> v = order_heap.pop()) {
            const size_t idx = static_cast<size_t>(v.value());
            if (levels[idx].has_value()) {
              continue;
            }

            Lit next = Lit(v.value(), assings[idx]);
            new_decision(next);
            break;
          } else {
            status = Status::Sat;
            return Status::Sat;
          }
        }
      }
    }
    status = Status::Unknown;
    return Status::Unknown;
  }
  // All variables
public:
  std::vector<bool> assings;
  std::optional<Status> status;

private:
  std::vector<CRef> clauses, learnts;
  std::vector<std::vector<CRef>> watchers;
  std::vector<std::optional<CRef>> reasons;
  std::vector<std::optional<int>> levels;
  std::vector<bool> seen;
  bool skip_simplify;

  std::deque<Lit> que;
  size_t que_head;
  Heap order_heap;
  double var_bump_inc;
};
struct CnfData {
  std::optional<size_t> var_num;
  std::optional<size_t> clause_num;
  std::vector<Clause> clauses;
};
CnfData parse_cnf(std::istream &in) {
  std::string line;
  const std::regex pattern("p cnf (\\d+) (\\d+)");
  CnfData data = {};
  while (std::getline(in, line)) {
    std::stringstream stream(line);
    std::string word;

    std::vector<std::string> words;

    while (std::getline(stream, word, ' ')) {
      if (word.empty()) {
        continue;
      }
      words.push_back(word);
    }
    if (words.empty() || words[0] == "c") {
      continue;
    }

    line = std::accumulate(
        std::next(words.begin()), words.end(), words[0],
        [](std::string left, std::string right) { return left + " " + right; });
    std::smatch matches;

    // p cnf 123 567
    if (std::regex_match(line, matches, pattern)) {
      int var_num = std::stoi(matches[1]);
      int clause_num = std::stoi(matches[2]);
      if (var_num >= 0 && clause_num >= 0) {
        data.var_num = var_num;
        data.clause_num = clause_num;
      }
      continue;
    }
    // Parse a clause from line.
    // 1 2 -3 0
    Clause clause = Clause();
    for (const auto &w : words) {
      if (w == "0") {
        break;
      }
      int num = std::stoi(w);
      assert(num != 0);
      int d = std::abs(num) - 1;
      if (num > 0) {
        clause.emplace_back(Lit(d, true));
      } else {
        clause.emplace_back(Lit(d, false));
      }
    }
    if (!clause.empty()) {
      data.clauses.emplace_back(clause);
    }
  }
  return data;
}
} // namespace bullsat

#endif // BULLSAT_HPP_
