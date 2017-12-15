#define LUBY_MULTIPLIER 500 

#include "graph.hh"
#include "solve_mcs.hh"

#include <algorithm>
#include <iostream>
#include <list>
#include <numeric>
#include <random>
#include <set>
#include <vector>

using std::cout;
using std::endl;
using std::list;
using std::vector;

std::atomic<bool> abort_due_to_timeout;

namespace {
    std::mt19937 global_rand;

    struct VarAssignment
    {
        Assignment assignment;
        bool is_decision;
    };

    class VarAssignments
    {
        vector<VarAssignment> var_assignments;
        unsigned num_vtx_assignments = 0;
    public:
        auto push(VarAssignment a) -> void
        {
            var_assignments.push_back(a);
            if (a.assignment.w != -1)
                ++num_vtx_assignments;
        }

        auto pop() -> void
        {
            if (var_assignments.back().assignment.w != -1)
                --num_vtx_assignments;
            var_assignments.pop_back();
        }

        auto get_var_assignments() const -> const vector<VarAssignment> &
        {
            return var_assignments;
        }

        auto get_num_vtx_assignments() -> unsigned
        {
            return num_vtx_assignments;
        }

        auto clear() -> void
        {
            var_assignments.clear();
        }

        bool contains(const Assignment & assignment) const
        {
            return var_assignments.end() != find_if(var_assignments.begin(), var_assignments.end(),
                    [&] (const VarAssignment & a) { return a.assignment == assignment; });
        }
    };

    struct Bidomain
    {
        int l,        r;        // start indices of left and right sets
        int left_len, right_len;
        bool is_adjacent;
    };

    auto calculate_degrees(const Graph & g) -> vector<int>
    {
        vector<int> degree(g.n, 0);
        for (int v=0; v<g.n; v++) {
            for (int w=0; w<g.n; w++) {
                unsigned int mask = 0xFFFFu;
                if (g.adjmat[v][w] & mask) degree[v]++;
                if (g.adjmat[v][w] & ~mask) degree[v]++;  // inward edge, in directed case
            }
        }
        return degree;
    }

    struct Nogood;

    using Nogoods = list<Nogood>;

    struct Nogood
    {
        vector<Assignment> literals;

        struct ClauseMembership
        {
            list<Nogoods::iterator> & lst;
            list<Nogoods::iterator>::iterator it;
        };

        // Keep track of lists and list nodes in literal_clause_membership that
        // point to this Nogood, so that we can efficiently delete the list nodes
        // if this clause is erased.
        vector<ClauseMembership> clause_memberships;

        // a hash of the nogood, used in a quick, incomplete subsumption test
        unsigned long long signature = 0;

        auto add_literal(Assignment literal) -> void
        {
            literals.emplace_back(literal);
            // Set a couple of bits in the signature.  I'm sure this could be done better!
            signature |= 1ull << ((11400714819323198549ull * (literal.v + literal.w + 1)) >> 58);
            signature |= 1ull << ((11400714819323198549ull * (literal.v * (literal.w + 2))) >> 58);
        }
        auto add_clause_membership(list<Nogoods::iterator> & lst,
                list<Nogoods::iterator>::iterator it) -> void
        {
            clause_memberships.push_back({lst, it});
        }
        auto remove_from_clause_membership_lists() -> void
        {
            for (auto & cm : clause_memberships)
                cm.lst.erase(cm.it);
        }
    };

    // One watched literal for our nogoods store.
    struct Watches
    {
        // for each watched literal, we have a list of watched things, each of
        // which is an iterator into the global watch list (so we can reorder
        // the literal to keep the watch as the first element)
        using WatchList = list<Nogoods::iterator>;

        unsigned target_size_plus_1;

        // two dimensional array, indexed by ((target_size+1) * p + t + 1)
        vector<WatchList> data;

        // not a ctor to avoid annoyingness with isolated vertices altering the
        // pattern size
        Watches(unsigned p, unsigned t)
            : target_size_plus_1(t + 1), data(p * (t + 1))
        { }

        auto operator[] (const Assignment & a) -> WatchList &
        {
            return data[target_size_plus_1 * a.v + a.w + 1];
        }
    };

    class MCS
    {
        const Graph & g0;
        const Graph & g1;
        const Params params;
		unsigned long long nodes = 0;
        vector<int> left;
        vector<int> right;
        vector<Assignment> incumbent;
        Nogoods nogoods;
        Watches watches;
        Watches literal_clause_membership;

