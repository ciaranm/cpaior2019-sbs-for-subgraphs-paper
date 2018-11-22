/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "solver.hh"
#include "fixed_bit_set.hh"
#include "template_voodoo.hh"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <thread>
#include <type_traits>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/utility.hpp>

using std::array;
using std::atomic;
using std::condition_variable;
using std::iota;
using std::fill;
using std::find_if;
using std::greater;
using std::is_same;
using std::list;
using std::max;
using std::map;
using std::move;
using std::mt19937;
using std::mutex;
using std::next;
using std::numeric_limits;
using std::optional;
using std::pair;
using std::scoped_lock;
using std::sort;
using std::stable_sort;
using std::string;
using std::swap;
using std::thread;
using std::to_string;
using std::uniform_int_distribution;
using std::unique_lock;
using std::vector;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::operator""ms;

using boost::dynamic_bitset;

using boost::mpi::all_gather;
using boost::mpi::communicator;

namespace
{
    struct barrier
    {
        const unsigned n;
        int w;
        long long g;
        mutex m;
        condition_variable cv;

        barrier(unsigned nn) :
            n(nn),
            w(int(nn)),
            g(0)
        {
        }

        void count_down_and_wait()
        {
            unique_lock<mutex> l(m);
            if (w == -1)
                return;

            if (0 == --w) {
                ++g;
                w = n;
                cv.notify_all();
            }
            else {
                int old_g = g;
                while (true) {
                    cv.wait(l);
                    if (old_g != g || -1 == w)
                        break;
                }
            }
        }

        auto cancel() -> void
        {
            unique_lock<mutex> l(m);
            w = -1;
            cv.notify_all();
        }
    };

    enum class SearchResult
    {
        Aborted,
        Unsatisfiable,
        Satisfiable,
        SatisfiableButKeepGoing,
        Restart
    };

    struct Assignment
    {
        unsigned pattern_vertex;
        unsigned target_vertex;

        auto operator== (const Assignment & other) const -> bool
        {
            return pattern_vertex == other.pattern_vertex && target_vertex == other.target_vertex;
        }

        auto operator!= (const Assignment & other) const -> bool
        {
            return ! (*this == other);
        }

        template <typename Archive_>
        void serialize(Archive_ & ar, const unsigned int) {
            ar & pattern_vertex & target_vertex;
        }
    };

    struct AssignmentInformation
    {
        Assignment assignment;
        bool is_decision;
        int discrepancy_count;
        int choice_count;
    };

    struct Assignments
    {
        vector<AssignmentInformation> values;

        bool contains(const Assignment & assignment) const
        {
            // this should not be a linear scan...
            return values.end() != find_if(values.begin(), values.end(), [&] (const auto & a) {
                    return a.assignment == assignment;
                    });
        }
    };

    // A nogood, aways of the form (list of assignments) -> false, where the
    // last part is implicit. If there are at least two assignments, then the
    // first two assignments are the watches (and the literals are permuted
    // when the watches are updates).
    struct Nogood
    {
        vector<Assignment> literals;

        template <typename Archive_>
        void serialize(Archive_ & ar, const unsigned int) {
            ar & literals;
        }
    };

    // nogoods stored here
    using Nogoods = list<Nogood>;

