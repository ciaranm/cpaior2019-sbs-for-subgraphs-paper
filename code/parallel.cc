/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "parallel.hh"
#include "fixed_bit_set.hh"
#include "template_voodoo.hh"

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/thread/barrier.hpp>

using std::array;
using std::atomic;
using std::iota;
using std::fill;
using std::find_if;
using std::get;
using std::greater;
using std::list;
using std::make_pair;
using std::map;
using std::max;
using std::move;
using std::mt19937;
using std::mutex;
using std::next;
using std::pair;
using std::sort;
using std::string;
using std::swap;
using std::to_string;
using std::thread;
using std::tuple;
using std::uniform_int_distribution;
using std::uniform_real_distribution;
using std::unique_lock;
using std::vector;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

using boost::barrier;

namespace
{
    enum class Search
    {
        Aborted,
        Unsatisfiable,
        Satisfiable
    };

    enum class RestartingSearch
    {
        Aborted,
        Unsatisfiable,
        Satisfiable,
        Restart
    };

    using Assignment = pair<unsigned, unsigned>;

    struct Assignments
    {
        vector<tuple<Assignment, bool, int> > values;

        bool contains(const Assignment & assignment) const
        {
            // this should not be a linear scan...
            return values.end() != find_if(values.begin(), values.end(), [&] (const auto & a) {
                    return get<0>(a) == assignment;
                    });
        }
    };

    // A nogood, aways of the form (list of assignments) -> false, where the
    // last part is implicit. Unlike in the sequential algorithm, these are not
    // permuted.
    struct Nogood
    {
        vector<Assignment> literals;
    };

    // nogoods stored here
    using Nogoods = list<Nogood>;

    struct NogoodWithTwoWatches;

    // nogoods with per-thread watch notes stored here
    using NogoodWithTwoWatchesList = list<NogoodWithTwoWatches>;

    struct NogoodWithTwoWatches
    {
        Nogoods::const_iterator nogood;
        Assignment first_watch;
        Assignment second_watch;
    };

    // Two watched literals for our nogoods store.
    struct Watches
    {
        NogoodWithTwoWatchesList nogoods_with_watches;

        // for each watched literal, we have a list of watched things, each of
        // which is an iterator into the global watch list. Unlike in the
        // sequential algorithm, we cannot permute these, so we have some awful
        // indirection to deal with.
        using WatchList = list<NogoodWithTwoWatchesList::iterator>;

        // two dimensional array, indexed by (target_size * p + t)
        vector<WatchList> data;

        unsigned pattern_size = 0, target_size = 0;

        // not a ctor to avoid annoyingness with isolated vertices altering the
        // pattern size
        void initialise(unsigned p, unsigned t)
        {
            pattern_size = p;
            target_size = t;
            data.resize(p * t);
        }

        WatchList & operator[] (const Assignment & a)
        {
            return data[target_size * a.first + a.second];
        }
    };

    template <unsigned n_words_>
    struct SIP
    {
        struct Domain
        {
            unsigned v;
            unsigned popcount;
            bool fixed = false;
            FixedBitSet<n_words_> values;
        };

        using Domains = vector<Domain>;

        const Params & params;
        const int max_graphs;

        unsigned pattern_size, full_pattern_size, target_size;

        vector<uint8_t> pattern_adjacencies_bits;
        vector<FixedBitSet<n_words_> > pattern_graph_rows;
        vector<FixedBitSet<n_words_> > target_graph_rows;

        vector<int> pattern_permutation, target_permutation, isolated_vertices;
        vector<vector<int> > patterns_degrees, targets_degrees;

        vector<unsigned long long> target_vertex_biases;

        mutex new_nogoods_mutex;
        Nogoods nogoods;
        list<typename Nogoods::const_iterator> need_to_watch;

        struct ThreadData
        {
            Watches watches;
            mt19937 rand;
        };

