#include <atomic>

extern std::atomic<bool> abort_due_to_timeout;

enum class Heuristic {
    min_max,
    min_product
};

struct Params {
    bool quiet;
    bool verbose;
    bool restarts;
    bool biased_shuffle;
    bool connected;
    bool directed;
    bool edge_labelled;
    bool vertex_labelled;
    Heuristic heuristic;
};

struct Assignment {
    int v;
    int w;

    auto operator==(const Assignment & other) const -> bool
    {
        return v==other.v && w==other.w;
    }
};

auto solve_mcs(Graph & g0, Graph & g1, Params params)
		-> std::pair<std::vector<Assignment>, unsigned long long>;