    // Two watched literals for our nogoods store.
    struct Watches
    {
        // for each watched literal, we have a list of watched things, each of
        // which is an iterator into the global watch list (so we can reorder
        // the literal to keep the watches as the first two elements)
        using WatchList = list<Nogoods::iterator>;

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
            return data[target_size * a.pattern_vertex + a.target_vertex];
        }
    };

    struct SearchData
    {
        Nogoods nogoods, new_nogoods;
        Watches watches;
        unsigned long long nodes = 0;

        mt19937 thread_rand;
    };

    template <typename BitSetType_, typename ArrayType_>
    struct ParallelSIP
    {
        struct Domain
        {
            unsigned v;
            unsigned count;
            bool fixed = false;
            BitSetType_ values;

            explicit Domain(unsigned s) :
                values(s, 0)
            {
            }

            Domain(const Domain &) = default;
            Domain(Domain &&) = default;
        };

        using Domains = vector<Domain>;

        communicator mpi_comm;
        const Params & params;
        const int max_graphs;

        unsigned pattern_size, full_pattern_size, target_size;

        vector<uint8_t> pattern_adjacencies_bits;
        vector<dynamic_bitset<> > pattern_graph_rows;
        vector<BitSetType_> target_graph_rows;

        vector<int> pattern_permutation, isolated_vertices;
        vector<vector<int> > patterns_degrees, targets_degrees;
        int largest_target_degree;

        vector<int> pattern_vertex_labels, target_vertex_labels, pattern_edge_labels, target_edge_labels;

        ParallelSIP(const InputGraph & target, const InputGraph & pattern, const Params & a,
                communicator & c) :
            mpi_comm(c),
            params(a),
            max_graphs(5 + (params.induced ? 1 : 0)),
            pattern_size(pattern.size()),
            full_pattern_size(pattern.size()),
            target_size(target.size()),
            patterns_degrees(max_graphs),
            targets_degrees(max_graphs),
            largest_target_degree(0)
        {
            if (pattern.has_edge_labels() && ! params.induced)
                throw UnsupportedConfiguration{ "Currently edge labels only work with --induced" };

            if (params.enumerate)
                throw UnsupportedConfiguration{ "Parallel enumeration is not currently supported" };

            if (params.dds)
                throw UnsupportedConfiguration{ "Parallel discrepancy search is not supported" };

            // strip out isolated vertices in the pattern, and build pattern_permutation
            for (unsigned v = 0 ; v < full_pattern_size ; ++v)
                if ((! params.induced) && (! params.enumerate) && (0 == pattern.degree(v))) {
                    isolated_vertices.push_back(v);
                    --pattern_size;
                }
                else
                    pattern_permutation.push_back(v);

            // recode pattern to a bit graph
            pattern_graph_rows.resize(pattern_size * max_graphs, dynamic_bitset<>(pattern_size));
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                for (unsigned j = 0 ; j < pattern_size ; ++j)
                    if (pattern.adjacent(pattern_permutation.at(i), pattern_permutation.at(j)))
                        pattern_graph_rows[i * max_graphs + 0].set(j);

            // re-encode and store pattern labels
            map<string, int> vertex_labels_map;
            int next_vertex_label = 0;
            if (pattern.has_vertex_labels()) {
                for (unsigned i = 0 ; i < pattern_size ; ++i) {
                    if (vertex_labels_map.emplace(pattern.vertex_label(pattern_permutation.at(i)), next_vertex_label).second)
                        ++next_vertex_label;
                }

                pattern_vertex_labels.resize(pattern_size);
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    pattern_vertex_labels[pattern_permutation[i]] = vertex_labels_map.find(string{ pattern.vertex_label(pattern_permutation[i]) })->second;
            }

            // re-encode and store edge labels
            map<string, int> edge_labels_map;
            int next_edge_label = 0;
            if (pattern.has_edge_labels()) {
                pattern_edge_labels.resize(pattern_size * pattern_size);
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    for (unsigned j = 0 ; j < pattern_size ; ++j)
                        if (pattern.adjacent(pattern_permutation.at(i), pattern_permutation.at(j))) {
                            auto r = edge_labels_map.emplace(pattern.edge_label(pattern_permutation.at(i), pattern_permutation.at(j)), next_edge_label);
                            if (r.second)
                                ++next_edge_label;
                            pattern_edge_labels[pattern_permutation.at(i) * pattern_size + pattern_permutation.at(j)] = r.first->second;
                        }
            }

            // recode target to a bit graph
            target_graph_rows.resize(target_size * max_graphs, BitSetType_{ target_size, 0 });
            for (auto e = target.begin_edges(), e_end = target.end_edges() ; e != e_end ; ++e)
                if (target.adjacent(e->first.first, e->first.second))
                    target_graph_rows[e->first.first * max_graphs + 0].set(e->first.second);

            // target vertex labels
            if (pattern.has_vertex_labels()) {
                for (unsigned i = 0 ; i < target_size ; ++i) {
                    if (vertex_labels_map.emplace(target.vertex_label(i), next_vertex_label).second)
                        ++next_vertex_label;
                }

                target_vertex_labels.resize(target_size);
                for (unsigned i = 0 ; i < target_size ; ++i)
                    target_vertex_labels[i] = vertex_labels_map.find(string{ target.vertex_label(i) })->second;
            }

            // target edge labels
            if (pattern.has_edge_labels()) {
                target_edge_labels.resize(target_size * target_size);
                for (auto e = target.begin_edges(), e_end = target.end_edges() ; e != e_end ; ++e) {
                    auto r = edge_labels_map.emplace(e->second, next_edge_label);
                    if (r.second)
                        ++next_edge_label;

                    target_edge_labels[e->first.first * target_size + e->first.second] = r.first->second;
                }
            }
        }

        template <typename PossiblySomeOtherBitSetType_>
        auto build_supplemental_graphs(vector<PossiblySomeOtherBitSetType_> & graph_rows, unsigned size) -> void
        {
            vector<vector<unsigned> > path_counts(size, vector<unsigned>(size, 0));

            // count number of paths from w to v (only w >= v, so not v to w)
            for (unsigned v = 0 ; v < size ; ++v) {
                auto nv = graph_rows[v * max_graphs + 0];
                for (auto c = nv.find_first() ; c != decltype(nv)::npos ; c = nv.find_first()) {
                    nv.reset(c);
                    auto nc = graph_rows[c * max_graphs + 0];
                    for (auto w = nc.find_first() ; w != decltype(nc)::npos && w <= v ; w = nc.find_first()) {
                        nc.reset(w);
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

        template <typename PossiblySomeOtherBitSetType_>
        auto build_complement_graphs(vector<PossiblySomeOtherBitSetType_> & graph_rows, unsigned size) -> void
        {
            for (unsigned v = 0 ; v < size ; ++v)
                for (unsigned w = 0 ; w < size ; ++w)
                    if (! graph_rows[v * max_graphs + 0].test(w))
                        graph_rows[v * max_graphs + 5].set(w);
        }

        auto find_unit_domain(Domains & domains) -> typename Domains::iterator
        {
            return find_if(domains.begin(), domains.end(), [] (Domain & d) {
                    return (! d.fixed) && 1 == d.count;
                    });
        }

        auto propagate_watches(SearchData & s, Domains & new_domains, Assignments & assignments, const Assignment & current_assignment) -> bool
        {
            if (params.enumerate)
                return true;

            auto & watches_to_update = s.watches[current_assignment];
            for (auto watch_to_update = watches_to_update.begin() ; watch_to_update != watches_to_update.end() ; ) {
                Nogood & nogood = **watch_to_update;

                // make the first watch the thing we just triggered
                if (nogood.literals[0] != current_assignment)
                    swap(nogood.literals[0], nogood.literals[1]);

                // can we find something else to watch?
                bool success = false;
                for (auto new_literal = next(nogood.literals.begin(), 2) ; new_literal != nogood.literals.end() ; ++new_literal) {
                    if (! assignments.contains(*new_literal)) {
                        // we can watch new_literal instead of current_assignment in this nogood
                        success = true;

                        // move the new watch to be the first item in the nogood
                        swap(nogood.literals[0], *new_literal);

                        // start watching it
                        s.watches[nogood.literals[0]].push_back(*watch_to_update);

                        // remove the current watch, and update the loop iterator
                        watches_to_update.erase(watch_to_update++);

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

                    if (d.v == nogood.literals[1].pattern_vertex) {
                        d.values.reset(nogood.literals[1].target_vertex);
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
        template <int max_graphs_, bool has_edge_labels_>
        auto propagate_adjacency_constraints(Domain & d, const Assignment & current_assignment) -> void
        {
            auto pattern_adjacency_bits = pattern_adjacencies_bits[pattern_size * current_assignment.pattern_vertex + d.v];

            // for each graph pair...
            for (int g = 0 ; g < max_graphs_ ; ++g) {
                // if we're adjacent...
                if (pattern_adjacency_bits & (1u << g)) {
                    // ...then we can only be mapped to adjacent vertices
                    d.values &= target_graph_rows[current_assignment.target_vertex * max_graphs_ + g];
                }
            }

            if constexpr (has_edge_labels_) {
                // if we're adjacent in the original graph, additionally the edge labels need to match up
                if (pattern_adjacency_bits & (1u << 0)) {
                    auto check_d_values = d.values;

                    auto want_forward_label = pattern_edge_labels.at(pattern_size * d.v + current_assignment.pattern_vertex);
                    auto want_reverse_label = pattern_edge_labels.at(pattern_size * current_assignment.pattern_vertex + d.v);
                    for (auto c = check_d_values.find_first() ; c != decltype(check_d_values)::npos ; c = check_d_values.find_first()) {
                        check_d_values.reset(c);

                        auto got_forward_label = target_edge_labels.at(target_size * c + current_assignment.target_vertex);
                        auto got_reverse_label = target_edge_labels.at(target_size * current_assignment.target_vertex + c);

                        if (got_forward_label != want_forward_label || got_reverse_label != want_reverse_label)
                            d.values.reset(c);
                    }
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
                d.values.reset(current_assignment.target_vertex);

                // adjacency
                if (pattern_edge_labels.empty()) {
                    switch (max_graphs) {
                        case 5: propagate_adjacency_constraints<5, false>(d, current_assignment); break;
                        case 6: propagate_adjacency_constraints<6, false>(d, current_assignment); break;

                        default:
                            throw "you forgot to update the ugly max_graphs hack";
                    }
                }
                else {
                    switch (max_graphs) {
                        case 5: propagate_adjacency_constraints<5, true>(d, current_assignment); break;
                        case 6: propagate_adjacency_constraints<6, true>(d, current_assignment); break;

                        default:
                            throw "you forgot to update the ugly max_graphs hack";
                    }
                }

                // we might have removed values
                d.count = d.values.count();
                if (0 == d.count)
                    return false;
            }

            return true;
        }

        auto propagate(SearchData & s, Domains & new_domains, Assignments & assignments) -> bool
        {
            // whilst we've got a unit domain...
            for (typename Domains::iterator branch_domain = find_unit_domain(new_domains) ;
                    branch_domain != new_domains.end() ;
                    branch_domain = find_unit_domain(new_domains)) {
                // what are we assigning?
                Assignment current_assignment = { branch_domain->v, unsigned(branch_domain->values.find_first()) };

                // ok, make the assignment
                branch_domain->fixed = true;
                assignments.values.push_back({ current_assignment, false, -1, -1 });

                // propagate watches
                if (! propagate_watches(s, new_domains, assignments, current_assignment))
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
                            (d.count < result->count) ||
                            (d.count == result->count && patterns_degrees[0][d.v] > patterns_degrees[0][result->v]))
                        result = &d;
            return result;
        }

        auto copy_nonfixed_domains_and_make_assignment(
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
                    new_domains.back().values.reset();
                    new_domains.back().values.set(f_v);
                    new_domains.back().count = 1;
                }
            }
            return new_domains;
        }

        auto post_nogood(
                SearchData & s,
                const Assignments & assignments)
        {
            if (params.enumerate || params.nogood_size_limit == 0)
                return;

            Nogood nogood;

            for (auto & a : assignments.values)
                if (a.is_decision)
                    nogood.literals.emplace_back(a.assignment);

            if (nogood.literals.size() > params.nogood_size_limit)
                return;

            s.new_nogoods.emplace_back(move(nogood));
        }

        auto softmax_shuffle(
                SearchData & s,
                ArrayType_ & branch_v,
                unsigned branch_v_end
                ) -> void
        {
            // repeatedly pick a softmax-biased vertex, move it to the front of branch_v,
            // and then only consider items further to the right in the next iteration.

            // Using floating point here turned out to be way too slow. Fortunately the base
            // of softmax doesn't seem to matter, so we use 2 instead of e, and do everything
            // using bit voodoo.
            auto expish = [largest_target_degree = this->largest_target_degree] (int degree) {
                constexpr int sufficient_space_for_adding_up = numeric_limits<long long>::digits - 18;
                auto shift = max<int>(degree - largest_target_degree + sufficient_space_for_adding_up, 0);
                return 1ll << shift;
            };

            long long total = 0;
            for (unsigned v = 0 ; v < branch_v_end ; ++v)
                total += expish(targets_degrees[0][branch_v[v]]);

            for (unsigned start = 0 ; start < branch_v_end ; ++start) {
                // pick a random number between 1 and total inclusive
                uniform_int_distribution<long long> dist(1, total);
                long long select_score = dist(s.thread_rand);

                // go over the list until we hit the score
                unsigned select_element = start;
                for ( ; select_element + 1 < branch_v_end ; ++select_element) {
                    select_score -= expish(targets_degrees[0][branch_v[select_element]]);
                    if (select_score <= 0)
                        break;
                }

                // move to front
                total -= expish(targets_degrees[0][branch_v[select_element]]);
                swap(branch_v[select_element], branch_v[start]);
            }
        }

        auto degree_sort(
                ArrayType_ & branch_v,
                unsigned branch_v_end,
                bool reverse
                ) -> void
        {
            stable_sort(branch_v.begin(), branch_v.begin() + branch_v_end, [&] (int a, int b) -> bool {
                    return (targets_degrees[0][a] >= targets_degrees[0][b]) ^ reverse;
                    });
        }

        auto restarting_search(
                SearchData & s,
                Assignments & assignments,
                const Domains & domains,
                unsigned long long & nodes,
                unsigned long long & propagations,
                unsigned long long & solution_count,
                int depth,
                long long & backtracks_until_restart,
                steady_clock::time_point * restart_at,
                atomic<bool> & do_a_restart) -> SearchResult
        {
            if (params.abort->load())
                return SearchResult::Aborted;

            ++nodes;

            // find ourselves a domain, or succeed if we're all assigned
            const Domain * branch_domain = find_branch_domain(domains);
            if (! branch_domain) {
                if (params.enumerate) {
                    ++solution_count;
                    return SearchResult::SatisfiableButKeepGoing;
                }
                else
                    return SearchResult::Satisfiable;
            }

            // pull out the remaining values in this domain for branching
            auto remaining = branch_domain->values;

            ArrayType_ branch_v;
            if constexpr (is_same<ArrayType_, vector<int> >::value)
                branch_v.resize(target_size);

            unsigned branch_v_end = 0;
            for (auto f_v = remaining.find_first() ; f_v != decltype(remaining)::npos ; f_v = remaining.find_first()) {
                remaining.reset(f_v);
                branch_v[branch_v_end++] = f_v;
            }

            switch (params.value_ordering_heuristic) {
                case ValueOrdering::Degree:
                    degree_sort(branch_v, branch_v_end, false);
                    break;

                case ValueOrdering::AntiDegree:
                    degree_sort(branch_v, branch_v_end, true);
                    break;

                case ValueOrdering::Biased:
                    softmax_shuffle(s, branch_v, branch_v_end);
                    break;

                case ValueOrdering::Random:
                    shuffle(branch_v.begin(), branch_v.begin() + branch_v_end, s.thread_rand);
                    break;
            }

            int discrepancy_count = 0;
            bool actually_hit_a_success = false, actually_hit_a_failure = false;

            // for each value remaining...
            for (auto f_v = branch_v.begin(), f_end = branch_v.begin() + branch_v_end ; f_v != f_end ; ++f_v) {
                // modified in-place by appending, we can restore by shrinking
                auto assignments_size = assignments.values.size();

                // make the assignment
                assignments.values.push_back({ { branch_domain->v, unsigned(*f_v) }, true, discrepancy_count, int(branch_v_end) });

                // set up new domains
                Domains new_domains = copy_nonfixed_domains_and_make_assignment(domains, branch_domain->v, *f_v);

                // propagate
                ++propagations;
                if (! propagate(s, new_domains, assignments)) {
                    // failure? restore assignments and go on to the next thing
                    assignments.values.resize(assignments_size);
                    actually_hit_a_failure = true;
                    continue;
                }

                // recursive search
                auto search_result = restarting_search(s, assignments, new_domains, nodes, propagations,
                        solution_count, depth + 1, backtracks_until_restart, restart_at, do_a_restart);

                switch (search_result) {
                    case SearchResult::Satisfiable:
                        return SearchResult::Satisfiable;

                    case SearchResult::Aborted:
                        return SearchResult::Aborted;

                    case SearchResult::Restart:
                        // restore assignments before posting nogoods, it's easier
                        assignments.values.resize(assignments_size);

                        // post nogoods for everything we've done so far
                        for (auto l = branch_v.begin() ; l != f_v ; ++l) {
                            assignments.values.push_back({ { branch_domain->v, unsigned(*l) }, true, -2, -2 });
                            post_nogood(s, assignments);
                            assignments.values.pop_back();
                        }

                        return SearchResult::Restart;

                    case SearchResult::SatisfiableButKeepGoing:
                        // restore assignments
                        assignments.values.resize(assignments_size);
                        actually_hit_a_success = true;
                        break;

                    case SearchResult::Unsatisfiable:
                        // restore assignments
                        assignments.values.resize(assignments_size);
                        actually_hit_a_failure = true;
                        break;
                }

                ++discrepancy_count;
            }

            // no values remaining, backtrack, or possibly kick off a restart
            if ((do_a_restart) || (actually_hit_a_failure && ((backtracks_until_restart > 0 && 0 == --backtracks_until_restart)
                            || (restart_at && steady_clock::now() > *restart_at)))) {
                do_a_restart = true;
                post_nogood(s, assignments);
                return SearchResult::Restart;
            }
            else if (actually_hit_a_success)
                return SearchResult::SatisfiableButKeepGoing;
            else
                return SearchResult::Unsatisfiable;
        }

        auto initialise_domains(Domains & domains) -> bool
        {
            int graphs_to_consider = max_graphs;
            if (params.induced) {
                // when looking at the complement graph, if the largest degree
                // in the pattern is smaller than the smallest degree in the
                // target, then we're going to spend a lot of time doing
                // nothing useful
                auto largest_pattern_c_degree = max_element(patterns_degrees[max_graphs - 1].begin(), patterns_degrees[max_graphs - 1].end());
                auto smallest_target_c_degree = min_element(targets_degrees[max_graphs - 1].begin(), targets_degrees[max_graphs - 1].end());
                if (largest_pattern_c_degree != patterns_degrees[max_graphs - 1].end() &&
                        smallest_target_c_degree != targets_degrees[max_graphs - 1].end() &&
                        *largest_pattern_c_degree < *smallest_target_c_degree)
                    --graphs_to_consider;
            }

            /* pattern and target neighbourhood degree sequences */
            vector<vector<vector<int> > > patterns_ndss(graphs_to_consider);
            vector<vector<optional<vector<int> > > > targets_ndss(graphs_to_consider);

            for (int g = 0 ; g < graphs_to_consider ; ++g) {
                patterns_ndss.at(g).resize(pattern_size);
                targets_ndss.at(g).resize(target_size);
            }

            for (int g = 0 ; g < graphs_to_consider ; ++g) {
                for (unsigned i = 0 ; i < pattern_size ; ++i) {
                    auto ni = pattern_graph_rows[i * max_graphs + g];
                    for (auto j = ni.find_first() ; j != decltype(ni)::npos ; j = ni.find_first()) {
                        ni.reset(j);
                        patterns_ndss.at(g).at(i).push_back(patterns_degrees.at(g).at(j));
                    }
                    sort(patterns_ndss.at(g).at(i).begin(), patterns_ndss.at(g).at(i).end(), greater<int>());
                }
            }

            auto need_nds = [&] (int i) {
                if (! targets_ndss.at(0).at(i)) {
                    for (int g = 0 ; g < graphs_to_consider ; ++g) {
                        targets_ndss.at(g).at(i) = vector<int>{};
                        auto ni = target_graph_rows[i * max_graphs + g];
                        for (auto j = ni.find_first() ; j != decltype(ni)::npos ; j = ni.find_first()) {
                            ni.reset(j);
                            targets_ndss.at(g).at(i)->push_back(targets_degrees.at(g).at(j));
                        }
                        sort(targets_ndss.at(g).at(i)->begin(), targets_ndss.at(g).at(i)->end(), greater<int>());
                    }
                }
            };

            for (unsigned i = 0 ; i < pattern_size ; ++i) {
                domains.at(i).v = i;
                domains.at(i).values.reset();

                for (unsigned j = 0 ; j < target_size ; ++j) {
                    bool ok = true;

                    if (! pattern_vertex_labels.empty())
                        if (pattern_vertex_labels[i] != target_vertex_labels[j])
                            ok = false;

                    // check for loops
                    for (int g = 0 ; g < max_graphs && ok ; ++g) {
                        if (pattern_graph_rows[i * max_graphs + g].test(i) && ! target_graph_rows[j * max_graphs + g].test(j)) {
                            ok = false;
                        }
                    }

                    // check degree-like things
                    for (int g = 0 ; g < graphs_to_consider && ok ; ++g) {
                        if (targets_degrees.at(g).at(j) < patterns_degrees.at(g).at(i)) {
                            // not ok, degrees differ
                            ok = false;
                        }
                        else {
                            // full compare of neighbourhood degree sequences
                            need_nds(j);
                            for (unsigned x = 0 ; ok && x < patterns_ndss.at(g).at(i).size() ; ++x) {
                                if (targets_ndss.at(g).at(j)->at(x) < patterns_ndss.at(g).at(i).at(x))
                                    ok = false;
                            }
                        }
                    }

                    if (ok)
                        domains.at(i).values.set(j);
                }

                domains.at(i).count = domains.at(i).values.count();
            }

            BitSetType_ domains_union{ target_size, 0 };
            for (auto & d : domains)
                domains_union |= d.values;

            unsigned domains_union_popcount = domains_union.count();
            if (domains_union_popcount < unsigned(pattern_size))
                return false;

            for (auto & d : domains)
                d.count = d.values.count();

            return true;
        }

        auto cheap_all_different(Domains & domains) -> bool
        {
            // Pick domains smallest first; ties are broken by smallest .v first.
            // For each count p we have a linked list, whose first member is
            // first[p].  The element following x in one of these lists is next[x].
            // Any domain with a count greater than domains.size() is put
            // int the "count==domains.size()" bucket.
            // The "first" array is sized to be able to hold domains.size()+1
            // elements
            ArrayType_ first, next;

            if constexpr (is_same<ArrayType_, vector<int> >::value)
                first.resize(target_size + 1);
            fill(first.begin(), first.begin() + domains.size() + 1, -1);

            if constexpr (is_same<ArrayType_, vector<int> >::value)
                next.resize(target_size);
            fill(next.begin(), next.begin() + domains.size(), -1);

            // Iterate backwards, because we insert elements at the head of
            // lists and we want the sort to be stable
            for (int i = int(domains.size()) - 1 ; i >= 0; --i) {
                unsigned count = domains.at(i).count;
                if (count > domains.size())
                    count = domains.size();
                next.at(i) = first.at(count);
                first.at(count) = i;
            }

            // counting all-different
            BitSetType_ domains_so_far{ target_size, 0 }, hall{ target_size, 0 };
            unsigned neighbours_so_far = 0;

            for (unsigned i = 0 ; i <= domains.size() ; ++i) {
                // iterate over linked lists
                int domain_index = first[i];
                while (domain_index != -1) {
                    auto & d = domains.at(domain_index);

                    d.values &= ~hall;
                    d.count = d.values.count();

                    if (0 == d.count)
                        return false;

                    domains_so_far |= d.values;
                    ++neighbours_so_far;

                    unsigned domains_so_far_popcount = domains_so_far.count();
                    if (domains_so_far_popcount < neighbours_so_far) {
                        return false;
                    }
                    else if (domains_so_far_popcount == neighbours_so_far) {
                        // equivalent to hall=domains_so_far
                        hall |= domains_so_far;
                    }
                    domain_index = next[domain_index];
                }
            }

            return true;
        }

        auto save_result(const Assignments & assignments, Result & result) -> void
        {
            for (auto & a : assignments.values)
                result.isomorphism.emplace(pattern_permutation.at(a.assignment.pattern_vertex), a.assignment.target_vertex);

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
                where.append(" " + to_string(a.discrepancy_count) + "/" + to_string(a.choice_count));
            result.extra_stats.push_back(where);
        }

        auto solve() -> Result
        {
            Result shared_result;

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
                    patterns_degrees.at(g).at(i) = pattern_graph_rows[i * max_graphs + g].count();

                for (unsigned i = 0 ; i < target_size ; ++i)
                    targets_degrees.at(g).at(i) = target_graph_rows[i * max_graphs + g].count();
            }

            for (unsigned i = 0 ; i < target_size ; ++i)
                largest_target_degree = max(largest_target_degree, targets_degrees[0][i]);

            // pattern adjacencies, compressed
            pattern_adjacencies_bits.resize(pattern_size * pattern_size);
            for (int g = 0 ; g < max_graphs ; ++g)
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    for (unsigned j = 0 ; j < pattern_size ; ++j)
                        if (pattern_graph_rows[i * max_graphs + g].test(j))
                            pattern_adjacencies_bits[i * pattern_size + j] |= (1u << g);

            // domains
            Domains top_domains(pattern_size, Domain{ target_size });
            if (! initialise_domains(top_domains)) {
                shared_result.complete = true;
                return shared_result;
            }

            // start search timer
            auto search_start_time = steady_clock::now();

            list<thread> workers;
            mutex shared_result_mutex;
            atomic<bool> do_a_restart{ false };

            vector<SearchData> search_data(params.n_threads);
            for (int t = 0 ; t < params.n_threads ; ++t) {
                if (0 != t || 0 != mpi_comm.rank())
                    search_data[t].thread_rand.seed(t + (mpi_comm.rank() * params.n_threads));
                search_data[t].watches.initialise(pattern_size, target_size);
            }

            Nogoods combined_new_nogoods;

            barrier nogoods_ready_barrier{ unsigned(params.n_threads) },
                    nogoods_communicated_barrier{ unsigned(params.n_threads) },
                    nogoods_shared_barrier{ unsigned(params.n_threads) };

            for (int t = 0 ; t < params.n_threads ; ++t) {
                workers.emplace_back([t, &params = this->params, &s = search_data[t], &all_search_data = search_data,
                        &top_domains, &shared_result, &shared_result_mutex, &nogoods_ready_barrier, &nogoods_communicated_barrier,
                        &nogoods_shared_barrier, &combined_new_nogoods, &do_a_restart, this] () -> void {
                    Result thread_result;
                    Domains domains = top_domains;

                    Assignments assignments;
                    assignments.values.reserve(pattern_size);

                    // do the appropriate search variant
                    bool done = false;
                    list<long long> luby = {{ 1 }};
                    auto current_luby = luby.begin();
                    double current_geometric = params.restarts_constant;
                    unsigned number_of_restarts = 0;
                    bool first_pass = true;

                    // keep going until someone posts an empty nogood, signifying done
                    while (true) {
                        long long backtracks_until_restart;

                        if (params.enumerate || 0 == params.restarts_constant)
                            backtracks_until_restart = -1;
                        else if (params.triggered_restarts && 0 != t)
                            backtracks_until_restart = -1;
                        else if (0.0 != params.geometric_multiplier) {
                            current_geometric *= params.geometric_multiplier;
                            backtracks_until_restart = round(current_geometric);
                        } else {
                            backtracks_until_restart = *current_luby * params.restarts_constant;
                            if (next(current_luby) == luby.end()) {
                                luby.insert(luby.end(), luby.begin(), luby.end());
                                luby.push_back(*luby.rbegin() * 2);
                            }
                            ++current_luby;
                        }

                        ++number_of_restarts;

                        if (! first_pass) {
                            nogoods_ready_barrier.count_down_and_wait();

                            // gather nogoods from other hosts
                            if (0 == t) {
                                combined_new_nogoods.clear();
                                for (auto & ss : all_search_data)
                                    combined_new_nogoods.splice(combined_new_nogoods.end(), ss.new_nogoods);

                                vector<Nogoods> dist_nogoods;
                                all_gather(mpi_comm, combined_new_nogoods, dist_nogoods);

                                for (auto & ng : dist_nogoods)
                                    combined_new_nogoods.splice(combined_new_nogoods.end(), ng);
                            }

                            nogoods_communicated_barrier.count_down_and_wait();

                            // start watching new nogoods from all hosts
                            for (auto & new_nogood : combined_new_nogoods) {
                                // have to copy nogoods not owned by us, they are permuted
                                Nogoods::iterator n = s.nogoods.insert(s.nogoods.end(), new_nogood);

                                if (n->literals.empty()) {
                                    done = true;
                                    break;
                                }
                                else if (1 == n->literals.size()) {
                                    for (auto & d : domains)
                                        if (d.v == n->literals[0].pattern_vertex) {
                                            d.values.reset(n->literals[0].target_vertex);
                                            d.count = d.values.count();
                                            break;
                                        }
                                }
                                else {
                                    s.watches[n->literals[0]].push_back(n);
                                    s.watches[n->literals[1]].push_back(n);
                                }
                            }

                            if (0 == t)
                                do_a_restart = false;

                            nogoods_shared_barrier.count_down_and_wait();

                            // clear our own "need to watch" list
                            s.new_nogoods.clear();
                        }

                        first_pass = false;

                        if (done)
                            break;

                        ++thread_result.propagations;
                        if (propagate(s, domains, assignments)) {
                            auto assignments_copy = assignments;

                            steady_clock::time_point restart_at, * restart_at_ptr = nullptr;
                            if (0ms != params.restart_timer)
                                if ((! params.triggered_restarts) || (0 == t)) {
                                    restart_at = steady_clock::now() + params.restart_timer;
                                    restart_at_ptr = &restart_at;
                                }

                            switch (restarting_search(s, assignments_copy, domains, thread_result.nodes, thread_result.propagations,
                                        thread_result.solution_count, 0, backtracks_until_restart, restart_at_ptr, do_a_restart)) {
                                case SearchResult::Satisfiable:
                                    save_result(assignments_copy, thread_result);
                                    thread_result.extra_stats.emplace_back("stop_reason = satisfiable");
                                    thread_result.complete = true;
                                    done = true;
                                    post_nogood(s, Assignments{});
                                    params.abort->store(true);
                                    break;

                                case SearchResult::SatisfiableButKeepGoing:
                                    thread_result.extra_stats.emplace_back("stop_reason = satisfiable_but_keep_going");
                                    thread_result.complete = true;
                                    done = true;
                                    post_nogood(s, Assignments{});
                                    break;

                                case SearchResult::Unsatisfiable:
                                    thread_result.extra_stats.emplace_back("stop_reason = unsatisfiable");
                                    thread_result.complete = true;
                                    done = true;
                                    params.abort->store(true);
                                    post_nogood(s, Assignments{});
                                    break;

                                case SearchResult::Aborted:
                                    thread_result.extra_stats.emplace_back("stop_reason = aborted");
                                    done = true;
                                    post_nogood(s, Assignments{});
                                    break;

                                case SearchResult::Restart:
                                    break;
                            }
                        }
                        else {
                            thread_result.extra_stats.emplace_back("stop_reason = propagation");
                            thread_result.complete = true;
                            done = true;
                            params.abort->store(true);
                            post_nogood(s, Assignments{});
                        }
                    }

                    if (! params.enumerate)
                        thread_result.extra_stats.emplace_back("restarts = " + to_string(number_of_restarts));

                    thread_result.extra_stats.emplace_back("nogoods_size = " + to_string(s.nogoods.size()));
                    thread_result.extra_stats.emplace_back("nodes = " + to_string(thread_result.nodes));
                    thread_result.extra_stats.emplace_back("propagations = " + to_string(thread_result.nodes));

                    scoped_lock<mutex> result_lock(shared_result_mutex);
                    shared_result.merge("t" + to_string(t) + ".", thread_result);
                });
            }

            for (auto & w : workers)
                w.join();

            shared_result.extra_stats.emplace_back("search_time = " + to_string(
                        duration_cast<milliseconds>(steady_clock::now() - search_start_time).count()));

            return shared_result;
        }

        auto run() -> Result
        {
            if (full_pattern_size > target_size) {
                /* some of our fixed size data structures will throw a hissy
                 * fit. check this early. */
                Result result;
                result.extra_stats.emplace_back("prepresolved = true");
                return result;
            }

            Result result = solve();

            auto mpi_prefix = "h" + to_string(mpi_comm.rank()) + ".";
            for (auto & s : result.extra_stats)
                s = mpi_prefix + s;
            result.extra_stats.emplace_back(mpi_prefix + "nodes" + " = " + to_string(result.nodes));
            result.extra_stats.emplace_back(mpi_prefix + "propagations" + " = " + to_string(result.propagations));

            return result;
        }
    };
}

auto mpi_subgraph_isomorphism(communicator & mpi_comm, const pair<InputGraph, InputGraph> & graphs, const Params & params) -> Result
{
    if (graphs.first.size() > graphs.second.size())
        return Result{ };

    return select_graph_size<ParallelSIP, Result>(AllGraphSizes(), graphs.second, graphs.first, params, mpi_comm);
}

