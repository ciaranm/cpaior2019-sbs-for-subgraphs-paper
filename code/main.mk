BUILD_DIR := intermediate
TARGET_DIR := ./

boost_ldlibs := -lboost_regex -lboost_thread -lboost_system -lboost_program_options

override CXXFLAGS += -O3 -march=native -std=c++17 -I./ -W -Wall -g -ggdb3 -pthread
override LDFLAGS += -pthread

TARGET := solve_subgraph_isomorphism

SOURCES := \
    unit.cc \
    fixed_bit_set.cc \
    graph.cc \
    lad.cc \
    solve_subgraph_isomorphism.cc

TGT_LDLIBS := $(boost_ldlibs)

