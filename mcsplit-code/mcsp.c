#define RESTARTS 1
#define LUBY_MULTIPLIER 500

#include "graph.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <argp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using std::cout;
using std::endl;
using std::list;
using std::vector;

static void fail(std::string msg) {
    std::cerr << msg << std::endl;
    exit(1);
}

enum Heuristic { min_max, min_product };

/*******************************************************************************
                             Command-line arguments
*******************************************************************************/

static char doc[] = "Find a maximum clique in a graph in DIMACS format\vHEURISTIC can be min_max or min_product";
static char args_doc[] = "HEURISTIC FILENAME1 FILENAME2";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Quiet output"},
    {"verbose", 'v', 0, 0, "Verbose output"},
    {"dimacs", 'd', 0, 0, "Read DIMACS format"},
    {"lad", 'l', 0, 0, "Read LAD format"},
    {"position-shuffle", 'p', 0, 0, "Position-shuffle value ordering heuristic"},
    {"connected", 'c', 0, 0, "Solve max common CONNECTED subgraph problem"},
    {"directed", 'i', 0, 0, "Use directed graphs"},
    {"labelled", 'a', 0, 0, "Use edge and vertex labels"},
    {"vertex-labelled-only", 'x', 0, 0, "Use vertex labels, but not edge labels"},
    {"timeout", 't', "timeout", 0, "Specify a timeout (seconds)"},
    { 0 }
};

struct Arguments {
    bool quiet;
    bool verbose;
    bool dimacs;
    bool lad;
    bool position_shuffle;
    bool connected;
    bool directed;
    bool edge_labelled;
    bool vertex_labelled;
    Heuristic heuristic;
    char *filename1;
    char *filename2;
    int timeout;
    int arg_num;
};

static Arguments arguments;

static std::atomic<bool> abort_due_to_timeout;

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    switch (key) {
        case 'd':
            if (arguments.lad)
                fail("The -d and -l options cannot be used together.\n");
            arguments.dimacs = true;
            break;
        case 'l':
            if (arguments.dimacs)
                fail("The -d and -l options cannot be used together.\n");
            arguments.lad = true;
            break;
        case 'p':
            arguments.position_shuffle = true;
            break;
        case 'q':
            arguments.quiet = true;
            break;
        case 'v':
            arguments.verbose = true;
            break;
        case 'c':
            if (arguments.directed)
                fail("The connected and directed options can't be used together.");
            arguments.connected = true;
            break;
        case 'i':
            if (arguments.connected)
                fail("The connected and directed options can't be used together.");
            arguments.directed = true;
            break;
        case 'a':
            if (arguments.vertex_labelled)
                fail("The -a and -x options can't be used together.");
            arguments.edge_labelled = true;
            arguments.vertex_labelled = true;
            break;
        case 'x':
            if (arguments.edge_labelled)
                fail("The -a and -x options can't be used together.");
            arguments.vertex_labelled = true;
            break;
        case 't':
            arguments.timeout = std::stoi(arg);
            break;
        case ARGP_KEY_ARG:
            if (arguments.arg_num == 0) {
                if (std::string(arg) == "min_max")
                    arguments.heuristic = min_max;
                else if (std::string(arg) == "min_product")
                    arguments.heuristic = min_product;
                else
                    fail("Unknown heuristic (try min_max or min_product)");
            } else if (arguments.arg_num == 1) {
                arguments.filename1 = arg;
            } else if (arguments.arg_num == 2) {
                arguments.filename2 = arg;
            } else {
                argp_usage(state);
            }
            arguments.arg_num++;
            break;
        case ARGP_KEY_END:
            if (arguments.arg_num == 0)
                argp_usage(state);
            break;
        default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

/*******************************************************************************
                                     Stats
*******************************************************************************/

unsigned long long nodes{ 0 };

/*******************************************************************************
                                 MCS functions
*******************************************************************************/

std::mt19937 global_rand;

struct Assignment {
    int v;
    int w;

    auto operator==(const Assignment & other) const -> bool
    {
        return v==other.v && w==other.w;
    }
};

struct VarAssignment {
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
};

struct Bidomain {
    int l,        r;        // start indices of left and right sets
    int left_len, right_len;
    bool is_adjacent;
    Bidomain(int l, int r, int left_len, int right_len, bool is_adjacent):
            l(l),
            r(r),
            left_len (left_len),
            right_len (right_len),
            is_adjacent (is_adjacent) { };
};

bool check_sol(const Graph & g0, const Graph & g1 , const vector<Assignment> & solution) {
    return true;
    vector<bool> used_left(g0.n, false);
    vector<bool> used_right(g1.n, false);
    for (unsigned int i=0; i<solution.size(); i++) {
        struct Assignment p0 = solution[i];
        if (used_left[p0.v] || used_right[p0.w])
            return false;
        used_left[p0.v] = true;
        used_right[p0.w] = true;
        if (g0.label[p0.v] != g1.label[p0.w])
            return false;
        for (unsigned int j=i+1; j<solution.size(); j++) {
            struct Assignment p1 = solution[j];
            if (g0.adjmat[p0.v][p1.v] != g1.adjmat[p0.w][p1.w])
                return false;
        }
    }
    return true;
}

struct Nogood
{
    vector<Assignment> literals;
};

using Nogoods = list<Nogood>;

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
        : target_size_plus_1(t + 1), data(p * (t + 1)) {}

    WatchList & operator[] (const Assignment & a)
    {
        return data[target_size_plus_1 * a.v + a.w + 1];
    }
};

