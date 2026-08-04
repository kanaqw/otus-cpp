.PHONY: all test clean

CXXFLAGS = -std=c++17 -O2 -Wall

all: bin/mapper bin/reducer_mean bin/reducer_variance

bin/mapper: mapper.cpp include/common.hpp
	mkdir -p bin
	g++ $(CXXFLAGS) -o $@ mapper.cpp

bin/reducer_mean: reducer_mean.cpp
	mkdir -p bin
	g++ $(CXXFLAGS) -o $@ reducer_mean.cpp

bin/reducer_variance: reducer_variance.cpp
	mkdir -p bin
	g++ $(CXXFLAGS) -o $@ reducer_variance.cpp

bin/test_common: test_common.cpp include/common.hpp
	mkdir -p bin
	g++ $(CXXFLAGS) -o $@ test_common.cpp

test: bin/test_common
	./bin/test_common

clean:
	rm -rf bin output_mean output_variance
