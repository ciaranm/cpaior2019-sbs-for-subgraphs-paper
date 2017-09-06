/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "degree_sort.hh"

#include <vector>
#include <algorithm>

auto degree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void
{
    // pre-calculate degrees
    std::vector<std::pair<int, int> > degrees;

    for (int v = 0 ; v < graph.size() ; ++v)
        degrees.push_back(std::make_pair(graph.degree(v), 0));

    for (int v = 0 ; v < graph.size() ; ++v)
        for (int w = 0 ; w < graph.size() ; ++w)
            if (graph.adjacent(v, w))
                degrees.at(v).second += degrees.at(w).first;

    // sort on degree
    std::sort(p.begin(), p.end(),
            [&] (int a, int b) { return (! reverse) ^ (degrees[a] < degrees[b] || (degrees[a] == degrees[b] && a > b)); });
}