class MCS
{
    const Graph & g0;
    const Graph & g1;
    vector<int> left;
    vector<int> right;
    vector<Assignment> incumbent;
    Nogoods nogoods;
    Watches watches;
    list<typename Nogoods::iterator> need_to_watch;

    vector<int> vtx_current_assignment;

    void show(const vector<VarAssignment>& current, const vector<Bidomain> &domains)
    {
        cout << "Nodes: " << nodes << std::endl;
        cout << "Length of current assignment: " << current.size() << std::endl;
        cout << "Current assignment:";
        for (unsigned int i=0; i<current.size(); i++) {
            cout << "  (" << current[i].assignment.v << " -> " << current[i].assignment.w
                    << " ; " << current[i].is_decision << ")";
        }
        cout << std::endl;
        for (unsigned int i=0; i<domains.size(); i++) {
            struct Bidomain bd = domains[i];
            cout << "Left  ";
            for (int j=0; j<bd.left_len; j++)
                cout << left[bd.l + j] << " ";
            cout << std::endl;
            cout << "Right  ";
            for (int j=0; j<bd.right_len; j++)
                cout << right[bd.r + j] << " ";
            cout << std::endl;
        }
        cout << "\n" << std::endl;
    }

    int calc_bound(const vector<Bidomain>& domains) {
        int bound = 0;
        for (const Bidomain &bd : domains) {
            bound += std::min(bd.left_len, bd.right_len);
        }
        return bound;
    }

    int find_min_value(const vector<int>& arr, int start_idx, int len) {
        int min_v = INT_MAX;
        for (int i=0; i<len; i++)
            if (arr[start_idx + i] < min_v)
                min_v = arr[start_idx + i];
        return min_v;
    }

    int select_bidomain(const vector<Bidomain>& domains, int current_matching_size)
    {
        // Select the bidomain with the smallest max(leftsize, rightsize), breaking
        // ties on the smallest vertex index in the left set
        int min_size = INT_MAX;
        int min_tie_breaker = INT_MAX;
        int best = -1;
        for (unsigned int i=0; i<domains.size(); i++) {
            const Bidomain &bd = domains[i];
            if (arguments.connected && current_matching_size>0 && !bd.is_adjacent) continue;
            int len = arguments.heuristic == min_max ?
                    std::max(bd.left_len, bd.right_len) :
                    bd.left_len * bd.right_len;
            if (len < min_size) {
                min_size = len;
                min_tie_breaker = find_min_value(left, bd.l, bd.left_len);
                best = i;
            } else if (len == min_size) {
                int tie_breaker = find_min_value(left, bd.l, bd.left_len);
                if (tie_breaker < min_tie_breaker) {
                    min_tie_breaker = tie_breaker;
                    best = i;
                }
            }
        }
        return best;
    }

    // Returns length of left half of array
    int partition(vector<int>& all_vv, int start, int len, const vector<unsigned int> & adjrow) {
        int i=0;
        for (int j=0; j<len; j++) {
            if (adjrow[all_vv[start+j]]) {
                std::swap(all_vv[start+i], all_vv[start+j]);
                i++;
            }
        }
        return i;
    }

    // multiway is for directed and/or labelled graphs
    vector<Bidomain> filter_domains(const vector<Bidomain> & d, int v, int w, bool multiway)
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

    // returns the index of the smallest value in arr that is >w.
    // Assumption: such a value exists
    // Assumption: arr contains no duplicates
    // Assumption: arr has no values==INT_MAX
    int index_of_next_smallest(const vector<int>& arr, int start_idx, int len, int w) {
        int idx = -1;
        int smallest = INT_MAX;
        for (int i=0; i<len; i++) {
            if (arr[start_idx + i]>w && arr[start_idx + i]<smallest) {
                smallest = arr[start_idx + i];
                idx = i;
            }
        }
        return idx;
    }

