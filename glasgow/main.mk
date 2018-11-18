BUILD_DIR := intermediate
TARGET_DIR := ./

SUBMAKEFILES := solve.mk solve_mpi.mk

boost_ldlibs := -lboost_thread -lboost_system -lboost_program_options
boost_mpilibs := -lboost_mpi -lboost_serialization

MPICXX := mpicxx
override CXXFLAGS += -O3 -march=native -std=c++17 -I./ -W -Wall -g -ggdb3 -pthread
override LDFLAGS += -pthread
