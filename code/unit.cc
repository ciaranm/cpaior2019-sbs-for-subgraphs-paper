/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "unit.hh"
#include "bit_graph.hh"
#include "template_voodoo.hh"

#include <algorithm>
#include <numeric>
#include <limits>
#include <random>
#include <iostream>

namespace
{
    enum class Search
    {
        Aborted,
        Unsatisfiable,
        Satisfiable
    };

    auto tiebreaking_degree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void
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

    template <unsigned n_words_, int k_, int l_>
    struct SequentialSubgraphIsomorphism
    {
        struct Domain
        {
            unsigned v;
            unsigned popcount;
            bool fixed = false;
            FixedBitSet<n_words_> values;
        };

        using Domains = std::vector<Domain>;
        using Assignments = std::vector<std::pair<unsigned, unsigned> >;

        const Params & params;

        unsigned pattern_size, full_pattern_size, target_size;

        static constexpr int max_graphs = 1 + (l_ - 1) * k_;
        std::vector<FixedBitGraph<n_words_> > target_graphs;
        std::vector<FixedBitGraph<n_words_> > pattern_graphs;

        std::vector<int> pattern_order, target_order, isolated_vertices;
        std::vector<std::pair<int, int> > pattern_degree_tiebreak;

        SequentialSubgraphIsomorphism(const Graph & target, const Graph & pattern, const Params & a) :
            params(a),
            pattern_size(pattern.size()),
            full_pattern_size(pattern.size()),
            target_size(target.size()),
            target_graphs(max_graphs),
            pattern_graphs(max_graphs),
            target_order(target.size()),
            pattern_degree_tiebreak(pattern_size)
        {
            // strip out isolated vertices in the pattern
            for (unsigned v = 0 ; v < full_pattern_size ; ++v)
                if (0 == pattern.degree(v)) {
                    isolated_vertices.push_back(v);
                    --pattern_size;
                }
                else
                    pattern_order.push_back(v);

            // recode pattern to a bit graph
            pattern_graphs.at(0).resize(pattern_size);
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                for (unsigned j = 0 ; j < pattern_size ; ++j)
                    if (pattern.adjacent(pattern_order.at(i), pattern_order.at(j)))
                        pattern_graphs.at(0).add_edge(i, j);

            // determine ordering for target graph vertices
            std::iota(target_order.begin(), target_order.end(), 0);
            tiebreaking_degree_sort(target, target_order, false);

            // recode target to a bit graph
            target_graphs.at(0).resize(target_size);
            for (unsigned i = 0 ; i < target_size ; ++i)
                for (unsigned j = 0 ; j < target_size ; ++j)
                    if (target.adjacent(target_order.at(i), target_order.at(j)))
                        target_graphs.at(0).add_edge(i, j);

            for (unsigned j = 0 ; j < pattern_size ; ++j)
                pattern_degree_tiebreak.at(j).first = pattern_graphs.at(0).degree(j);
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                for (unsigned j = 0 ; j < pattern_size ; ++j)
                    if (pattern_graphs.at(0).adjacent(i, j))
                        pattern_degree_tiebreak.at(j).second += pattern_degree_tiebreak.at(i).first;
        }

        auto build_supplemental_graphs(std::vector<FixedBitGraph<n_words_> > & graphs, unsigned size) -> void
        {
            for (int g = 1 ; g < max_graphs ; ++g)
                graphs.at(g).resize(size);

            if (l_ >= 2) {
                for (unsigned v = 0 ; v < size ; ++v) {
                    auto nv = graphs.at(0).neighbourhood(v);
                    for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                        nv.unset(c);
                        auto nc = graphs.at(0).neighbourhood(c);
                        for (int w = nc.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nc.first_set_bit()) {
                            nc.unset(w);
                            if (k_ >= 5 && graphs.at(4).adjacent(v, w))
                                graphs.at(5).add_edge(v, w);
                            else if (k_ >= 4 && graphs.at(3).adjacent(v, w))
                                graphs.at(4).add_edge(v, w);
                            else if (k_ >= 3 && graphs.at(2).adjacent(v, w))
                                graphs.at(3).add_edge(v, w);
                            else if (k_ >= 2 && graphs.at(1).adjacent(v, w))
                                graphs.at(2).add_edge(v, w);
                            else if (k_ >= 1)
                                graphs.at(1).add_edge(v, w);
                        }
                    }
                }
            }
        }

