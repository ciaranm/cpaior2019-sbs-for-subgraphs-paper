/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CODE_GUARD_PARAMS_HH
#define CODE_GUARD_PARAMS_HH 1

#include <chrono>
#include <atomic>

struct Params
{
    /// If this is set to true, we should abort due to a time limit.
    std::atomic<bool> * abort;

    /// The start time of the algorithm.
    std::chrono::time_point<std::chrono::steady_clock> start_time;

    /// Use dds?
    bool dds = false;

    /// Use ddds?
    bool ddds = false;

    /// Use restarts?
    bool restarts = false;

    /// Use no heuristic?
    bool input_order = false;

    /// Use shuffles? (For science purposes, not real use)
    bool shuffle = false;

    /// Use biased shuffles?
    bool biased_shuffle = false;

    /// Use position shuffle?
    bool position_shuffle = false;

    /// Use antiheuristic? (For science purposes)
    bool antiheuristic = false;

    /// Don't use nogoods?
    bool goods = false;

    /// Induced?
    bool induced = false;

    /// Default chosen by divine revelation
    static constexpr unsigned long long dodgy_default_magic_luby_multiplier = 666;

    /// Multiplier for Luby sequence
    unsigned long long luby_multiplier = dodgy_default_magic_luby_multiplier;

    /// Multiplier for geometric sequence (set to 0 for luby)
    unsigned long long geometric_multiplier = 0.0;
};

#endif
