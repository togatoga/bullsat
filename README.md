# Bullsat

`bullsat` is an educational CDCL (Conflict-Driven-Clause-Learning) SAT solver to understand and explore SAT solver algorithms and implementations.  
[ゼロから作るCDCL SATソルバ](https://togatoga.github.io/bullsat/)

## How to
### Command
```bash
% make release
mkdir -p build/release/
clang++ -std=c++17 -Weverything -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-padded -O3 -DNDEBUG -o build/release/bullsat main.cpp
% ./build/release/bullsat
Usage: bullsat <input-file> [output-file]
% ./build/release/bullsat cnf/sat.cnf                                                     
s SAT
1 2 -3 0
% ./build/release/bullsat cnf/unsat.cnf                                                     
s UNSAT
```

### Test
```bash
% make test   
clang++ -std=c++17 -Weverything -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-padded -g -fsanitize=undefined -o test test.cpp 
./test
===================== test ===================== 
==================== test_heap ==================== 
==================== test_lit ==================== 
==================== test_enqueue_and_eval ==================== 
==================== test_propagate ==================== 
==================== test_queue ==================== 
==================== test_analyze ==================== 
==================== test_solve ==================== 
==================== test_parse_cnf ==================== 

```