        auto find_unit_domain(Domains & domains) -> typename Domains::iterator
        {
            return std::find_if(domains.begin(), domains.end(), [] (Domain & d) {
                    return (! d.fixed) && 1 == d.popcount;
                    });
        }

        auto propagate(Domains & new_domains, Assignments & assignments) -> bool
        {
            for (typename Domains::iterator branch_domain = find_unit_domain(new_domains) ;
                    branch_domain != new_domains.end() ;
                    branch_domain = find_unit_domain(new_domains)) {
                int branch_v = branch_domain->v;
                int f_v = branch_domain->values.first_set_bit();
                branch_domain->fixed = true;

                assignments.emplace_back(branch_v, f_v);

                // propagate for each remaining domain...
                for (auto & d : new_domains) {
                    if (d.fixed)
                        continue;

                    // all different
                    d.values.unset(f_v);

                    // for each graph pair...
                    for (int g = 0 ; g < max_graphs ; ++g) {
                        // if we're adjacent...
                        if (pattern_graphs.at(g).adjacent(branch_v, d.v)) {
                            // ...then we can only be mapped to adjacent vertices
                            target_graphs.at(g).intersect_with_row(f_v, d.values);
                        }
                    }

                    // we might have removed values
                    d.popcount = d.values.popcount();
                    if (0 == d.popcount)
                        return false;
                }
            }

            if (! cheap_all_different(new_domains))
                return false;

            return true;
        }

        auto find_branch_domain(const Domains & domains) -> const Domain *
        {
            const Domain * result = nullptr;
            for (auto & d : domains)
                if (! d.fixed)
                    if ((! result) ||
                            (d.popcount < result->popcount) ||
                            (d.popcount == result->popcount && pattern_degree_tiebreak.at(d.v) > pattern_degree_tiebreak.at(result->v)))
                        result = &d;
            return result;
        }

        auto prepare_domains(
                const Domains & domains,
                unsigned branch_v,
                unsigned f_v) -> Domains
        {
            Domains new_domains;
            new_domains.reserve(domains.size());
            for (auto & d : domains) {
                if (d.fixed)
                    continue;

                new_domains.push_back(d);
                if (d.v == branch_v) {
                    new_domains.back().values.unset_all();
                    new_domains.back().values.set(f_v);
                    new_domains.back().popcount = 1;
                }
            }
            return new_domains;
        }

        auto search(
                Assignments & assignments,
                const Domains & domains,
                unsigned long long & nodes,
                int depth) -> Search
        {
            if (params.abort->load())
                return Search::Aborted;

            ++nodes;

            const Domain * branch_domain = find_branch_domain(domains);
            if (! branch_domain)
                return Search::Satisfiable;

            auto remaining = branch_domain->values;

            for (int f_v = remaining.first_set_bit() ; f_v != -1 ; f_v = remaining.first_set_bit()) {
                remaining.unset(f_v);

                auto assignments_size = assignments.size();

                /* set up new domains */
                Domains new_domains = prepare_domains(domains, branch_domain->v, f_v);

                if (propagate(new_domains, assignments)) {
                    auto search_result = search(assignments, new_domains, nodes, depth + 1);

                    switch (search_result) {
                        case Search::Satisfiable:    return Search::Satisfiable;
                        case Search::Aborted:        return Search::Aborted;
                        case Search::Unsatisfiable:  break;
                    }
                }

                assignments.resize(assignments_size);
            }

            return Search::Unsatisfiable;
        }

