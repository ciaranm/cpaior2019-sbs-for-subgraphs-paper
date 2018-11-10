all : lad2arg solve_vf3_induced

override CXXFLAGS += -O3 -march=native -std=c++14 -I./ -W -Wall -g -ggdb3 -pthread

lad2arg : lad2arg.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

solve_vf3_induced : solve_vf3_induced.cc
	$(CXX) $(CXXFLAGS) -o $@ $< -I vflib-3.0.3/include/
