TARGET := solve_subgraph_isomorphism

SOURCES := \
    sip.cc \
    fixed_bit_set.cc \
    graph.cc \
    lad.cc \
    dimacs.cc \
    graph_file_error.cc \
    solve_subgraph_isomorphism.cc

TGT_LDLIBS := $(boost_ldlibs)