        // used in subsumption test; assigned once to avoid a vector
        // allocation on each function call
        vector<int> assignments_in_nogood;

        vector<unsigned long long> target_vertex_biases;

        auto show(const vector<VarAssignment>& current, const vector<Bidomain> &domains) -> void
        {
            cout << "Nodes: " << nodes << endl;
            cout << "Length of current assignment: " << current.size() << endl;
            cout << "Current assignment:";
            for (unsigned int i=0; i<current.size(); i++) {
                cout << "  (" << current[i].assignment.v << " -> " << current[i].assignment.w
                        << " ; " << current[i].is_decision << ")";
            }
            cout << endl;
            for (unsigned int i=0; i<domains.size(); i++) {
                struct Bidomain bd = domains[i];
                cout << "Left  ";
                for (int j=0; j<bd.left_len; j++)
                    cout << left[bd.l + j] << " ";
                cout << endl;
                cout << "Right  ";
                for (int j=0; j<bd.right_len; j++)
                    cout << right[bd.r + j] << " ";
                cout << endl;
            }
            cout << "\n" << endl;
        }

        auto calc_bound(const vector<Bidomain>& domains) -> int
        {
            int bound = 0;
            for (const Bidomain &bd : domains)
                bound += std::min(bd.left_len, bd.right_len);
            return bound;
        }

        // precondition: len > 0
        auto find_min_value(const vector<int>& arr, int start_idx, int len) -> int
        {
            return *std::min_element(arr.begin() + start_idx, arr.begin() + start_idx + len);
        }