        SIP(const Graph & target, const Graph & pattern, const Params & a) :
            params(a),
            max_graphs(5 + (params.induced ? 1 : 0)),
            pattern_size(pattern.size()),
            full_pattern_size(pattern.size()),
            target_size(target.size()),
            target_permutation(target.size()),
            patterns_degrees(max_graphs),
            targets_degrees(max_graphs)
        {
            // strip out isolated vertices in the pattern, and build pattern_permutation
            for (unsigned v = 0 ; v < full_pattern_size ; ++v)
                if ((! params.induced) && (0 == pattern.degree(v))) {
                    isolated_vertices.push_back(v);
                    --pattern_size;
                }
                else
                    pattern_permutation.push_back(v);

            // recode pattern to a bit graph
            pattern_graph_rows.resize(pattern_size * max_graphs);
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                for (unsigned j = 0 ; j < pattern_size ; ++j)
                    if (pattern.adjacent(pattern_permutation.at(i), pattern_permutation.at(j)))
                        pattern_graph_rows[i * max_graphs + 0].set(j);

            // determine ordering for target graph vertices
            iota(target_permutation.begin(), target_permutation.end(), 0);

            // recode target to a bit graph
            target_graph_rows.resize(target_size * max_graphs);
            for (unsigned i = 0 ; i < target_size ; ++i)
                for (unsigned j = 0 ; j < target_size ; ++j)
                    if (target.adjacent(target_permutation.at(i), target_permutation.at(j)))
                        target_graph_rows[i * max_graphs + 0].set(j);

            // build up vertex selection biases
            unsigned max_degree = 0;
            for (unsigned j = 0 ; j < target_size ; ++j)
                max_degree = max(max_degree, target_graph_rows[j * max_graphs + 0].popcount());

            for (unsigned j = 0 ; j < target_size ; ++j) {
                unsigned degree = target_graph_rows[j * max_graphs + 0].popcount();
                if (max_degree - degree >= 50)
                    target_vertex_biases.push_back(1);
                else
                    target_vertex_biases.push_back(1ull << (50 - (max_degree - degree)));
            }
        }

        auto build_supplemental_graphs(vector<FixedBitSet<n_words_> > & graph_rows, unsigned size) -> void
        {
            vector<vector<unsigned> > path_counts(size, vector<unsigned>(size, 0));

            // count number of paths from w to v (only w >= v, so not v to w)
            for (unsigned v = 0 ; v < size ; ++v) {
                auto nv = graph_rows[v * max_graphs + 0];
                unsigned cp = 0;
                for (int c = nv.first_set_bit_from(cp) ; c != -1 ; c = nv.first_set_bit_from(cp)) {
                    nv.unset(c);
                    auto nc = graph_rows[c * max_graphs + 0];
                    unsigned wp = 0;
                    for (int w = nc.first_set_bit_from(wp) ; w != -1 && w <= int(v) ; w = nc.first_set_bit_from(wp)) {
                        nc.unset(w);
                        ++path_counts[v][w];
                    }
                }
            }

            for (unsigned v = 0 ; v < size ; ++v) {
                for (unsigned w = v ; w < size ; ++w) {
                    // w to v, not v to w, see above
                    unsigned path_count = path_counts[w][v];
                    for (unsigned p = 1 ; p <= 4 ; ++p) {
                        if (path_count >= p) {
                            graph_rows[v * max_graphs + p].set(w);
                            graph_rows[w * max_graphs + p].set(v);
                        }
                    }
                }
            }
        }

        auto build_complement_graphs(vector<FixedBitSet<n_words_> > & graph_rows, unsigned size) -> void
        {
            for (unsigned v = 0 ; v < size ; ++v)
                for (unsigned w = 0 ; w < size ; ++w)
                    if (! graph_rows[v * max_graphs + 0].test(w))
                        graph_rows[v * max_graphs + 5].set(w);
        }

        auto find_unit_domain(Domains & domains) -> typename Domains::iterator
        {
            return find_if(domains.begin(), domains.end(), [] (Domain & d) {
                    return (! d.fixed) && 1 == d.popcount;
                    });
        }

