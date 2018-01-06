/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "sequential.hh"

#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

#include <iostream>

#include <boost/dynamic_bitset.hpp>

using std::find_if;
using std::get;
using std::greater;
using std::list;
using std::make_pair;
using std::map;
using std::max;
using std::move;
using std::mt19937;
using std::numeric_limits;
using std::pair;
using std::swap;
using std::to_string;
using std::tuple;
using std::uniform_int_distribution;
using std::unique;
using std::vector;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

using std::cout;
using std::endl;

namespace
{
    /// We'll use an array of unsigned long longs to represent our bits.
    using BitWord = unsigned long long;

    /// Number of bits per word.
    static const constexpr int bits_per_word = sizeof(BitWord) * 8;

    /**
     * A bitset with a fixed maximum size. This only provides the operations
     * we actually use in the bitset algorithms: it's more readable this way
     * than doing all the bit voodoo inline.
     *
     * Indices start at 0.
     */
    template <unsigned words_>
    class FixedBitSet
    {
        private:
            using Bits = std::array<BitWord, words_>;

            Bits _bits = {{ }};

        public:
            FixedBitSet() = default;

            FixedBitSet(unsigned size)
            {
                assert(size <= words_ * bits_per_word);
            }

            FixedBitSet(const FixedBitSet &) = default;

            FixedBitSet & operator= (const FixedBitSet &) = default;

            /**
             * Set a given bit 'on'.
             */
            auto set(int a) -> void
            {
                // The 1 does have to be of type BitWord. If we just specify a
                // literal, it ends up being an int, and it isn't converted
                // upwards until after the shift is done.
                _bits[a / bits_per_word] |= (BitWord{ 1 } << (a % bits_per_word));
            }

            /**
             * Set a given bit 'off'.
             */
            auto reset(int a) -> void
            {
                _bits[a / bits_per_word] &= ~(BitWord{ 1 } << (a % bits_per_word));
            }

            auto reset() -> void
            {
                for (auto & p : _bits)
                    p = 0;
            }

            /**
             * Is a given bit on?
             */
            auto operator[] (int a) const -> bool
            {
                return _bits[a / bits_per_word] & (BitWord{ 1 } << (a % bits_per_word));
            }

            /**
             * How many bits are on?
             */
            auto count() const -> unsigned
            {
                unsigned result = 0;
                for (auto & p : _bits)
                    result += __builtin_popcountll(p);
                return result;
            }

            /**
             * Are any bits on?
             */
            auto none() const -> bool
            {
                for (auto & p : _bits)
                    if (0 != p)
                        return false;
                return true;
            }

            /**
             * Intersect (bitwise-and) with another set.
             */
            auto operator&= (const FixedBitSet<words_> & other) -> FixedBitSet &
            {
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    _bits[i] = _bits[i] & other._bits[i];
                return *this;
            }

            auto operator& (const FixedBitSet & other) const -> FixedBitSet
            {
                FixedBitSet result;
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    result._bits[i] = _bits[i] & other._bits[i];
                return result;
            }

            /**
             * Union (bitwise-or) with another set.
             */
            auto operator|= (const FixedBitSet<words_> & other) -> FixedBitSet &
            {
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    _bits[i] = _bits[i] | other._bits[i];
                return *this;
            }

            auto operator| (const FixedBitSet & other) const -> FixedBitSet
            {
                FixedBitSet result;
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    result._bits[i] = _bits[i] | other._bits[i];
                return result;
            }

            static const constexpr unsigned npos = numeric_limits<unsigned>::max();

            /**
             * Return the index of the first set ('on') bit, or -1 if we are
             * empty.
             */
            auto find_first() const -> unsigned
            {
                for (typename Bits::size_type i = 0 ; i < _bits.size() ; ++i) {
                    int b = __builtin_ffsll(_bits[i]);
                    if (0 != b)
                        return i * bits_per_word + b - 1;
                }
                return npos;
            }

            auto find_next(unsigned start_after) const -> unsigned
            {
                unsigned start = start_after + 1;

                auto word = _bits[start / bits_per_word];
                word &= ~((BitWord(1) << (start % bits_per_word)) - 1);
                int b = __builtin_ffsll(word);
                if (0 != b)
                    return start / bits_per_word * bits_per_word + b - 1;

                for (typename Bits::size_type i = start / bits_per_word + 1; i < _bits.size() ; ++i) {
                    int b = __builtin_ffsll(_bits[i]);
                    if (0 != b)
                        return i * bits_per_word + b - 1;
                }

                return npos;
            }