        auto select_bidomain(const vector<Bidomain>& domains, int current_matching_size) -> int
        {
            // Select the bidomain with the smallest max(leftsize, rightsize), breaking
            // ties on the smallest vertex index in the left set
            std::pair<int, int> best_score { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
            int best = -1;
            for (unsigned int i=0; i<domains.size(); i++) {
                const Bidomain &bd = domains[i];
                if (params.connected && current_matching_size>0 && !bd.is_adjacent) continue;
                int len = params.heuristic == Heuristic::min_max ?
                        std::max(bd.left_len, bd.right_len) :
                        bd.left_len * bd.right_len;
                int tie_breaker = find_min_value(left, bd.l, bd.left_len);
                auto score = std::make_pair( len, tie_breaker );
                if (score < best_score) {
                    best_score = score;
                    best = i;
                }
            }
            return best;
        }

        // Returns length of left half of array
        auto partition(vector<int>& all_vv, int start, int len, const vector<unsigned int> & adjrow) -> int
        {
            auto it = std::partition(
                    all_vv.begin() + start,
                    all_vv.begin() + start + len,
                    [&](const int elem){ return 0 != adjrow[elem]; });
            return it - (all_vv.begin() + start);
        }

        // multiway is for directed and/or labelled graphs
        auto filter_domains(const vector<Bidomain> & d, int v, int w, bool multiway) -> vector<Bidomain>
        {
            vector<Bidomain> new_d;
            new_d.reserve(d.size());
            for (const Bidomain &old_bd : d) {
                int l = old_bd.l;
                int r = old_bd.r;
                // After these two partitions, left_len and right_len are the lengths of the
                // arrays of vertices with edges from v or w (int the directed case, edges
                // either from or to v or w)
                int left_len = partition(left, l, old_bd.left_len, g0.adjmat[v]);
                int right_len = partition(right, r, old_bd.right_len, g1.adjmat[w]);
                int left_len_noedge = old_bd.left_len - left_len;
                int right_len_noedge = old_bd.right_len - right_len;
                if (left_len_noedge && right_len_noedge)
                    new_d.push_back({l+left_len, r+right_len, left_len_noedge, right_len_noedge, old_bd.is_adjacent});
                if (multiway && left_len && right_len) {
                    auto& adjrow_v = g0.adjmat[v];
                    auto& adjrow_w = g1.adjmat[w];
                    auto l_begin = std::begin(left) + l;
                    auto r_begin = std::begin(right) + r;
                    std::sort(l_begin, l_begin+left_len, [&](int a, int b)
                            { return adjrow_v[a] < adjrow_v[b]; });
                    std::sort(r_begin, r_begin+right_len, [&](int a, int b)
                            { return adjrow_w[a] < adjrow_w[b]; });
                    int l_top = l + left_len;
                    int r_top = r + right_len;
                    while (l<l_top && r<r_top) {
                        unsigned int left_label = adjrow_v[left[l]];
                        unsigned int right_label = adjrow_w[right[r]];
                        if (left_label < right_label) {
                            l++;
                        } else if (left_label > right_label) {
                            r++;
                        } else {
                            int lmin = l;
                            int rmin = r;
                            do { l++; } while (l<l_top && adjrow_v[left[l]]==left_label);
                            do { r++; } while (r<r_top && adjrow_w[right[r]]==left_label);
                            new_d.push_back({lmin, rmin, l-lmin, r-rmin, true});
                        }
                    }
                } else if (left_len && right_len) {
                    new_d.push_back({l, r, left_len, right_len, true});
                }
            }
            return new_d;
        }

        auto remove_vtx_from_left_domain(Bidomain& bd, int v) -> void
        {
            int i = 0;
            while(left[bd.l + i] != v) i++;
            std::swap(left[bd.l+i], left[bd.l+bd.left_len-1]);
            bd.left_len--;
        }

        auto remove_bidomain(vector<Bidomain>& domains, int idx) -> void
        {
            domains[idx] = domains[domains.size()-1];
            domains.pop_back();
        }

        auto biased_shuffle(vector<int> & vec) -> void
        {
            // sum up the bias scores of every branch vertex
            unsigned long long remaining_score = 0;
            for (unsigned v = 0 ; v < vec.size() ; ++v)
                remaining_score += target_vertex_biases[vec[v]];

            // now repeatedly pick a biased-random vertex, move it to the front of vec,
            // and then only consider items further to the right in the next iteration.
            for (unsigned start = 0 ; start < vec.size() ; ++start) {
                // pick a random number between 0 and remaining_score inclusive
                std::uniform_int_distribution<unsigned long long> dist(1, remaining_score);
                unsigned long long select_score = dist(global_rand);

                // go over the list until we've used up bias values totalling our
                // random number
                unsigned select_element = start;
                for ( ; select_element < vec.size() ; ++select_element) {
                    if (select_score <= target_vertex_biases[vec[select_element]])
                        break;
                    select_score -= target_vertex_biases[vec[select_element]];
                }

                // move to front, and update remaining_score
                remaining_score -= target_vertex_biases[vec[select_element]];
                std::swap(vec[select_element], vec[start]);
            }
        }

        auto update_incumbent(vector<Assignment> & incumbent, VarAssignments current) -> void
        {
            incumbent.clear();
            for (auto a : current.get_var_assignments())
                if (a.assignment.w != -1)
                    incumbent.push_back(a.assignment);
            if (!params.quiet) cout << "Incumbent size: " << incumbent.size() << endl;
        }

        auto post_nogood(const VarAssignments & current) -> bool
        {
            nogoods.push_back({});
            auto & nogood = nogoods.back();

            for (auto & a : current.get_var_assignments())
                if (a.is_decision)
                    nogood.add_literal(a.assignment);

            if (nogood.literals.empty())
                return false;

            auto nogood_it = prev(nogoods.end());

            for (auto & a : nogood.literals) {
                auto & lcm = literal_clause_membership[a];
                lcm.emplace_back(nogood_it);
                nogood.add_clause_membership(lcm, prev(lcm.end()));
            }

            remove_subsumed_clauses(nogood_it);
            watches[nogood.literals[0]].push_back(nogood_it);

            return true;
        }

        auto erase_nogood(Nogoods::iterator nogood_it) -> void
        {
            auto & wl = watches[nogood_it->literals[0]];
            wl.erase(std::find(wl.begin(), wl.end(), nogood_it));

            nogood_it->remove_from_clause_membership_lists();

            nogoods.erase(nogood_it);
        }

        auto can_find_another_watch(Nogood & nogood,
                list<Nogoods::iterator> & watches_to_update,
                list<Nogoods::iterator>::iterator watch_to_update,
                VarAssignments & current) -> bool
        {
            // can we find something else to watch?
            for (auto new_literal = next(nogood.literals.begin(), 1) ; new_literal != nogood.literals.end() ; ++new_literal) {
                if (!current.contains(*new_literal)) {
                    // move the new watch to be the first item in the nogood
                    std::swap(nogood.literals[0], *new_literal);

                    // start watching it
                    watches[nogood.literals[0]].push_back(*watch_to_update);

                    // remove the current watch, and update the loop iterator
                    watches_to_update.erase(watch_to_update++);

                    return true;
                }
            }
            return false;
        }

        auto current_contains_nogood(
                Assignment most_recent_assignment,
                VarAssignments & current) -> bool
        {
            auto & watches_to_update = watches[most_recent_assignment];
            for (auto watch_to_update = watches_to_update.begin() ; watch_to_update != watches_to_update.end() ; ) {
                Nogood & nogood = **watch_to_update;

                auto next_watch_to_update = next(watch_to_update);

                if (!can_find_another_watch(nogood, watches_to_update, watch_to_update, current))
                    return true;
                
                watch_to_update = next_watch_to_update;
            }

            return false;
        }

        auto contains_a_nogood(VarAssignments & current,
                vector<Bidomain> & domains) -> bool
        {
            if (current.get_var_assignments().empty())
                return false;

            auto most_recent_assignment = current.get_var_assignments().back().assignment;

            return current_contains_nogood(most_recent_assignment, current);
        }

        enum class Search
        {
            Aborted,
            Done,
            Restart
        };

        auto restarting_search(
                VarAssignments & current,
                vector<Bidomain> & domains,
                long long & backtracks_until_restart) -> Search
        {
            if (abort_due_to_timeout)
                return Search::Aborted;

            nodes++;

            if (params.verbose)
                show(current.get_var_assignments(), domains);

            if (current.get_num_vtx_assignments() > incumbent.size())
                update_incumbent(incumbent, current);

            unsigned int bound = current.get_num_vtx_assignments() + calc_bound(domains);

            int bd_idx;

            // Is more than one value choice available at this search node?
            // This will be set to true if more than one possible value is found to exist
            // later in the function.
            bool is_decision = false;

            if (bound > incumbent.size() &&
                    !contains_a_nogood(current, domains) &&
                    -1 != (bd_idx = select_bidomain(domains, current.get_num_vtx_assignments())))
            {
                Bidomain &bd = domains[bd_idx];

                int v = find_min_value(left, bd.l, bd.left_len);

                // Try assigning v to each vertex w in the colour class beginning at bd.r, in turn
                std::vector<int> possible_values(right.begin() + bd.r,  // the vertices in the colour class beginning at bd.r
                        right.begin() + bd.r + bd.right_len);
                std::sort(possible_values.begin(), possible_values.end());

                if (params.biased_shuffle)
                    biased_shuffle(possible_values);

                if (bound != incumbent.size() + 1 || bd.left_len > bd.right_len)
                    possible_values.push_back(-1);

                if (possible_values.size() > 1)
                    is_decision = true;

                remove_vtx_from_left_domain(domains[bd_idx], v);
                bd.right_len--;

                for (int w : possible_values) {
                    current.push({{v, w}, is_decision});
                    Search search_result;
                    if (w != -1) {
                        // swap w to the end of its colour class
                        auto it = std::find(right.begin() + bd.r, right.end(), w);
                        *it = right[bd.r + bd.right_len];
                        right[bd.r + bd.right_len] = w;

                        auto new_domains = filter_domains(domains, v, w, params.directed || params.edge_labelled);
                        search_result = restarting_search(current, new_domains, backtracks_until_restart);
                    } else {
                        bd.right_len++;
                        if (bd.left_len == 0)
                            remove_bidomain(domains, bd_idx);
                        search_result = restarting_search(current, domains, backtracks_until_restart);
                    }
                    current.pop();
                    switch (search_result)
                    {
                    case Search::Restart:
                        for (int u : possible_values) {
                            if (u == w)
                                break;
                            current.push({{v, u}, true});
                            post_nogood(current);
                            current.pop();
                        }
                        return Search::Restart;
                    case Search::Aborted:
                        return Search::Aborted;
                    case Search::Done:
                        break;
                    }
                }
            }

            if (is_decision && backtracks_until_restart > 0 && 0 == --backtracks_until_restart && post_nogood(current)) {
                return Search::Restart;
            } else {
                return Search::Done;
            }
        }

        auto nogood_subsumes(const Nogood & a, const Nogood & b) -> bool
        {
            // This is based on Armin Biere (2005), Resolve and Expand
            // https://link.springer.com/content/pdf/10.1007/11527695_5.pdf
            // Biere's algorithm is described in the solution to exercise
            // 374 of Knuth's TAOCP fascicle on SAT.

            if (a.literals.size() > b.literals.size())
                return false;

            // do a quick partial test to rule out some non-subsumptions
            if (a.signature & ~b.signature)
                return false;

            // Return false if some literal in `a` does not appear in `b`.
            unsigned count = 0;
            for (auto lit : b.literals)
                count += (assignments_in_nogood[lit.v] == lit.w);
            return count == a.literals.size();
        }

        // precondition: the nogood that n_it points to is not empty
        auto remove_subsumed_clauses(Nogoods::iterator n_it) -> void {
            auto & n = *n_it;
            // `literal_with_lowest_tally` will be an iterator to the literal in `n` that appears in
            // fewest members of `nogoods`
            auto literal_with_lowest_tally = std::min_element(
                    n.literals.begin(),
                    n.literals.end(),
                    [&](const Assignment & a, const Assignment & b) {
                        return literal_clause_membership[a].size() < literal_clause_membership[b].size();
                    });

            // `lst` is the list of clauses containing the
            // literal `*literal_with_lowest_tally`
            auto & lst = literal_clause_membership[*literal_with_lowest_tally];

            std::fill(assignments_in_nogood.begin(), assignments_in_nogood.end(), -2);
            for (auto lit : n.literals)
                assignments_in_nogood[lit.v] = lit.w;

            // erase all nogoods in `lst` that are subsumed by `n`
            for (auto nogood_it_it = lst.begin() ; nogood_it_it != lst.end() ; ) {
                auto nxt = next(nogood_it_it);
                if (n_it != *nogood_it_it && nogood_subsumes(n, **nogood_it_it)) {
                    erase_nogood(*nogood_it_it);
                }
                nogood_it_it = nxt;
            }
        }

        auto run_with_restarts(VarAssignments & current, vector<Bidomain> & domains) -> void
        {
            list<long long> luby = {{ 1 }};
            auto current_luby = luby.begin();
            unsigned number_of_restarts = 0;
            while (true) {
                long long backtracks_until_restart = *current_luby * LUBY_MULTIPLIER;
                if (next(current_luby) == luby.end()) {
                    luby.insert(luby.end(), luby.begin(), luby.end());
                    luby.push_back(*luby.rbegin() * 2);
                }
                ++current_luby;

                ++number_of_restarts;

                current.clear();
                auto domains_copy = domains;
    //            std::cout << "restarting search" << endl;
    //            std::cout << "nogood count: " << nogoods.size() << endl;
                switch (restarting_search(current, domains_copy, backtracks_until_restart))
                {
                case Search::Done:
    //                std::cout << "done!" << endl;
                    return;
                case Search::Aborted:
                    return;
                case Search::Restart:
                    break;
                }
            }
        }

        auto search(
                VarAssignments & current,
                vector<Bidomain> & domains) -> Search
        {
            if (abort_due_to_timeout)
                return Search::Aborted;

            nodes++;

            if (params.verbose)
                show(current.get_var_assignments(), domains);

            if (current.get_num_vtx_assignments() > incumbent.size())
                update_incumbent(incumbent, current);

            unsigned int bound = current.get_num_vtx_assignments() + calc_bound(domains);
            if (bound <= incumbent.size())
                return Search::Done;

            int bd_idx = select_bidomain(domains, current.get_num_vtx_assignments());
            if (bd_idx == -1)   // In the MCCS case, there may be nothing we can branch on
                return Search::Done;
            Bidomain &bd = domains[bd_idx];

            int v = find_min_value(left, bd.l, bd.left_len);

            // Try assigning v to each vertex w in the colour class beginning at bd.r, in turn
            std::vector<int> possible_values(right.begin() + bd.r,  // the vertices in the colour class beginning at bd.r
                    right.begin() + bd.r + bd.right_len);
            std::sort(possible_values.begin(), possible_values.end());

            if (params.biased_shuffle)
                biased_shuffle(possible_values);

            if (bound != incumbent.size() + 1 || bd.left_len > bd.right_len)
                possible_values.push_back(-1);

            remove_vtx_from_left_domain(domains[bd_idx], v);
            bd.right_len--;

            for (int w : possible_values) {
                current.push({{v, w}, possible_values.size() > 1});
                Search search_result;
                if (w != -1) {
                    // swap w to the end of its colour class
                    auto it = std::find(right.begin() + bd.r, right.end(), w);
                    *it = right[bd.r + bd.right_len];
                    right[bd.r + bd.right_len] = w;

                    auto new_domains = filter_domains(domains, v, w, params.directed || params.edge_labelled);
                    search_result = search(current, new_domains);
                } else {
                    bd.right_len++;
                    if (bd.left_len == 0)
                        remove_bidomain(domains, bd_idx);
                    search_result = search(current, domains);
                }
                current.pop();
                if (search_result == Search::Aborted)
                    return Search::Aborted;
            }

            return Search::Done;
        }

    public:
        MCS(Graph & g0, Graph & g1, Params params)
            : g0(g0), g1(g1), params(params),
              watches(g0.n, g1.n),
              literal_clause_membership(g0.n, g1.n),
              assignments_in_nogood(g0.n)
        { }

        auto run() -> std::pair<vector<Assignment>, unsigned long long>
        {
            if (params.biased_shuffle) {
                auto g0_deg = calculate_degrees(g0);
                auto g1_deg = calculate_degrees(g1);
                int max_degree = 0;
                for (int j = 0 ; j < g1.n ; ++j)
                    max_degree = std::max(max_degree, g1_deg[j]);

                for (int j = 0 ; j < g1.n ; ++j) {
                    unsigned degree = g1_deg[j];
                    if (max_degree - degree >= 50)
                        target_vertex_biases.push_back(1);
                    else
                        target_vertex_biases.push_back(1ull << (50 - (max_degree - degree)));
                }
            }

            auto domains = vector<Bidomain> {};

            std::set<unsigned int> left_labels;
            std::set<unsigned int> right_labels;
            for (unsigned int label : g0.label) left_labels.insert(label);
            for (unsigned int label : g1.label) right_labels.insert(label);
            std::set<unsigned int> labels;  // labels that appear in both graphs
            std::set_intersection(std::begin(left_labels),
                                  std::end(left_labels),
                                  std::begin(right_labels),
                                  std::end(right_labels),
                                  std::inserter(labels, std::begin(labels)));

            // Create a bidomain for each label that appears in both graphs
            for (unsigned int label : labels) {
                int start_l = left.size();
                int start_r = right.size();

                for (int i=0; i<g0.n; i++)
                    if (g0.label[i]==label)
                        left.push_back(i);
                for (int i=0; i<g1.n; i++)
                    if (g1.label[i]==label)
                        right.push_back(i);

                int left_len = left.size() - start_l;
                int right_len = right.size() - start_r;
                domains.push_back({start_l, start_r, left_len, right_len, false});
            }

            VarAssignments current;

            if (params.restarts) {
                run_with_restarts(current, domains);
            } else {
                search(current, domains);
            }

    //        for (auto & n : nogoods) {
    //            for (auto a : n.literals) {
    //                std::cout << a.v << "," << a.w << " ";
    //            }
    //            std::cout << endl;
    //        }
            return {incumbent, nodes};
        }
    };