        auto propagate_watches(
                Watches & watches,
                Domains & new_domains,
                Assignments & assignments,
                const Assignment & current_assignment) -> bool
        {
            auto & watches_to_update = watches[current_assignment];
            for (auto watch_to_update = watches_to_update.begin() ; watch_to_update != watches_to_update.end() ; ) {
                const Nogood & nogood = *(*watch_to_update)->nogood;
                Assignment & first_watch = (*watch_to_update)->first_watch;
                Assignment & second_watch = (*watch_to_update)->second_watch;

                // swap if necessary so the current assignment is the first watch
                if (first_watch != current_assignment)
                    swap(first_watch, second_watch);

                // can we find something else to watch?
                bool success = false;
                for (auto new_literal = nogood.literals.begin() ; new_literal != nogood.literals.end() ; ++new_literal) {
                    if (*new_literal == first_watch || *new_literal == second_watch)
                        continue;

                    if (! assignments.contains(*new_literal)) {
                        // we can watch new_literal instead of current_assignment in this nogood
                        success = true;

                        // start watching it
                        watches[*new_literal].push_back(*watch_to_update);

                        // remove the current watch, and update the loop iterator
                        watches_to_update.erase(watch_to_update++);

                        // update the watching assignments
                        first_watch = *new_literal;

                        break;
                    }
                }

                // found something new? nothing to propagate (and we've already updated our loop iterator in the erase)
                if (success)
                    continue;

                // no new watch, this nogood will now propagate. do a linear scan to find the variable for now... note
                // that it might not exist if we've assigned it something other value anyway.
                for (auto & d : new_domains) {
                    if (d.fixed)
                        continue;

                    if (d.v == second_watch.first) {
                        d.values.unset(second_watch.second);
                        break;
                    }
                }

                // step the loop variable, only if we've not already erased it
                ++watch_to_update;
            }

            return true;
        }

        // The max_graphs_ template parameter is so that the for each graph
        // pair loop gets unrolled, which makes an annoyingly large difference
        // to performance. Note that for larger target graphs, half of the
        // total runtime is spent in this function.
        template <int max_graphs_>
        auto propagate_adjacency_constraints(Domain & d, const Assignment & current_assignment) -> void
        {
            auto pattern_adjacency_bits = pattern_adjacencies_bits[pattern_size * current_assignment.first + d.v];

            // for each graph pair...
            for (int g = 0 ; g < max_graphs_ ; ++g) {
                // if we're adjacent...
                if (pattern_adjacency_bits & (1u << g)) {
                    // ...then we can only be mapped to adjacent vertices
                    d.values.intersect_with(target_graph_rows[current_assignment.second * max_graphs_ + g]);
                }
            }
        }

        auto propagate_simple_constraints(Domains & new_domains, const Assignment & current_assignment) -> bool
        {
            // propagate for each remaining domain...
            for (auto & d : new_domains) {
                if (d.fixed)
                    continue;

                // all different
                d.values.unset(current_assignment.second);

                // adjacency
                switch (max_graphs) {
                    case 5: propagate_adjacency_constraints<5>(d, current_assignment); break;
                    case 6: propagate_adjacency_constraints<6>(d, current_assignment); break;

                    default:
                        throw "you forgot to update the ugly max_graphs hack";
                }

                // we might have removed values
                d.popcount = d.values.popcount();
                if (0 == d.popcount)
                    return false;
            }

            return true;
        }

        auto propagate(Watches & watches, Domains & new_domains, Assignments & assignments) -> bool
        {
            // whilst we've got a unit domain...
            for (typename Domains::iterator branch_domain = find_unit_domain(new_domains) ;
                    branch_domain != new_domains.end() ;
                    branch_domain = find_unit_domain(new_domains)) {
                // what are we assigning?
                Assignment current_assignment = { branch_domain->v, branch_domain->values.first_set_bit() };

                // ok, make the assignment
                branch_domain->fixed = true;
                assignments.values.push_back({ { current_assignment.first, current_assignment.second }, false, -1 });

                // propagate watches
                if (! propagate_watches(watches, new_domains, assignments, current_assignment))
                    return false;

                // propagate simple all different and adjacency
                if (! propagate_simple_constraints(new_domains, current_assignment))
                    return false;

                // propagate all different
                if (! cheap_all_different(new_domains))
                    return false;
            }

            return true;
        }

