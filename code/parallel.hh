/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef GUARD_PARALLEL_HH
#define GUARD_PARALLEL_HH 1

#include "params.hh"
#include "result.hh"
#include "graph.hh"

auto parallel_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const Params & params) -> Result;

#endif
