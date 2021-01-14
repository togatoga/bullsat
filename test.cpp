#include "bullsat.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;
using namespace bullsat;

void test_start(const char *funcname) {
  cerr << "==================== " << funcname
       << " ==================== " << endl;
}

void test_lit() {
  test_start(__func__);
  Lit x0 = Lit(0, true); // x0
  Lit nx0 = ~x0;         // !x0
  assert(x0.lidx() == 0);
  assert(nx0.lidx() == 1);

  assert(x0.pos());
  assert(!x0.neg());
  assert(nx0.neg());
  assert(x0.pos());

  assert(x0.var() == nx0.var());

  assert(x0 != nx0);
  assert(~x0 == nx0);
  assert(x0.lidx() == (~nx0).lidx());

  Lit x1 = Lit(1, true); // x1
  assert(x1.lidx() == 2);
  assert(x0 < x1);
  assert(!x1.neg());
}

void test_enqueue_and_eval() {
  test_start(__func__);
  {
    size_t n = 10;
    Solver solver = Solver(n);
    for (size_t i = 0; i < n; i++) {
      Var x = static_cast<Var>(i);
      assert(solver.eval(Lit(x, true)) == LitBool::Undefine);
      assert(solver.eval(Lit(x, false)) == LitBool::Undefine);
    }
    Lit x0 = Lit(0, true);
    Lit nx0 = ~x0;
    Lit x1 = Lit(1, true);
    solver.enqueue(x0);
    assert(solver.eval(x0) == LitBool::True);
    assert(solver.eval(nx0) == LitBool::False);
    assert(solver.eval(x1) == LitBool::Undefine);
  }
}

void test_propagate() {
  test_start(__func__);
  {
    // Conflict
    // x0 & x1 & (!x0 v !x1)
    Solver solver = Solver(10);
    {
      // x0
      Clause clause = Clause();
      clause.push_back(Lit(0, true));
      solver.add_clause(clause);
    }
    {
      // x1
      Clause clause = Clause();
      clause.push_back(Lit(1, true));
      solver.add_clause(clause);
    }
    {
      // (!x0 v !x1)
      Clause clause = Clause();
      clause.push_back(Lit(0, false));
      clause.push_back(Lit(1, false));
      solver.add_clause(clause);
    }
    auto confl = solver.propagate();
    auto clause = *confl->get();
    std::sort(clause.begin(), clause.end());
    assert(clause[0] == Lit(0, false));
    assert(clause[1] == Lit(1, false));
  }
  {
    // Unit Propagation
    // x0 & x1 & (!x0 v !x1 v !x2)
    // x2=false
    Solver solver = Solver(10);
    {
      // x0
      Clause clause = Clause();
      clause.push_back(Lit(0, true));
      solver.add_clause(clause);
    }
    {
      // x1
      Clause clause = Clause();
      clause.push_back(Lit(1, true));
      solver.add_clause(clause);
    }
    {
      // (!x0 v !x1 v x2)
      Clause clause = Clause();
      clause.push_back(Lit(0, false));
      clause.push_back(Lit(1, false));
      clause.push_back(Lit(2, false));
      solver.add_clause(clause);
    }
    auto confl = solver.propagate();
    assert(!confl.has_value());
    assert(solver.eval(Lit(2, false)) == LitBool::True);
  }
}

void test_analyze() {
  test_start(__func__);
  {

    Solver solver = Solver(7);
    {
      // (!x0 v x1)
      Clause clause = Clause();
      clause.push_back(Lit(0, false));
      clause.push_back(Lit(1, true));
      solver.add_clause(clause);
    }
    {
      // (!x1 v x2)
      Clause clause = Clause();
      clause.push_back(Lit(1, false));
      clause.push_back(Lit(2, true));
      solver.add_clause(clause);
    }
    {
      // (!x1 v x3)
      Clause clause = Clause();
      clause.push_back(Lit(1, false));
      clause.push_back(Lit(3, true));
      solver.add_clause(clause);
    }

    {
      // (!x5 v !x2 v x4)
      Clause clause = Clause();
      clause.push_back(Lit(5, false));
      clause.push_back(Lit(2, false));
      clause.push_back(Lit(4, true));
      solver.add_clause(clause);
    }
    {
      // (!x6 v !x3 v !x4);
      Clause clause = Clause();
      clause.push_back(Lit(6, false));
      clause.push_back(Lit(3, false));
      clause.push_back(Lit(4, false));
      solver.add_clause(clause);
    }

    solver.new_decision(Lit(5, true));
    solver.new_decision(Lit(6, true));
    solver.new_decision(Lit(0, true));
    auto confl = solver.propagate();

    auto [learnt_clause, level] = solver.analyze(confl.value());
    assert(learnt_clause.size() == 3 && level == 2);
    // (!x1 v !x5 v !x6)
    Clause l = Clause{Lit(1, false), Lit(5, false), Lit(6, false)};
    assert(learnt_clause == l);
  }
}