        auto find_branch_domain(const Domains & domains) -> const Domain *
        {
            const Domain * result = nullptr;
            for (auto & d : domains)
                if (! d.fixed)
                    if ((! result) ||
                            (d.popcount < result->popcount) ||
                            (d.popcount == result->popcount && patterns_degrees[0][d.v] > patterns_degrees[0][result->v]))
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

        auto post_nogood(
                const Assignments & assignments)
        {
            Nogood nogood;

            for (auto & a : assignments.values)
                if (get<1>(a))
                    nogood.literals.emplace_back(get<0>(a));

            unique_lock<mutex> guard(new_nogoods_mutex);
            nogoods.emplace_back(move(nogood));
            need_to_watch.emplace_back(prev(nogoods.end()));
        }

        auto parallel_restarting_search(
                ThreadData & my_thread_data,
                Assignments & assignments,
                const Domains & domains,
                unsigned long long & nodes,
                int depth,
                long long & backtracks_until_restart) -> RestartingSearch
        {
            if (params.abort->load())
                return RestartingSearch::Aborted;

            ++nodes;

            // find ourselves a domain, or succeed if we're all assigned
            const Domain * branch_domain = find_branch_domain(domains);
            if (! branch_domain)
                return RestartingSearch::Satisfiable;

            // pull out the remaining values in this domain for branching
            auto remaining = branch_domain->values;

            array<unsigned, n_words_ * bits_per_word + 1> branch_v;
            unsigned branch_v_end = 0;
            for (int f_v = remaining.first_set_bit() ; f_v != -1 ; f_v = remaining.first_set_bit()) {
                remaining.unset(f_v);
                branch_v[branch_v_end++] = f_v;
            }

            // sum up the bias scores of every branch vertex
            unsigned long long remaining_score = 0;
            for (unsigned v = 0 ; v < branch_v_end ; ++v)
                remaining_score += target_vertex_biases[branch_v[v]];

            // now repeatedly pick a biased-random vertex, move it to the front of branch_v,
            // and then only consider items further to the right in the next iteration.
            for (unsigned start = 0 ; start < branch_v_end ; ++start) {
                // pick a random number between 0 and remaining_score inclusive
                uniform_int_distribution<unsigned long long> dist(1, remaining_score);
                unsigned long long select_score = dist(my_thread_data.rand);

                // go over the list until we've used up bias values totalling our
                // random number
                unsigned select_element = start;
                for ( ; select_element < branch_v_end ; ++select_element) {
                    if (select_score <= target_vertex_biases[branch_v[select_element]])
                        break;
                    select_score -= target_vertex_biases[branch_v[select_element]];
                }

                // move to front, and update remaining_score
                remaining_score -= target_vertex_biases[branch_v[select_element]];
                swap(branch_v[select_element], branch_v[start]);
            }

            int discrepancy_count = 0;

            // for each value remaining...
            for (auto f_v = branch_v.begin(), f_end = branch_v.begin() + branch_v_end ; f_v != f_end ; ++f_v) {
                // modified in-place by appending, we can restore by shrinking
                auto assignments_size = assignments.values.size();

                // make the assignment
                assignments.values.push_back({ { branch_domain->v, *f_v }, true, discrepancy_count });

                // set up new domains
                Domains new_domains = prepare_domains(domains, branch_domain->v, *f_v);

                // propagate
                if (! propagate(my_thread_data.watches, new_domains, assignments)) {
                    // failure? restore assignments and go on to the next thing
                    assignments.values.resize(assignments_size);
                    continue;
                }

                // recursive search
                auto search_result = parallel_restarting_search(
                        my_thread_data, assignments, new_domains, nodes, depth + 1, backtracks_until_restart);

                switch (search_result) {
                    case RestartingSearch::Satisfiable:
                        return RestartingSearch::Satisfiable;

                    case RestartingSearch::Aborted:
                        return RestartingSearch::Aborted;

                    case RestartingSearch::Restart:
                        // restore assignments before posting nogoods, it's easier
                        assignments.values.resize(assignments_size);

                        // post nogoods for everything we've done so far
                        for (auto l = branch_v.begin() ; l != f_v ; ++l) {
                            assignments.values.push_back({ { branch_domain->v, *l }, true, -2 });
                            post_nogood(assignments);
                            assignments.values.pop_back();
                        }

                        return RestartingSearch::Restart;

                    case RestartingSearch::Unsatisfiable:
                        // restore assignments
                        assignments.values.resize(assignments_size);
                        break;
                }

                ++discrepancy_count;
            }

            // no values remaining, backtrack, or possibly kick off a restart
            if (backtracks_until_restart > 0 && 0 == --backtracks_until_restart) {
                post_nogood(assignments);
                return RestartingSearch::Restart;
            }
            else
                return RestartingSearch::Unsatisfiable;
        }

        auto initialise_domains(Domains & domains) -> bool
        {
            /* pattern and target neighbourhood degree sequences */
            vector<vector<vector<int> > > patterns_ndss(max_graphs);
            vector<vector<vector<int> > > targets_ndss(max_graphs);

            for (int g = 0 ; g < max_graphs ; ++g) {
                patterns_ndss.at(g).resize(pattern_size);
                targets_ndss.at(g).resize(target_size);
            }

            for (int g = 0 ; g < max_graphs ; ++g) {
                for (unsigned i = 0 ; i < pattern_size ; ++i) {
                    auto ni = pattern_graph_rows[i * max_graphs + g];
                    unsigned np = 0;
                    for (int j = ni.first_set_bit_from(np) ; j != -1 ; j = ni.first_set_bit_from(np)) {
                        ni.unset(j);
                        patterns_ndss.at(g).at(i).push_back(patterns_degrees.at(g).at(j));
                    }
                    sort(patterns_ndss.at(g).at(i).begin(), patterns_ndss.at(g).at(i).end(), greater<int>());
                }

                for (unsigned i = 0 ; i < target_size ; ++i) {
                    auto ni = target_graph_rows[i * max_graphs + g];
                    unsigned np = 0;
                    for (int j = ni.first_set_bit_from(np) ; j != -1 ; j = ni.first_set_bit_from(np)) {
                        ni.unset(j);
                        targets_ndss.at(g).at(i).push_back(targets_degrees.at(g).at(j));
                    }
                    sort(targets_ndss.at(g).at(i).begin(), targets_ndss.at(g).at(i).end(), greater<int>());
                }
            }

            for (unsigned i = 0 ; i < pattern_size ; ++i) {
                domains.at(i).v = i;
                domains.at(i).values.unset_all();

                for (unsigned j = 0 ; j < target_size ; ++j) {
                    bool ok = true;

                    for (int g = 0 ; g < max_graphs ; ++g) {
                        if (pattern_graph_rows[i * max_graphs + g].test(i) && ! target_graph_rows[j * max_graphs + g].test(j)) {
                            // not ok, loops
                            ok = false;
                        }
                        else if (targets_ndss.at(g).at(j).size() < patterns_ndss.at(g).at(i).size()) {
                            // not ok, degrees differ
                            ok = false;
                        }
                        else {
                            // full compare of neighbourhood degree sequences
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
            array<int, n_words_ * bits_per_word + 1> first;
            array<int, n_words_ * bits_per_word> next;
            fill(first.begin(), first.begin() + domains.size() + 1, -1);
            fill(next.begin(), next.begin() + domains.size(), -1);
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
            for (auto & a : assignments.values)
                result.isomorphism.emplace(pattern_permutation.at(get<0>(a).first), target_permutation.at(get<0>(a).second));

            // re-add isolated vertices
            int t = 0;
            for (auto & v : isolated_vertices) {
                while (result.isomorphism.end() != find_if(result.isomorphism.begin(), result.isomorphism.end(),
                            [&t] (const pair<int, int> & p) { return p.second == t; }))
                        ++t;
                result.isomorphism.emplace(v, t);
            }

            string where = "where =";
            for (auto & a : assignments.values)
                where.append(" " + to_string(get<2>(a)));
            result.extra_stats.push_back(where);
        }

        auto run() -> Result
        {
            mutex result_mutex;
            Result result;

            if (full_pattern_size > target_size) {
                /* some of our fixed size data structures will throw a hissy
                 * fit. check this early. */
                return result;
            }

            build_supplemental_graphs(pattern_graph_rows, pattern_size);
            build_supplemental_graphs(target_graph_rows, target_size);

            // build complement graphs
            if (params.induced) {
                build_complement_graphs(pattern_graph_rows, pattern_size);
                build_complement_graphs(target_graph_rows, target_size);
            }

            // pattern and target degrees, including supplemental graphs
            for (int g = 0 ; g < max_graphs ; ++g) {
                patterns_degrees.at(g).resize(pattern_size);
                targets_degrees.at(g).resize(target_size);
            }

            for (int g = 0 ; g < max_graphs ; ++g) {
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    patterns_degrees.at(g).at(i) = pattern_graph_rows[i * max_graphs + g].popcount();

                for (unsigned i = 0 ; i < target_size ; ++i)
                    targets_degrees.at(g).at(i) = target_graph_rows[i * max_graphs + g].popcount();
            }

            // pattern adjacencies, compressed
            pattern_adjacencies_bits.resize(pattern_size * pattern_size);
            for (int g = 0 ; g < max_graphs ; ++g)
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    for (unsigned j = 0 ; j < pattern_size ; ++j)
                        if (pattern_graph_rows[i * max_graphs + g].test(j))
                            pattern_adjacencies_bits[i * pattern_size + j] |= (1u << g);

            // domains
            Domains initial_domains(pattern_size);
            if (! initialise_domains(initial_domains))
                return result;

            // do the actual search
            atomic<bool> done{ false };
            list<long long> luby = {{ 1 }};

            unsigned number_of_threads = params.n_threads;
            if (0 == number_of_threads)
                number_of_threads = thread::hardware_concurrency();

            list<thread> workers;
            vector<ThreadData> thread_data{ number_of_threads };

            for (unsigned thread_number = 0 ; thread_number != number_of_threads ; ++thread_number) {
                thread_data[thread_number].watches.initialise(pattern_size, target_size);
                if (0 != thread_number)
                    thread_data[thread_number].rand.seed(thread_number);
            }

            // start search timer
            auto search_start_time = steady_clock::now();

            barrier luby_ready_barrier(number_of_threads), before_starting_search_barrier(number_of_threads),
                    after_search_barrier(number_of_threads), updated_nogoods_barrier(number_of_threads);

            map<pair<unsigned, unsigned>, milliseconds> thread_search_times;

            for (unsigned thread_number = 0 ; thread_number != number_of_threads ; ++thread_number) {
                workers.emplace_back([thread_number, this, &done, &luby, &initial_domains, &my_thread_data = thread_data[thread_number],
                        &result, &result_mutex, &luby_ready_barrier, &before_starting_search_barrier, &after_search_barrier,
                        &updated_nogoods_barrier, &thread_search_times] () {
                    auto current_luby = luby.begin();
                    unsigned number_of_restarts = 0;

                    // assignments
                    Assignments assignments;
                    assignments.values.reserve(pattern_size);

                    Domains domains = initial_domains;

                    unsigned long long my_nodes = 0;

                    while (! done) {
                        long long backtracks_until_restart = *current_luby * params.luby_multiplier;

                        if (0 == thread_number && next(current_luby) == luby.end()) {
                            luby.insert(luby.end(), luby.begin(), luby.end());
                            luby.push_back(*luby.rbegin() * 2);
                        }

                        luby_ready_barrier.count_down_and_wait();

                        ++current_luby;
                        ++number_of_restarts;

                        // start watching new nogoods. we're not backjumping so this is a bit icky.
                        for (auto & n : need_to_watch) {
                            if (n->literals.empty()) {
                                done = true;
                                break;
                            }
                            else if (1 == n->literals.size()) {
                                for (auto & d : domains)
                                    if (d.v == n->literals[0].first) {
                                        d.values.unset(n->literals[0].second);
                                        d.popcount = d.values.popcount();
                                        break;
                                    }
                            }
                            else {
                                auto p = my_thread_data.watches.nogoods_with_watches.insert(
                                        my_thread_data.watches.nogoods_with_watches.end(),
                                        NogoodWithTwoWatches{ n, n->literals[0], n->literals[1] });
                                my_thread_data.watches[n->literals[0]].push_back(p);
                                my_thread_data.watches[n->literals[1]].push_back(p);
                            }
                        }

                        updated_nogoods_barrier.count_down_and_wait();

                        if (0 == thread_number)
                            need_to_watch.clear();

                        if (done)
                            params.abort->store(true);

                        before_starting_search_barrier.count_down_and_wait();

                        auto work_start_time = steady_clock::now();

                        if ((! done) && propagate(my_thread_data.watches, domains, assignments)) {
                            auto assignments_copy = assignments;

                            switch (parallel_restarting_search(my_thread_data, assignments_copy, domains,
                                        my_nodes, 0, backtracks_until_restart)) {
                                case RestartingSearch::Satisfiable:
                                    {
                                        unique_lock<mutex> guard(result_mutex);
                                        save_result(assignments_copy, result);
                                        params.abort->store(true);
                                    }
                                    done = true;
                                    break;

                                case RestartingSearch::Unsatisfiable:
                                    params.abort->store(true);
                                    done = true;
                                    break;

                                case RestartingSearch::Aborted:
                                    done = true;
                                    break;

                                case RestartingSearch::Restart:
                                    break;
                            }
                        }
                        else
                            done = true;

                        {
                            unique_lock<mutex> guard(result_mutex);
                            thread_search_times.emplace(
                                    make_pair(thread_number, number_of_restarts),
                                    duration_cast<milliseconds>(steady_clock::now() - work_start_time));
                        }

                        after_search_barrier.count_down_and_wait();
                    }

                    {
                        unique_lock<mutex> guard(result_mutex);
                        result.nodes += my_nodes;
                        if (0 == thread_number) {
                            result.extra_stats.emplace_back("restarts = " + to_string(number_of_restarts));
                        }
                    }
                });
            }

            for (auto & t : workers)
                t.join();

            result.extra_stats.emplace_back("search_time = " + to_string(
                        duration_cast<milliseconds>(steady_clock::now() - search_start_time).count()));
            result.extra_stats.emplace_back("nogoods_size = " + to_string(nogoods.size()));

            {
                map<string, string> thread_search_times_strings;
                for (auto & t : thread_search_times)
                    thread_search_times_strings[to_string(t.first.second)].append(" " + to_string(t.second.count()));
                for (auto & t : thread_search_times_strings)
                    result.extra_stats.emplace_back("thread_seach_times_" + t.first + " =" + t.second);
            }

            map<int, int> nogoods_lengths;
            for (auto & n : nogoods)
                nogoods_lengths[n.literals.size()]++;

            string nogoods_lengths_str;
            for (auto & n : nogoods_lengths) {
                nogoods_lengths_str += " ";
                nogoods_lengths_str += to_string(n.first) + ":" + to_string(n.second);
            }
            result.extra_stats.emplace_back("nogoods_lengths =" + nogoods_lengths_str);

            return result;
        }
    };
}

auto parallel_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    if (graphs.first.size() > graphs.second.size())
        return Result{ };

    return select_graph_size<SIP, Result>(AllGraphSizes(), graphs.second, graphs.first, params);
}