    void remove_vtx_from_left_domain(Bidomain& bd, int v)
    {
        int i = 0;
        while(left[bd.l + i] != v) i++;
        std::swap(left[bd.l+i], left[bd.l+bd.left_len-1]);
        bd.left_len--;
    }

    void remove_bidomain(vector<Bidomain>& domains, int idx) {
        domains[idx] = domains[domains.size()-1];
        domains.pop_back();
    }

    void position_shuffle(vector<int> & vec)
    {
        for (unsigned start=0; start<vec.size()-1; ++start) {
            std::uniform_real_distribution<double> dist(0, 1);
            double select_score = dist(global_rand);

            double select_if_score_ge = 1.0;

            unsigned select_element = start;
            for ( ; select_element + 1 < vec.size(); ++select_element) {
                select_if_score_ge /= 2.0;
                if (select_score >= select_if_score_ge)
                    break;
            }
            
            std::swap(vec[select_element], vec[start]);
        }
    }

    enum class Search
    {
        Aborted,
        Done,
        Restart
    };

    auto update_incumbent(vector<Assignment> & incumbent, VarAssignments current) -> void
    {
        incumbent.clear();
        for (auto a : current.get_var_assignments())
            if (a.assignment.w != -1)
                incumbent.push_back(a.assignment);
        if (!arguments.quiet) cout << "Incumbent size: " << incumbent.size() << endl;
    }

    auto post_nogood(const VarAssignments & current) -> void
    {
        Nogood nogood;

        for (auto & a : current.get_var_assignments())
            if (a.is_decision)
                nogood.literals.emplace_back(a.assignment);

//        std::cout << "literals size " << nogood.literals.size() << std::endl;
        nogoods.emplace_back(std::move(nogood));
        need_to_watch.emplace_back(prev(nogoods.end()));
    }

    auto current_contains_nogood(
            Assignment most_recent_assignment,
            vector<int> & vtx_current_assignment) -> bool
    {
        auto & watches_to_update = watches[most_recent_assignment];
        for (auto watch_to_update = watches_to_update.begin() ; watch_to_update != watches_to_update.end() ; ) {
            Nogood & nogood = **watch_to_update;

            // can we find something else to watch?
            bool success = false;
            for (auto new_literal = next(nogood.literals.begin(), 1) ; new_literal != nogood.literals.end() ; ++new_literal) {
                if (vtx_current_assignment[new_literal->v] != new_literal->w) {
                    // we can watch new_literal instead of current_assignment in this nogood
                    success = true;

                    // move the new watch to be the first item in the nogood
                    std::swap(nogood.literals[0], *new_literal);

                    // start watching it
                    watches[nogood.literals[0]].push_back(*watch_to_update);

                    // remove the current watch, and update the loop iterator
                    watches_to_update.erase(watch_to_update++);

                    break;
                }
            }

            if (!success)
                return true;
        }

        return false;

///////        std::cout << n.literals.size() << std::endl;
/////        for (const Assignment a : n.literals)
/////            if (vtx_current_assignment[a.v] != a.w)
/////                return false;
/////
/////        return true;
    }

    auto contains_a_nogood(VarAssignments & current,
            vector<Bidomain> & domains) -> bool
    {
        std::fill(vtx_current_assignment.begin(), vtx_current_assignment.end(), -1);

        for (auto & bd : domains) {
            for (int i=0; i<bd.left_len; i++) {
                int v = left[bd.l + i];
                vtx_current_assignment[v] = -2;
            }
        }

        for (auto & a : current.get_var_assignments())
            vtx_current_assignment[a.assignment.v] = a.assignment.w;

        auto most_recent_assignment = current.get_var_assignments().back().assignment;

        return current_contains_nogood(most_recent_assignment, vtx_current_assignment);
    }