void test_queue() {
  test_start(__func__);
  Solver solver = Solver(10);

  // @1: x0 x1 x2
  // @2: x3 x4
  // @3: x5
  assert(solver.decision_level() == 0);
  solver.new_decision(Lit(0, true));
  solver.enqueue(Lit(1, true));
  solver.enqueue(Lit(2, true));
  assert(solver.decision_level() == 1);
  solver.new_decision(Lit(3, true));
  solver.enqueue(Lit(4, true));
  assert(solver.decision_level() == 2);
  solver.new_decision(Lit(5, true));

  // pop queue until the level 1.
  // @1: x0 x1 x2
  solver.pop_queue_until(1);
  assert(solver.decision_level() == 1);
  assert(solver.eval(Lit(0, true)) == LitBool::True);
  assert(solver.eval(Lit(1, true)) == LitBool::True);
  assert(solver.eval(Lit(2, true)) == LitBool::True);

  assert(solver.eval(Lit(3, true)) == LitBool::Undefine);
}

bool validate_satisfiable(const vector<Clause> &clauses, const Solver &solver) {
  for (const auto &clause : clauses) {
    bool satisfied = false;
    for (const auto &lit : clause) {
      if (solver.eval(lit) == LitBool::True) {
        satisfied = true;
        break;
      }
    }
    if (!satisfied) {
      return false;
    }
  }
  return true;
}
void test_solve() {
  test_start(__func__);

  {
    // SATISFIABLE
    //(x0 v !x4 v x3) and (!x0 v x4 v x2 v x3) and (x2 v x3)
    Solver solver = Solver(5);
    vector<Clause> clauses = {
        Clause{Lit(0, true), Lit(4, false), Lit(3, true)},
        Clause{Lit(0, false), Lit(4, true), Lit(2, true), Lit(3, true)},
        Clause{Lit(2, true), Lit(3, true)}};
    std::for_each(clauses.begin(), clauses.end(),
                  [&](Clause &c) { solver.add_clause(c); });

    assert(solver.solve() == Status::Sat);
    assert(validate_satisfiable(clauses, solver));
  }
  {
    // UNSATISFIABLE
    // (x0) and (!x0 v !x2) and (!x0 v x1 v x2) and (x2 v !x1)
    Solver solver = Solver(3);
    vector<Clause> clauses = {Clause{Lit(0, true)},
                              Clause{Lit(0, false), Lit(2, false)},
                              Clause{Lit(0, false), Lit(1, true), Lit(2, true)},
                              Clause{Lit(2, true), Lit(1, false)}};
    std::for_each(clauses.begin(), clauses.end(),
                  [&](Clause &c) { solver.add_clause(c); });

    assert(solver.solve() == Status::Unsat);
  }
}

void test_parse_cnf() {
  test_start(__func__);

  std::ifstream file("./cnf/sat.cnf");
  // c
  // c This is a sample input file.
  // c
  // p cnf 3 5
  //  1 -2  3 0
  // -1  2 0
  // -2 -3 0
  //  1  2 -3 0
  //  1  3 0
  CnfData data = parse_cnf(file);
  assert(data.var_num.value() == 3);
  assert(data.clause_num.value() == 5);
  vector<Clause> clauses = {Clause{Lit(0, true), Lit(1, false), Lit(2, true)},
                            Clause{Lit(0, false), Lit(1, true)},
                            Clause{Lit(1, false), Lit(2, false)},
                            Clause{Lit(0, true), Lit(1, true), Lit(2, false)},
                            Clause{Lit(0, true), Lit(2, true)}};
  assert(data.clauses == clauses);
}

int main() {
  cerr << "===================== test ===================== " << endl;
  test_lit();
  test_enqueue_and_eval();
  test_propagate();
  test_queue();
  test_analyze();
  test_solve();
  test_parse_cnf();
}
