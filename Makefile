CXX := clang++
CXXFLAGS := -std=c++17 -Weverything -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-padded
DEBUGFLAGS := -g -fsanitize=undefined

all: format test

test: test.cpp bullsat.hpp
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ test.cpp 
	./$@

format:
	clang-format -i *.cpp *.hpp

clean:
	rm -f test *.o

.PHONY: all test format clean
