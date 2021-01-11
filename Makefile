APP=bullsat
CXX := clang++
CXXFLAGS := -std=c++17 -Weverything -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-padded
DEBUGFLAGS := -g -fsanitize=undefined

all: release debug

release: main.cpp bullsat.hpp
	mkdir -p build/release/
	$(CXX) $(CXXFLAGS) -O3 -DNDEBUG -o build/release/$(APP) main.cpp

debug: main.cpp bullsat.hpp
	mkdir -p build/debug/
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o build/debug/$(APP) main.cpp

test: test.cpp bullsat.hpp
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ test.cpp 
	./$@

format:
	clang-format -i *.cpp *.hpp

clean:
	rm -rf build test *.o

.PHONY: all release test format clean
