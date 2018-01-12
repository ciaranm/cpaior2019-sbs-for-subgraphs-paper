/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "lad.hh"
#include "dimacs.hh"
#include "sequential.hh"
#include "parallel.hh"

#include <boost/program_options.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

#include <unistd.h>

namespace po = boost::program_options;

using std::atomic;
using std::boolalpha;
using std::cerr;
using std::condition_variable;
using std::cout;
using std::cv_status;
using std::endl;
using std::exception;
using std::function;
using std::localtime;
using std::make_pair;
using std::mutex;
using std::put_time;
using std::string;
using std::thread;
using std::unique_lock;

using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

/* Helper: return a function that runs the specified algorithm, dealing
 * with timing information and timeouts. */
template <typename Result_, typename Params_, typename Data_>
auto run_this_wrapped(const function<Result_ (const Data_ &, const Params_ &)> & func)
    -> function<Result_ (const Data_ &, Params_ &, bool &, int)>
{
    return [func] (const Data_ & data, Params_ & params, bool & aborted, int timeout) -> Result_ {
        /* For a timeout, we use a thread and a timed CV. We also wake the
         * CV up if we're done, so the timeout thread can terminate. */
        thread timeout_thread;
        mutex timeout_mutex;
        condition_variable timeout_cv;
        atomic<bool> abort;
        abort.store(false);
        params.abort = &abort;
        if (0 != timeout) {
            timeout_thread = thread([&] {
                    auto abort_time = steady_clock::now() + seconds(timeout);
                    {
                        /* Sleep until either we've reached the time limit,
                         * or we've finished all the work. */
                        unique_lock<mutex> guard(timeout_mutex);
                        while (! abort.load()) {
                            if (cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                                /* We've woken up, and it's due to a timeout. */
                                aborted = true;
                                break;
                            }
                        }
                    }
                    abort.store(true);
                    });
        }

        /* Start the clock */
        params.start_time = steady_clock::now();
        auto result = func(data, params);

        /* Clean up the timeout thread */
        if (timeout_thread.joinable()) {
            {
                unique_lock<mutex> guard(timeout_mutex);
                abort.store(true);
                timeout_cv.notify_all();
            }
            timeout_thread.join();
        }

        return result;
    };
}

/* Helper: return a function that runs the specified algorithm, dealing
 * with timing information and timeouts. */
template <typename Result_, typename Params_, typename Data_>
auto run_this(Result_ func(const Data_ &, const Params_ &)) -> function<Result_ (const Data_ &, Params_ &, bool &, int)>
{
    return run_this_wrapped(function<Result_ (const Data_ &, const Params_ &)>(func));
}

