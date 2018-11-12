/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "result.hh"

using std::string;

auto
Result::merge(const string & prefix, const Result & other) -> void
{
    if (other.isomorphism.size() > isomorphism.size())
        isomorphism = other.isomorphism;

    nodes += other.nodes;
    propagations += other.propagations;
    solution_count += other.solution_count;
    complete = complete || other.complete;

    for (auto & x : other.extra_stats)
        extra_stats.push_back(prefix + x);
}

