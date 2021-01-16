#include "bullsat.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace bullsat;
void help() {
  std::cout << "Usage: bullsat <input-file> [output-file]" << std::endl;
}

void write_result(const Solver &solver, Status status, std::ostream &os,
                  bool tostdout) {
  std::string result;
  if (status == Status::Sat) {
    result = "SAT";
  } else if (status == Status::Unsat) {
    result = "UNSAT";
  } else {
    result = "UNKNOWN";
  }

  if (tostdout) {
    os << "s " << result << std::endl;
  } else {
    os << result << std::endl;
  }
  if (status == Status::Sat) {
    std::string assigns = "";
    for (size_t v = 0; v < solver.assings.size(); v++) {
      if (solver.assings[v]) {
        assigns += std::to_string(v + 1) + " ";
      } else {
        assigns += "-" + std::to_string(v + 1) + " ";
      }
    }
    assigns += "0";
    os << assigns << std::endl;
  }
}
int main(int argc, char *argv[]) {
  if (!(argc == 2 || argc == 3)) {
    help();
    std::exit(1);
  }
  std::ifstream input(argv[1]);
  auto cnf = bullsat::parse_cnf(input);
  Solver solver;
  if (cnf.var_num) {
    solver = Solver(cnf.var_num.value());
  }
  auto clauses = cnf.clauses;
  std::for_each(clauses.begin(), clauses.end(),
                [&](const Clause &clause) { solver.add_clause(clause); });
  Status status = solver.solve();

  if (argc == 3) {
    std::ofstream ofs(argv[2]);
    write_result(solver, status, ofs, false);
  } else {
    write_result(solver, status, std::cout, true);
  }
}
