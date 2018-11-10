/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef GLASGOW_SUBGRAPH_SOLVER_PARAMS_HH
#define GLASGOW_SUBGRAPH_SOLVER_PARAMS_HH 1

#include <chrono>
#include <atomic>
#include <cmath>
#include <limits>

#include "value_ordering.hh"

struct Params
{
    /// If this is set to true, we should abort due to a time limit.
    std::atomic<bool> * abort;

    /// The start time of the algorithm.
    std::chrono::time_point<std::chrono::steady_clock> start_time;

    /// Induced?
    bool induced = false;

    /// Enumerate?
    bool enumerate = false;

    /// Presolve?
    bool presolve = false;

    /// Which value-ordering heuristic?
    ValueOrdering value_ordering_heuristic = ValueOrdering::Biased;

    /// Default chosen by SMAC
    static constexpr unsigned long long dodgy_default_magic_constant_restart_multiplier = 660;

    /// Constant multiplier for restarts sequence (0 disables restarts)
    unsigned long long restarts_constant = dodgy_default_magic_constant_restart_multiplier;

    /// Largest size of nogood to store (0 disables nogoods)
    unsigned nogood_size_limit = std::numeric_limits<unsigned>::max();
};

#endif