    auto sum(const vector<int> & vec) -> int
    {
        return std::accumulate(std::begin(vec), std::end(vec), 0);
    }
};

auto solve_mcs(Graph & g0, Graph & g1, Params params)
		-> std::pair<vector<Assignment>, unsigned long long>
{
    auto g0_deg = calculate_degrees(g0);
    auto g1_deg = calculate_degrees(g1);

    // As implemented here, g1_dense and g0_dense are false for all instances
    // in the Experimental Evaluation section of the paper.  Thus,
    // we always sort the vertices in descending order of degree (or total degree,
    // in the case of directed graphs.  Improvements could be made here: it would
    // be nice if the program explored exactly the same search tree if both
    // input graphs were complemented.
    vector<int> vv0(g0.n);
    std::iota(std::begin(vv0), std::end(vv0), 0);
    bool g1_dense = sum(g1_deg) > g1.n*(g1.n-1);
    std::stable_sort(std::begin(vv0), std::end(vv0), [&](int a, int b) {
        return g1_dense ? (g0_deg[a]<g0_deg[b]) : (g0_deg[a]>g0_deg[b]);
    });
    vector<int> vv1(g1.n);
    std::iota(std::begin(vv1), std::end(vv1), 0);
    bool g0_dense = sum(g0_deg) > g0.n*(g0.n-1);
    std::stable_sort(std::begin(vv1), std::end(vv1), [&](int a, int b) {
        return g0_dense ? (g1_deg[a]<g1_deg[b]) : (g1_deg[a]>g1_deg[b]);
    });

    struct Graph g0_sorted = induced_subgraph(g0, vv0);
    struct Graph g1_sorted = induced_subgraph(g1, vv1);

    auto solution = MCS(g0_sorted, g1_sorted, params).run();

    // Convert to indices from original, unsorted graphs
    for (auto& vtx_pair : solution.first) {
        vtx_pair.v = vv0[vtx_pair.v];
        vtx_pair.w = vv1[vtx_pair.w];
    }

    return solution;
}
