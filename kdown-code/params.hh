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

    bool induced = false;

    bool restarts = false;

    /// Default chosen by divine revelation
    static constexpr unsigned long long dodgy_default_magic_luby_multiplier = 660;

    /// Multiplier for Luby sequence
    unsigned long long luby_multiplier = dodgy_default_magic_luby_multiplier;

    unsigned except = 0;
};

#endif