    auto restarting_search(
            VarAssignments & current,
            vector<Bidomain> & domains,
            long long & backtracks_until_restart) -> Search
    {
        if (abort_due_to_timeout)
            return Search::Aborted;

        nodes++;

        if (arguments.verbose)
            show(current.get_var_assignments(), domains);

        if (current.get_num_vtx_assignments() > incumbent.size())
            update_incumbent(incumbent, current);

        unsigned int bound = current.get_num_vtx_assignments() + calc_bound(domains);
        if (bound <= incumbent.size())
            return Search::Done;

        if (current.get_var_assignments().size() != 0 &&
                current.get_var_assignments().back().is_decision &&
                contains_a_nogood(current, domains))
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

        if (arguments.position_shuffle)
            position_shuffle(possible_values);

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

                auto new_domains = filter_domains(domains, v, w, arguments.directed || arguments.edge_labelled);
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
//                        std::cout << "sz " << current.get_var_assignments().size() << std::endl;
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

        if (backtracks_until_restart > 0 && 0 == --backtracks_until_restart) {
//            std::cout << "sz " << current.get_var_assignments().size() << std::endl;
            post_nogood(current);
            return Search::Restart;
        } else {
            return Search::Done;
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

            for (auto & n : need_to_watch) {
                if (n->literals.empty()) {
                    return;
                } else {
                    watches[n->literals[0]].push_back(n);
                }
            }
            need_to_watch.clear();

            current.clear();
            auto domains_copy = domains;
            std::cout << "restarting search" << std::endl;
            std::cout << "nogood count: " << nogoods.size() << std::endl;
            switch (restarting_search(current, domains_copy, backtracks_until_restart))
            {
            case Search::Done:
//                std::cout << "done!" << std::endl;
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

        if (arguments.verbose)
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

        if (arguments.position_shuffle)
            position_shuffle(possible_values);

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

                auto new_domains = filter_domains(domains, v, w, arguments.directed || arguments.edge_labelled);
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
    MCS(Graph & g0, Graph & g1)
        : g0(g0), g1(g1), watches(g0.n, g1.n), vtx_current_assignment(g0.n) {}

    vector<Assignment> run() {
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

        if (RESTARTS) {
            run_with_restarts(current, domains);
        } else {
            search(current, domains);
        }

        return incumbent;
    }
};

vector<int> calculate_degrees(const Graph & g) {
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

int sum(const vector<int> & vec) {
    return std::accumulate(std::begin(vec), std::end(vec), 0);
}

int main(int argc, char** argv) {
    argp_parse(&argp, argc, argv, 0, 0, 0);

    char format = arguments.dimacs ? 'D' : arguments.lad ? 'L' : 'B';
    struct Graph g0 = readGraph(arguments.filename1, format, arguments.directed,
            arguments.edge_labelled, arguments.vertex_labelled);
    struct Graph g1 = readGraph(arguments.filename2, format, arguments.directed,
            arguments.edge_labelled, arguments.vertex_labelled);

    std::thread timeout_thread;
    std::mutex timeout_mutex;
    std::condition_variable timeout_cv;
    abort_due_to_timeout.store(false);
    bool aborted = false;

    if (0 != arguments.timeout) {
        timeout_thread = std::thread([&] {
                auto abort_time = std::chrono::steady_clock::now() + std::chrono::seconds(arguments.timeout);
                {
                    /* Sleep until either we've reached the time limit,
                     * or we've finished all the work. */
                    std::unique_lock<std::mutex> guard(timeout_mutex);
                    while (! abort_due_to_timeout.load()) {
                        if (std::cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                            /* We've woken up, and it's due to a timeout. */
                            aborted = true;
                            break;
                        }
                    }
                }
                abort_due_to_timeout.store(true);
                });
    }

    auto start = std::chrono::steady_clock::now();

    vector<int> g0_deg = calculate_degrees(g0);
    vector<int> g1_deg = calculate_degrees(g1);

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

    vector<Assignment> solution = MCS(g0_sorted, g1_sorted).run();

    // Convert to indices from original, unsorted graphs
    for (auto& vtx_pair : solution) {
        vtx_pair.v = vv0[vtx_pair.v];
        vtx_pair.w = vv1[vtx_pair.w];
    }

    auto stop = std::chrono::steady_clock::now();
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

    /* Clean up the timeout thread */
    if (timeout_thread.joinable()) {
        {
            std::unique_lock<std::mutex> guard(timeout_mutex);
            abort_due_to_timeout.store(true);
            timeout_cv.notify_all();
        }
        timeout_thread.join();
    }

    if (!check_sol(g0, g1, solution))
        fail("*** Error: Invalid solution\n");

    cout << "Solution size " << solution.size() << std::endl;
    for (int i=0; i<g0.n; i++)
        for (unsigned int j=0; j<solution.size(); j++)
            if (solution[j].v == i)
                cout << "(" << solution[j].v << " -> " << solution[j].w << ") ";
    cout << std::endl;

    cout << "Nodes:                      " << nodes << endl;
    cout << "CPU time (ms):              " << time_elapsed << endl;
    if (aborted)
        cout << "TIMEOUT" << endl;
}

