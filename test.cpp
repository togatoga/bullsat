#include "bullsat.hpp"
#include <cassert>
#include <iostream>
using namespace std;
using namespace bullsat;

void test_start(const char *funcname) {
  cerr << "==================== " << funcname
       << " ==================== " << endl;
}

void test_lit() {
  test_start(__func__);
  Lit x0 = Lit(0, true);   // x0
  Lit nx0 = Lit(0, false); // !x0
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
  cerr << x0 << " " << nx0 << " " << x1 << endl;
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
int main() {
  cerr << "===================== test ===================== " << endl;
  test_lit();
  test_enqueue_and_eval();
  test_propagate();
  test_analyze();
}