            auto operator== (const FixedBitSet<words_> & other) const -> bool
            {
                if (_bits.size() != other._bits.size())
                    return false;

                for (typename Bits::size_type i = 0 ; i < _bits.size() ; ++i)
                    if (_bits[i] != other._bits[i])
                        return false;

                return true;
            }

            auto operator~ () const -> FixedBitSet
            {
                FixedBitSet result = *this;
                for (auto & p : result._bits)
                    p = ~p;
                return result;
            }
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

        bool contains(const Assignment & assignment, unsigned wildcard_start) const
        {
            // this should not be a linear scan...
            return values.end() != find_if(values.begin(), values.end(), [&] (const auto & a) {
                    if (get<0>(a).first != assignment.first)
                        return false;
                    if (get<0>(a).second >= wildcard_start)
                        return assignment.second >= wildcard_start;
                    else
                        return get<0>(a).second == assignment.second;
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
            return data[target_size * a.first + a.second];
        }
    };

    template <typename Bitset_>
    struct SIP
    {
        struct Domain
        {
            unsigned v;
            bool fixed;
            Bitset_ values;
        };

        using Domains = vector<Domain>;

        const Params & params;
        unsigned domain_size;

        Result result;

        list<pair<vector<Bitset_>, vector<Bitset_> > > adjacency_constraints;
        vector<unsigned> pattern_degrees, target_degrees;
        unsigned largest_target_degree;

        Domains initial_domains;

        unsigned wildcard_start;
        Bitset_ all_wildcards;

        Nogoods nogoods;
        Watches watches;
        list<typename Nogoods::iterator> need_to_watch;

        mt19937 global_rand;

        SIP(const Params & k, const Graph & pattern, const Graph & target) :
            params(k),
            domain_size(target.size() + params.except),
            pattern_degrees(pattern.size()),
            target_degrees(domain_size),
            largest_target_degree(0),
            initial_domains(pattern.size()),
            wildcard_start(target.size()),
            all_wildcards(domain_size)
        {
            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                all_wildcards.set(v);

            // build up adjacency bitsets
            add_adjacency_constraints(pattern, target);

            for (unsigned p = 0 ; p < pattern.size() ; ++p)
                pattern_degrees[p] = pattern.degree(p);

            for (unsigned t = 0 ; t < target.size() ; ++t) {
                target_degrees[t] = target.degree(t);
                largest_target_degree = max(largest_target_degree, target_degrees[t]);
            }

            if (params.except >= 1) {
                for (unsigned v = 0 ; v < params.except ; ++v)
                    target_degrees.at(wildcard_start + v) = largest_target_degree + 1;
                ++largest_target_degree;
            }

            vector<vector<vector<unsigned> > > p_nds(adjacency_constraints.size());
            vector<vector<vector<unsigned> > > t_nds(adjacency_constraints.size());

            for (unsigned p = 0 ; p < pattern.size() ; ++p) {
                unsigned cn = 0;
                for (auto & c : adjacency_constraints) {
                    p_nds[cn].resize(pattern.size());
                    for (unsigned q = 0 ; q < pattern.size() ; ++q)
                        if (c.first[p][q])
                            p_nds[cn][p].push_back(c.first[q].count());
                    sort(p_nds[cn][p].begin(), p_nds[cn][p].end(), greater<unsigned>());
                    ++cn;
                }
            }

            // build up initial domains
            Bitset_ initial_domains_union = Bitset_(domain_size);
            for (unsigned q = 0 ; q < domain_size ; ++q)
                initial_domains_union.set(q);

            t_nds = vector<vector<vector<unsigned> > >(adjacency_constraints.size());

            for (unsigned t = 0 ; t < target.size() ; ++t) {
                unsigned cn = 0;
                for (auto & c : adjacency_constraints) {
                    t_nds[cn].resize(target.size());
                    for (unsigned q = 0 ; q < target.size() ; ++q)
                        if (c.second[t][q])
                            t_nds[cn][t].push_back((c.second[q] & initial_domains_union).count());
                    sort(t_nds[cn][t].begin(), t_nds[cn][t].end(), greater<unsigned>());
                    ++cn;
                }
            }

            for (unsigned p = 0 ; p < pattern.size() ; ++p) {
                initial_domains[p].v = p;
                initial_domains[p].values = Bitset_(domain_size);
                initial_domains[p].fixed = false;

                // decide initial domain values
                for (unsigned t = 0 ; t < target.size() ; ++t) {
                    if (! initial_domains_union[t])
                        continue;

                    bool ok = true;

                    for (auto & c : adjacency_constraints) {
                        // check loops
                        if (c.first[p][p] && ! c.second[t][t])
                            ok = false;

                        auto c_second_t = c.second[t] & initial_domains_union;

                        // check degree
                        if (ok && 0 == params.except && ! (c.first[p].count() <= c_second_t.count()))
                            ok = false;

                        // check except-degree
                        if (ok && params.except >= 1 && ! (c.first[p].count() <= c_second_t.count() + params.except))
                            ok = false;

                        if (! ok)
                            break;
                    }

                    // neighbourhood degree sequences
                    for (unsigned cn = 0 ; cn < 1 && ok ; ++cn) {
                        for (unsigned i = params.except ; i < p_nds[cn][p].size() ; ++i) {
                            if (t_nds[cn][t][i - params.except] + params.except < p_nds[cn][p][i]) {
                                ok = false;
                                break;
                            }
                        }
                    }

                    if (ok)
                        initial_domains[p].values.set(t);
                }

                // wildcard in domain?
                if (params.except >= 1)
                    for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                        initial_domains[p].values.set(v);

                // set up space for watches
                if (params.restarts)
                    watches.initialise(pattern.size(), domain_size);
            }
        }

        auto post_nogood(
                const Assignments & assignments)
        {
            Nogood nogood;

            for (auto & a : assignments.values)
                if (get<1>(a)) {
                    auto normalised_assignment = get<0>(a);
                    if (normalised_assignment.second >= wildcard_start)
                        normalised_assignment.second = wildcard_start;
                    nogood.literals.emplace_back(normalised_assignment);
                }

            nogoods.emplace_back(move(nogood));
            need_to_watch.emplace_back(prev(nogoods.end()));
        }

        auto save_result(const Assignments & assignments, Result & result) -> void
        {
            for (auto & a : assignments.values)
                if (get<0>(a).second >= wildcard_start)
                    result.isomorphism.emplace(get<0>(a).first, -1);
                else
                    result.isomorphism.emplace(get<0>(a).first, get<0>(a).second);
        }

        auto add_complement_constraints(const Graph & pattern, const Graph & target) -> auto
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            build_d1_adjacency(pattern, false, d1.first, true);
            build_d1_adjacency(target, true, d1.second, true);

            return d1;
        }

        auto add_adjacency_constraints(const Graph & pattern, const Graph & target) -> void
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            build_d1_adjacency(pattern, false, d1.first, false);
            build_d1_adjacency(target, true, d1.second, false);

            auto & d21 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            auto & d22 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            auto & d23 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));