auto main(int argc, char * argv[]) -> int
{
    auto subgraph_isomorphism_algorithms = {
        make_pair( string{ "simple" },                       sequential_subgraph_isomorphism ),
        make_pair( string{ "restarting" },                   sequential_subgraph_isomorphism ),
        make_pair( string{ "parallel" },                     parallel_subgraph_isomorphism ),
        make_pair( string{ "customisable-sequential" },      sequential_subgraph_isomorphism ),
        make_pair( string{ "customisable-parallel" },        parallel_subgraph_isomorphism )
    };

    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                         "Display help information")
            ("timeout",            po::value<int>(),         "Abort after this many seconds")
            ("dimacs",                                       "Read DIMACS format instead of LAD")
            ("induced",                                      "Solve the induced version");

        po::options_description parallel_options{ "Options for parallel algorithms" };
        parallel_options.add_options()
            ("threads",            po::value<int>(),         "Number of threads to use");

        po::options_description custom_options{ "Options for customisable algorithms (not all combinations make sense)" };
        custom_options.add_options()
            ("restarts",                                     "Use restarts")
            ("dds",                                          "Use dds")
            ("shuffle",                                      "Use shuffling")
            ("softmax-shuffle",                              "Use softmax shuffling")
            ("antiheuristic",                                "Use antiheuristic")
            ("input-order",                                  "Use input order")
            ("luby-multiplier",      po::value<unsigned>(),  "Specify a Luby multiplier")
            ("geometric-multiplier", po::value<double>(),    "Specify a Geometric multiplier")
            ("geometric-start",      po::value<unsigned>(),  "Specify geometric start value")
            ("goods",                                        "No nogoods")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm",    "Specify which algorithm to use (start with \"restarting\" or \"simple\")")
            ("pattern-file", "Specify the pattern file (LAD format)")
            ("target-file",  "Specify the target file (LAD format)")
            ;

        all_options.add(display_options);
        all_options.add(parallel_options);
        all_options.add(custom_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("pattern-file", 1)
            .add("target-file", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            cout << "Usage: " << argv[0] << " [options] algorithm pattern target" << endl;
            cout << endl;
            cout << display_options << endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || ! options_vars.count("pattern-file") || ! options_vars.count("target-file")) {
            cout << "Usage: " << argv[0] << " [options] algorithm pattern target" << endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = subgraph_isomorphism_algorithms.begin(), algorithm_end = subgraph_isomorphism_algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (algorithm->first == options_vars["algorithm"].as<string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            cerr << "Unknown algorithm " << options_vars["algorithm"].as<string>() << ", choose from:";
            for (auto a : subgraph_isomorphism_algorithms)
                cerr << " " << a.first;
            cerr << endl;
            return EXIT_FAILURE;
        }

        // Some sanity checking
        if (options_vars["algorithm"].as<string>() == "customisable-parallel") {
            if (! options_vars.count("restarts")
                    || options_vars.count("softmax-shuffle")
                    || ! options_vars.count("input-order")
                    || options_vars.count("dds")
                    || options_vars.count("shuffle")
                    || options_vars.count("goods")
                    || options_vars.count("antiheuristic")) {
                cerr << "Parallel algorithm currently requires --restarts --input-order" << endl;
                cerr << "  --softmax-shuffle, and cannot use any of --dds --shuffle " << endl;
                cerr << "  --softmax-shuffle --goods --antiheuristic" << endl;
                return EXIT_FAILURE;
            }
        }

        /* Figure out what our options should be. */
        Params params;

        params.induced = options_vars.count("induced");

        if (0 == options_vars["algorithm"].as<std::string>().compare(0, 13, "customisable-", 0, 13)) {
            params.restarts = options_vars.count("restarts");
            params.dds = options_vars.count("dds");
            params.shuffle = options_vars.count("shuffle");
            params.softmax_shuffle = options_vars.count("softmax-shuffle");
            params.antiheuristic = options_vars.count("antiheuristic");
            params.input_order = options_vars.count("input-order");
            params.goods = options_vars.count("goods");

            if (options_vars.count("luby-multiplier"))
                params.luby_multiplier = options_vars["luby-multiplier"].as<unsigned>();
            if (options_vars.count("geometric-multiplier"))
                params.geometric_multiplier = options_vars["geometric-multiplier"].as<double>();
            if (options_vars.count("geometric-start"))
                params.geometric_start = options_vars["geometric-start"].as<unsigned>();
        }
        else if (options_vars["algorithm"].as<std::string>() == "parallel") {
            params.restarts = true;
            params.softmax_shuffle = true;
            params.input_order = true;
        }
        else if (options_vars["algorithm"].as<std::string>() == "restarting") {
            params.restarts = true;
            params.softmax_shuffle = true;
            params.input_order = true;
        }
        else {
            params.input_order = false;
        }

        if (options_vars.count("threads"))
            params.n_threads = options_vars["threads"].as<unsigned>();

        char hostname_buf[255];
        if (0 == gethostname(hostname_buf, 255))
            cout << "hostname = " << string(hostname_buf) << endl;
        cout << "commandline =";
        for (int i = 0 ; i < argc ; ++i)
            cout << " " << argv[i];
        cout << endl;

        auto started_at = system_clock::to_time_t(system_clock::now());
        cout << "started_at = " << put_time(localtime(&started_at), "%F %T") << endl;

        /* Read in the graphs */
        auto graphs = make_pair(
            (options_vars.count("dimacs") ? read_dimacs : read_lad)(options_vars["pattern-file"].as<string>()),
            (options_vars.count("dimacs") ? read_dimacs : read_lad)(options_vars["target-file"].as<string>()));

        cout << "pattern_file = " << options_vars["pattern-file"].as<std::string>() << endl;
        cout << "target_file = " << options_vars["target-file"].as<std::string>() << endl;

        /* Do the actual run. */
        bool aborted = false;
        auto result = run_this(algorithm->second)(
                graphs,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

        cout << "status = ";
        if (aborted)
            cout << "aborted";
        else if (! result.isomorphism.empty())
            cout << "true";
        else
            cout << "false";
        cout << endl;

        cout << "nodes = " << result.nodes << endl;

        if (! result.isomorphism.empty()) {
            cout << "mapping = ";
            for (auto v : result.isomorphism)
                cout << "(" << v.first << " -> " << v.second << ") ";
            cout << endl;
        }

        cout << "runtime = " << overall_time.count() << endl;

        for (const auto & s : result.extra_stats)
            cout << s << endl;

        if (! result.isomorphism.empty()) {
            for (int i = 0 ; i < graphs.first.size() ; ++i) {
                for (int j = 0 ; j < graphs.first.size() ; ++j) {
                    if (params.induced || graphs.first.adjacent(i, j)) {
                        if (graphs.first.adjacent(i, j) !=
                                graphs.second.adjacent(result.isomorphism.find(i)->second, result.isomorphism.find(j)->second)) {
                            cerr << "Oops! not an isomorphism: " << i << ", " << j << endl;
                            return EXIT_FAILURE;
                        }
                    }
                }
            }
        }

        return EXIT_SUCCESS;
    }
    catch (const po::error & e) {
        cerr << "Error: " << e.what() << endl;
        cerr << "Try " << argv[0] << " --help" << endl;
        return EXIT_FAILURE;
    }
    catch (const exception & e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}

