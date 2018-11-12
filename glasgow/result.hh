/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef GLASGOW_SUBGRAPH_SOLVER_RESULT_HH
#define GLASGOW_SUBGRAPH_SOLVER_RESULT_HH 1

#include <chrono>
#include <list>
#include <map>
#include <string>

struct Result
{
    /// The isomorphism, empty if none found.
    std::map<int, int> isomorphism;

    /// Total number of nodes processed (recursive calls).
    unsigned long long nodes = 0;

    /// Number of times propagate called.
    unsigned long long propagations = 0;

    /// Extra stats, to output
    std::list<std::string> extra_stats;

    /// Number of solutions, only if enumerating
    unsigned long long solution_count = 0;

    /// Did we perform a complete search?
    bool complete = false;

    /// Merge contents of another result
    auto merge(const std::string & prefix, const Result & other) -> void;
};

#endif