            build_d2_adjacency(pattern.size(), d1.first, false, d21.first, d22.first, d23.first);
            build_d2_adjacency(target.size(), d1.second, true, d21.second, d22.second, d23.second);

            if (params.induced) {
                auto d1c = add_complement_constraints(pattern, target);
            }
        }

        auto build_d1_adjacency(const Graph & graph, bool is_target, vector<Bitset_> & adj, bool complement) const -> void
        {
            adj.resize(graph.size());
            for (unsigned t = 0 ; t < graph.size() ; ++t) {
                adj[t] = Bitset_(is_target ? domain_size : graph.size());
                for (unsigned u = 0 ; u < graph.size() ; ++u)
                    if (graph.adjacent(t, u) != complement)
                        adj[t].set(u);
            }
        }

        auto build_d2_adjacency(
                const unsigned graph_size,
                const vector<Bitset_> & d1_adj,
                bool is_target,
                vector<Bitset_> & adj1,
                vector<Bitset_> & adj2,
                vector<Bitset_> & adj3) const -> void
        {
            adj1.resize(graph_size);
            adj2.resize(graph_size);
            adj3.resize(graph_size);

            vector<vector<unsigned> > counts(graph_size, vector<unsigned>(graph_size, 0));

            for (unsigned t = 0 ; t < graph_size ; ++t) {
                adj1[t] = Bitset_(is_target ? domain_size : graph_size);
                adj2[t] = Bitset_(is_target ? domain_size : graph_size);
                adj3[t] = Bitset_(is_target ? domain_size : graph_size);
                for (auto u = d1_adj[t].find_first() ; u != Bitset_::npos ; u = d1_adj[t].find_next(u))
                    if (t != u)
                        for (auto v = d1_adj[u].find_first() ; v != Bitset_::npos ; v = d1_adj[u].find_next(v))
                            if (u != v && t != v)
                                ++counts[t][v];
            }

            for (unsigned t = 0 ; t < graph_size ; ++t)
                for (unsigned u = 0 ; u < graph_size ; ++u) {
                    if (counts[t][u] >= 3 + (is_target ? 0 : params.except))
                        adj3[t].set(u);
                    if (counts[t][u] >= 2 + (is_target ? 0 : params.except))
                        adj2[t].set(u);
                    if (counts[t][u] >= 1 + (is_target ? 0 : params.except))
                        adj1[t].set(u);
                }
        }

        auto select_branch_domain(const Domains & domains) -> typename Domains::const_iterator
        {
            auto best = domains.end();

            for (auto d = domains.begin() ; d != domains.end() ; ++d) {
                if (d->fixed)
                    continue;

                if (best == domains.end())
                    best = d;
                else {
                    int best_c = best->values.count();
                    int d_c = d->values.count();

                    if (d_c < best_c)
                        best = d;
                    else if (d_c == best_c) {
                        if (pattern_degrees[d->v] > pattern_degrees[best->v])
                            best = d;
                        else if (pattern_degrees[d->v] == pattern_degrees[best->v] && d->v < best->v)
                            best = d;
                    }
                }
            }

            return best;
        }

        auto select_unit_domain(Domains & domains) -> typename Domains::iterator
        {
            return find_if(domains.begin(), domains.end(), [&] (const auto & a) {
                    if (! a.fixed) {
                        auto c = a.values.count();
                        return 1 == c || (c > 1 && a.values.find_first() >= wildcard_start);
                    }
                    else
                        return false;
                    });
        }

        auto propagate_watches(Domains & new_domains, Assignments & assignments, const Assignment & current_assignment) -> bool
        {
            auto & watches_to_update = watches[current_assignment];
            for (auto watch_to_update = watches_to_update.begin() ; watch_to_update != watches_to_update.end() ; ) {
                Nogood & nogood = **watch_to_update;

                // make the first watch the thing we just triggered
                if (nogood.literals[0] != current_assignment)
                    swap(nogood.literals[0], nogood.literals[1]);

                // can we find something else to watch?
                bool success = false;
                for (auto new_literal = next(nogood.literals.begin(), 2) ; new_literal != nogood.literals.end() ; ++new_literal) {
                    if (! assignments.contains(*new_literal, wildcard_start)) {
                        // we can watch new_literal instead of current_assignment in this nogood
                        success = true;

                        // move the new watch to be the first item in the nogood
                        swap(nogood.literals[0], *new_literal);

                        // start watching it
                        watches[nogood.literals[0]].push_back(*watch_to_update);

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

                    if (d.v == nogood.literals[1].first) {
                        d.values.reset(nogood.literals[1].second);
                        if (nogood.literals[1].second >= wildcard_start) {
                            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                                d.values.reset(v);
                        }
                        break;
                    }
                }

                // step the loop variable, only if we've not already erased it
                ++watch_to_update;
            }

            return true;
        }

        auto propagate(Domains & domains, Assignments & assignments) -> bool
        {
            while (! domains.empty()) {
                auto unit_domain_iter = select_unit_domain(domains);

                if (unit_domain_iter == domains.end()) {
                    if (! cheap_all_different(domains))
                        return false;
                    unit_domain_iter = select_unit_domain(domains);
                    if (unit_domain_iter == domains.end())
                        break;
                }

                auto unit_domain_v = unit_domain_iter->v;
                auto unit_domain_value = unit_domain_iter->values.find_first();
                unit_domain_iter->fixed = true;

                assignments.values.push_back({ { unit_domain_v, unit_domain_value }, false, -1 });

                // propagate watches
                if (params.restarts) {
                    Assignment normalised_current_assignment = { unit_domain_v, unit_domain_value };
                    if (normalised_current_assignment.second >= wildcard_start)
                        normalised_current_assignment.second = wildcard_start;
                    if (! propagate_watches(domains, assignments, normalised_current_assignment))
                        return false;
                }

                for (auto & d : domains) {
                    if (d.fixed)
                        continue;

                    // injectivity
                    d.values.reset(unit_domain_value);

                    // adjacency
                    if (unit_domain_value < wildcard_start)
                        for (auto & c : adjacency_constraints)
                            if (c.first[unit_domain_v][d.v])
                                d.values &= (c.second[unit_domain_value] | all_wildcards);

                    if (d.values.none())
                        return false;
                }
            }

            return true;
        }

        auto cheap_all_different(Domains & domains) -> bool
        {
            // pick domains smallest first, with tiebreaking
            vector<pair<int, int> > domains_order;
            domains_order.resize(domains.size());
            for (unsigned d = 0 ; d < domains.size() ; ++d) {
                domains_order[d].first = d;
                domains_order[d].second = domains[d].values.count();
            }

            sort(domains_order.begin(), domains_order.begin() + domains.size(),
                    [&] (const pair<int, int> & a, const pair<int, int> & b) {
                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                    });

            // counting all-different
            Bitset_ domains_so_far = Bitset_(domain_size), hall = Bitset_(domain_size);
            unsigned neighbours_so_far = 0;

            for (int i = 0, i_end = domains.size() ; i != i_end ; ++i) {
                auto & d = domains.at(domains_order.at(i).first);

                d.values &= ~hall;

                if (d.values.none())
                    return false;

                domains_so_far |= d.values;
                ++neighbours_so_far;

                unsigned domains_so_far_popcount = domains_so_far.count();
                if (domains_so_far_popcount < neighbours_so_far)
                    return false;
                else if (domains_so_far_popcount == neighbours_so_far) {
                    neighbours_so_far = 0;
                    hall |= domains_so_far;
                    domains_so_far.reset();
                }
            }

            return true;
        }

        auto solve(
                const Domains & domains,
                Assignments & assignments,
                int depth,
                long long & backtracks_until_restart) -> RestartingSearch
        {
            if (*params.abort)
                return RestartingSearch::Aborted;

            ++result.nodes;

            auto branch_domain = select_branch_domain(domains);

            if (domains.end() == branch_domain)
                return RestartingSearch::Satisfiable;

            vector<unsigned> branch_values;
            for (auto branch_value = branch_domain->values.find_first() ;
                    branch_value != Bitset_::npos ;
                    branch_value = branch_domain->values.find_next(branch_value))
                branch_values.push_back(branch_value);

            if (params.restarts) {
                // repeatedly pick a softmax-biased vertex, move it to the front of branch_values,
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
                for (unsigned v = 0 ; v < branch_values.size() ; ++v)
                    total += expish(target_degrees[branch_values[v]]);

                for (unsigned start = 0 ; start < branch_values.size() ; ++start) {
                    // pick a random number between 1 and total inclusive
                    uniform_int_distribution<long long> dist(1, total);
                    long long select_score = dist(global_rand);

                    // go over the list until we hit the score
                    unsigned select_element = start;
                    for ( ; select_element + 1 < branch_values.size() ; ++select_element) {
                        select_score -= expish(target_degrees[branch_values[select_element]]);
                        if (select_score <= 0)
                            break;
                    }

                    // move to front
                    total -= expish(target_degrees[branch_values[select_element]]);
                    swap(branch_values[select_element], branch_values[start]);
                }
            }
            else {
                sort(branch_values.begin(), branch_values.end(), [&] (const auto & a, const auto & b) {
                        return target_degrees.at(a) > target_degrees.at(b) || (target_degrees.at(a) == target_degrees.at(b) && a < b);
                        });
            }

            bool already_did_a_wildcard = false;

            int discrepancy_count = 0;
            for (auto & branch_value : branch_values) {
                if (*params.abort)
                    return RestartingSearch::Aborted;

                if (already_did_a_wildcard && branch_value >= wildcard_start)
                    continue;

                // a bit of jiggerypokery to get nogoods right (this loop contains continues)
                if (branch_value >= wildcard_start)
                    already_did_a_wildcard = true;

                auto assignments_size = assignments.values.size();
                assignments.values.push_back({ { branch_domain->v, branch_value }, true, discrepancy_count });

                Domains new_domains;
                new_domains.reserve(domains.size());
                for (auto & d : domains) {
                    if (d.fixed)
                        continue;

                    if (d.v == branch_domain->v) {
                        Bitset_ just_branch_value = d.values;
                        just_branch_value.reset();
                        just_branch_value.set(branch_value);
                        new_domains.emplace_back(Domain{ unsigned(d.v), false, just_branch_value });
                    }
                    else
                        new_domains.emplace_back(Domain{ unsigned(d.v), false, d.values });
                }

                // propagate
                if (! propagate(new_domains, assignments)) {
                    // failure? restore assignments and go on to the next thing
                    assignments.values.resize(assignments_size);
                    continue;
                }

                // recursive search
                auto search_result = solve(new_domains, assignments, depth + 1, backtracks_until_restart);

                switch (search_result) {
                    case RestartingSearch::Satisfiable:
                        return RestartingSearch::Satisfiable;

                    case RestartingSearch::Aborted:
                        return RestartingSearch::Aborted;

                    case RestartingSearch::Restart:
                        // restore assignments before posting nogoods, it's easier
                        assignments.values.resize(assignments_size);

                        // post nogoods for everything we've done so far
                        {
                            bool already_posted_a_wildcard = false;
                            for (auto l = branch_values.begin() ; *l != branch_value ; ++l) {
                                if (*l >= wildcard_start) {
                                    if (already_posted_a_wildcard)
                                        continue;
                                    already_posted_a_wildcard = true;
                                }
                                assignments.values.push_back({ { branch_domain->v, *l }, true, -2 });
                                post_nogood(assignments);
                                assignments.values.pop_back();
                            }
                        }

                        return RestartingSearch::Restart;

                    case RestartingSearch::Unsatisfiable:
                        // restore assignments
                        assignments.values.resize(assignments_size);
                        break;
                }

                // restore assignments
                assignments.values.resize(assignments_size);
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

        auto run()
        {
            Assignments assignments;

            if (params.restarts) {
                bool done = false;
                list<long long> luby = {{ 1 }};
                auto current_luby = luby.begin();
                unsigned number_of_restarts = 0;

                while (! done) {
                    long long backtracks_until_restart;

                    backtracks_until_restart = *current_luby * params.luby_multiplier;
                    if (next(current_luby) == luby.end()) {
                        luby.insert(luby.end(), luby.begin(), luby.end());
                        luby.push_back(*luby.rbegin() * 2);
                    }
                    ++current_luby;

                    ++number_of_restarts;

                    // start watching new nogoods. we're not backjumping so this is a bit icky.
                    for (auto & n : need_to_watch) {
                        if (n->literals.empty()) {
                            done = true;
                            break;
                        }
                        else if (1 == n->literals.size()) {
                            for (auto & d : initial_domains)
                                if (d.v == n->literals[0].first) {
                                    d.values.reset(n->literals[0].second);
                                    break;
                                }
                        }
                        else {
                            watches[n->literals[0]].push_back(n);
                            watches[n->literals[1]].push_back(n);
                        }
                    }
                    need_to_watch.clear();

                    if (done)
                        break;

                    if (propagate(initial_domains, assignments)) {
                        auto assignments_copy = assignments;

                        switch (solve(initial_domains, assignments_copy, 0, backtracks_until_restart)) {
                            case RestartingSearch::Satisfiable:
                                save_result(assignments_copy, result);
                                done = true;
                                break;

                            case RestartingSearch::Unsatisfiable:
                            case RestartingSearch::Aborted:
                                done = true;
                                break;

                            case RestartingSearch::Restart:
                                break;
                        }
                    }
                    else
                        done = true;
                }
            }
            else {
                if (propagate(initial_domains, assignments)) {
                    long long never_restart = -1;
                    if (RestartingSearch::Satisfiable == solve(initial_domains, assignments, 0, never_restart))
                        save_result(assignments, result);
                }
            }
        }
    };
}

auto sequential_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    if (graphs.second.size() + params.except <= 63) {
        SIP<FixedBitSet<64 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 127) {
        SIP<FixedBitSet<128 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 255) {
        SIP<FixedBitSet<256 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 447) {
        SIP<FixedBitSet<448 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 511) {
        SIP<FixedBitSet<512 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else {
        SIP<boost::dynamic_bitset<> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
}

auto sequential_ix_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    auto modified_params = params;
    Result modified_result;

    while (! *modified_params.abort) {
        auto start_time = steady_clock::now();

        SIP<boost::dynamic_bitset<> > sip(modified_params, graphs.first, graphs.second);

        sip.run();

        auto pass_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
        modified_result.times.push_back(pass_time);

        modified_result.nodes += sip.result.nodes;
        if (! sip.result.isomorphism.empty()) {
            modified_result.isomorphism = sip.result.isomorphism;
            modified_result.stats.emplace("EXCEPT", to_string(modified_params.except));
            modified_result.stats.emplace("SIZE", to_string(graphs.first.size() - modified_params.except));
            return modified_result;
        }
        else {
            cout << "-- " << pass_time.count() << " <" << graphs.first.size() - modified_params.except << " " << sip.result.nodes << endl;
            modified_result.stats.emplace("FAIL" + to_string(modified_params.except), to_string(pass_time.count()));
        }

        if (++modified_params.except >= graphs.first.size())
            break;
    }

    return modified_result;
}