        auto dds_search(
                Assignments & assignments,
                const Domains & domains,
                unsigned long long & nodes,
                int depth,
                int discrepancies_allowed) -> Search
        {
            if (params.abort->load())
                return Search::Aborted;

            ++nodes;

            const Domain * branch_domain = find_branch_domain(domains);
            if (! branch_domain)
                return Search::Satisfiable;

            auto remaining = branch_domain->values;

            for (int f_v = remaining.first_set_bit(), count = 0 ; f_v != -1 ; f_v = remaining.first_set_bit(), ++count) {
                remaining.unset(f_v);

                if ((0 == discrepancies_allowed && 0 == count)
                        || (1 == discrepancies_allowed && 0 != count)
                        || (discrepancies_allowed > 1)) {
                    auto assignments_size = assignments.size();

                    /* set up new domains */
                    Domains new_domains = prepare_domains(domains, branch_domain->v, f_v);

                    if (propagate(new_domains, assignments)) {
                        auto search_result = dds_search(assignments, new_domains, nodes, depth + 1, discrepancies_allowed > 1 ? discrepancies_allowed - 1 : 0);

                        switch (search_result) {
                            case Search::Satisfiable:    return Search::Satisfiable;
                            case Search::Aborted:        return Search::Aborted;
                            case Search::Unsatisfiable:  break;
                        }
                    }

                    assignments.resize(assignments_size);
                }
            }

            return Search::Unsatisfiable;
        }

        auto initialise_domains(Domains & domains) -> bool
        {
            std::vector<std::vector<int> > patterns_degrees(max_graphs);
            std::vector<std::vector<int> > targets_degrees(max_graphs);

            for (int g = 0 ; g < max_graphs ; ++g) {
                patterns_degrees.at(g).resize(pattern_size);
                targets_degrees.at(g).resize(target_size);
            }

            /* pattern and target degree sequences */
            for (int g = 0 ; g < max_graphs ; ++g) {
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    patterns_degrees.at(g).at(i) = pattern_graphs.at(g).degree(i);

                for (unsigned i = 0 ; i < target_size ; ++i)
                    targets_degrees.at(g).at(i) = target_graphs.at(g).degree(i);
            }

            /* pattern and target neighbourhood degree sequences */
            std::vector<std::vector<std::vector<int> > > patterns_ndss(max_graphs);
            std::vector<std::vector<std::vector<int> > > targets_ndss(max_graphs);

            for (int g = 0 ; g < max_graphs ; ++g) {
                patterns_ndss.at(g).resize(pattern_size);
                targets_ndss.at(g).resize(target_size);
            }

            for (int g = 0 ; g < max_graphs ; ++g) {
                for (unsigned i = 0 ; i < pattern_size ; ++i) {
                    for (unsigned j = 0 ; j < pattern_size ; ++j) {
                        if (pattern_graphs.at(g).adjacent(i, j))
                            patterns_ndss.at(g).at(i).push_back(patterns_degrees.at(g).at(j));
                    }
                    std::sort(patterns_ndss.at(g).at(i).begin(), patterns_ndss.at(g).at(i).end(), std::greater<int>());
                }

                for (unsigned i = 0 ; i < target_size ; ++i) {
                    for (unsigned j = 0 ; j < target_size ; ++j) {
                        if (target_graphs.at(g).adjacent(i, j))
                            targets_ndss.at(g).at(i).push_back(targets_degrees.at(g).at(j));
                    }
                    std::sort(targets_ndss.at(g).at(i).begin(), targets_ndss.at(g).at(i).end(), std::greater<int>());
                }
            }

            for (unsigned i = 0 ; i < pattern_size ; ++i) {
                domains.at(i).v = i;
                domains.at(i).values.unset_all();

                for (unsigned j = 0 ; j < target_size ; ++j) {
                    bool ok = true;

                    for (int g = 0 ; g < max_graphs ; ++g) {
                        if (pattern_graphs.at(g).adjacent(i, i) && ! target_graphs.at(g).adjacent(j, j))
                            ok = false;
                        else if (targets_ndss.at(g).at(j).size() < patterns_ndss.at(g).at(i).size())
                            ok = false;
                        else {
                            for (unsigned x = 0 ; ok && x < patterns_ndss.at(g).at(i).size() ; ++x) {
                                if (targets_ndss.at(g).at(j).at(x) < patterns_ndss.at(g).at(i).at(x))
                                    ok = false;
                            }
                        }

                        if (! ok)
                            break;
                    }

                    if (ok)
                        domains.at(i).values.set(j);
                }

                domains.at(i).popcount = domains.at(i).values.popcount();
            }

            FixedBitSet<n_words_> domains_union;
            for (auto & d : domains)
                domains_union.union_with(d.values);

            unsigned domains_union_popcount = domains_union.popcount();
            if (domains_union_popcount < unsigned(pattern_size))
                return false;

            for (auto & d : domains)
                d.popcount = d.values.popcount();

            return true;
        }

        auto cheap_all_different(Domains & domains) -> bool
        {
            // Pick domains smallest first; ties are broken by smallest .v first.
            // For each popcount p we have a linked list, whose first member is
            // first[p].  The element following x in one of these lists is next[x].
            // Any domain with a popcount greater than domains.size() is put
            // int the "popcount==domains.size()" bucket.
            // The "first" array is sized to be able to hold domains.size()+1
            // elements
            std::array<int, n_words_ * bits_per_word + 1> first;
            std::array<int, n_words_ * bits_per_word> next;
            std::fill(first.begin(), first.begin() + domains.size() + 1, -1);
            std::fill(next.begin(), next.begin() + domains.size(), -1);
            // Iterate backwards, because we insert elements at the head of
            // lists and we want the sort to be stable
            for (int i = int(domains.size()) - 1 ; i >= 0; --i) {
                unsigned popcount = domains.at(i).popcount;
                if (popcount > domains.size())
                    popcount = domains.size();
                next.at(i) = first.at(popcount);
                first.at(popcount) = i;
            }

            // counting all-different
            FixedBitSet<n_words_> domains_so_far, hall;
            unsigned neighbours_so_far = 0;

            for (unsigned i = 0 ; i <= domains.size() ; ++i) {
                // iterate over linked lists
                int domain_index = first[i];
                while (domain_index != -1) {
                    auto & d = domains.at(domain_index);

                    d.values.intersect_with_complement(hall);
                    d.popcount = d.values.popcount();

                    if (0 == d.popcount)
                        return false;

                    domains_so_far.union_with(d.values);
                    ++neighbours_so_far;

                    unsigned domains_so_far_popcount = domains_so_far.popcount();
                    if (domains_so_far_popcount < neighbours_so_far) {
                        return false;
                    }
                    else if (domains_so_far_popcount == neighbours_so_far) {
                        // equivalent to hall=domains_so_far
                        hall.union_with(domains_so_far);
                    }
                    domain_index = next[domain_index];
                }
            }

            return true;
        }

        auto save_result(const Assignments & assignments, Result & result) -> void
        {
            for (auto & a : assignments)
                result.isomorphism.emplace(pattern_order.at(a.first), target_order.at(a.second));

            int t = 0;
            for (auto & v : isolated_vertices) {
                while (result.isomorphism.end() != std::find_if(result.isomorphism.begin(), result.isomorphism.end(),
                            [&t] (const std::pair<int, int> & p) { return p.second == t; }))
                        ++t;
                result.isomorphism.emplace(v, t);
            }
        }

        auto run() -> Result
        {
            Result result;

            if (full_pattern_size > target_size) {
                /* some of our fixed size data structures will throw a hissy
                 * fit. check this early. */
                return result;
            }

            build_supplemental_graphs(pattern_graphs, pattern_size);
            build_supplemental_graphs(target_graphs, target_size);

            Domains domains(pattern_size);

            if (! initialise_domains(domains))
                return result;

            Assignments assignments;
            assignments.reserve(pattern_size);
            if (propagate(domains, assignments)) {
                if (params.dds) {
                    for (unsigned discrepancies_allowed = 0 ; discrepancies_allowed <= pattern_size ; ++discrepancies_allowed)
                        if (dds_search(assignments, domains, result.nodes, 0, discrepancies_allowed) == Search::Satisfiable) {
                            save_result(assignments, result);
                            break;
                        }
                }
                else {
                    if (search(assignments, domains, result.nodes, 0) == Search::Satisfiable)
                        save_result(assignments, result);
                }
            }
            return result;
        }
    };

    template <template <unsigned, int, int> class SGI_, int n_, int m_>
    struct Apply
    {
        template <unsigned size_, typename> using Type = SGI_<size_, n_, m_>;
    };
}

auto unit_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    if (graphs.first.size() > graphs.second.size())
        return Result{ };
    return select_graph_size<Apply<SequentialSubgraphIsomorphism, 4, 2>::template Type, Result>(
            AllGraphSizes(), graphs.second, graphs.first, params);
}

