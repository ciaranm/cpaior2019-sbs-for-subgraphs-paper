/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "formats/read_file_format.hh"
#include "solver.hh"

#ifdef WITH_MPI
#    include "mpi_solver.hh"
#    include <boost/mpi/environment.hpp>
#    include <boost/mpi/communicator.hpp>
#    include <boost/mpi/collectives.hpp>
#else
#    include "solver.hh"
#    include "parallel_solver.hh"
#endif

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
#include <vector>

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
using std::move;
using std::mutex;
using std::put_time;
using std::string;
using std::thread;
using std::unique_lock;
using std::vector;

using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

#ifdef WITH_MPI
using boost::mpi::gather;
#endif

auto main(int argc, char * argv[]) -> int
{
#ifdef WITH_MPI
    boost::mpi::environment mpi_env(boost::mpi::threading::level::multiple);
    boost::mpi::communicator mpi_comm;
#endif

    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                         "Display help information")
            ("timeout",            po::value<int>(),         "Abort after this many seconds")
            ("format",             po::value<std::string>(), "Specify input file format (auto, lad, labelledlad, dimacs)")
            ("pattern-format",     po::value<std::string>(), "Specify input file format just for the pattern graph")
            ("target-format",      po::value<std::string>(), "Specify input file format just for the target graph")
            ("induced",                                      "Solve the induced version")
            ("enumerate",                                    "Count the number of solutions");

        po::options_description configuration_options{ "Advanced configuration options" };
        configuration_options.add_options()
            ("dds",                                          "Use DDS (forces degree value-ordering, no restarts or nogoods)")
            ("presolve",                                     "Try presolving (hacky, experimental, possibly useful for easy instances")
            ("nogood-size-limit",  po::value<int>(),         "Maximum size of nogood to generate (0 disables nogoods")
            ("restarts-constant",  po::value<int>(),         "How often to perform restarts (0 disables restarts)")
            ("geometric-restarts", po::value<double>(),      "Use geometric restarts with the specified multiplier (default is Luby)")
            ("value-ordering",     po::value<std::string>(), "Specify value-ordering heuristic (biased / degree / antidegree / random)");
        display_options.add(configuration_options);

        po::options_description parallel_options{ "Parallelism options" };
        parallel_options.add_options()
            ("threads",            po::value<int>(),         "Number of threads to use (1 for sequential)")
            ("triggered-restarts",                           "Trigger restarts using the first thread");
            ;
        display_options.add(parallel_options);

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("pattern-file", "Specify the pattern file")
            ("target-file",  "Specify the target file")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
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
            cout << "Usage: " << argv[0] << " [options] pattern target" << endl;
            cout << endl;
            cout << display_options << endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("pattern-file") || ! options_vars.count("target-file")) {
            cout << "Usage: " << argv[0] << " [options] pattern target" << endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        Params params;

        if (options_vars.count("threads"))
            params.n_threads = options_vars["threads"].as<int>();

#ifndef WITH_MPI
        auto algorithm = (1 == params.n_threads) ? sequential_subgraph_isomorphism : parallel_subgraph_isomorphism;
#endif

        params.induced = options_vars.count("induced");
        params.enumerate = options_vars.count("enumerate");
        params.presolve = options_vars.count("presolve");
        params.dds = options_vars.count("dds");
        params.triggered_restarts = options_vars.count("triggered-restarts");

        if (options_vars.count("nogood-size-limit"))
            params.nogood_size_limit = options_vars["nogood-size-limit"].as<int>();
        if (options_vars.count("restarts-constant"))
            params.restarts_constant = options_vars["restarts-constant"].as<int>();
        if (options_vars.count("geometric-restarts"))
            params.geometric_multiplier = options_vars["geometric-restarts"].as<double>();

        if (options_vars.count("value-ordering")) {
            std::string value_ordering_heuristic = options_vars["value-ordering"].as<std::string>();
            if (value_ordering_heuristic == "biased")
                params.value_ordering_heuristic = ValueOrdering::Biased;
            else if (value_ordering_heuristic == "degree")
                params.value_ordering_heuristic = ValueOrdering::Degree;
            else if (value_ordering_heuristic == "antidegree")
                params.value_ordering_heuristic = ValueOrdering::AntiDegree;
            else if (value_ordering_heuristic == "random")
                params.value_ordering_heuristic = ValueOrdering::Random;
            else {
                cerr << "Unknown value-ordering heuristic '" << value_ordering_heuristic << "'" << endl;
                return EXIT_FAILURE;
            }
        }

        bool do_output_here = true;
#ifdef WITH_MPI
        do_output_here = mpi_comm.rank() == 0;
#endif

        if (do_output_here) {
            char hostname_buf[255];
            if (0 == gethostname(hostname_buf, 255))
                cout << "hostname = " << string(hostname_buf) << endl;
            cout << "commandline =";
            for (int i = 0 ; i < argc ; ++i)
                cout << " " << argv[i];
            cout << endl;

            auto started_at = system_clock::to_time_t(system_clock::now());
            cout << "started_at = " << put_time(localtime(&started_at), "%F %T") << endl;
        }

        /* Read in the graphs */
        string default_format_name = options_vars.count("format") ? options_vars["format"].as<string>() : "auto";
        string pattern_format_name = options_vars.count("pattern-format") ? options_vars["pattern-format"].as<string>() : default_format_name;
        string target_format_name = options_vars.count("target-format") ? options_vars["target-format"].as<string>() : default_format_name;
        auto graphs = make_pair(
            read_file_format(pattern_format_name, options_vars["pattern-file"].as<string>()),
            read_file_format(target_format_name, options_vars["target-file"].as<string>()));

        if (do_output_here) {
            cout << "pattern_file = " << options_vars["pattern-file"].as<std::string>() << endl;
            cout << "target_file = " << options_vars["target-file"].as<std::string>() << endl;
        }

        int timeout = options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0;

        /* For a timeout, we use a thread and a timed CV. We also wake the
         * CV up if we're done, so the timeout thread can terminate. */
        bool aborted = false;
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

#ifdef WITH_MPI
        auto result = mpi_subgraph_isomorphism(mpi_comm, graphs, params);
#else
        auto result = algorithm(graphs, params);
#endif

        /* Stop the clock. */
        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

        /* Clean up the timeout thread */
        if (timeout_thread.joinable()) {
            {
                unique_lock<mutex> guard(timeout_mutex);
                abort.store(true);
                timeout_cv.notify_all();
            }
            timeout_thread.join();
        }

#ifdef WITH_MPI
        /* Merge resulty things */
        Result combined_result;
        vector<Result> all_results;
        gather(mpi_comm, result, all_results, 0);
        if (do_output_here)
            for (auto & r : all_results)
                combined_result.merge("", r);
        result = move(combined_result);
#endif

        if (do_output_here) {
            cout << "status = ";
            if (aborted)
                cout << "aborted";
            else if ((! result.isomorphism.empty()) || (params.enumerate && result.solution_count > 0))
                cout << "true";
            else
                cout << "false";
            cout << endl;

            if (params.enumerate)
                cout << "solution_count = " << result.solution_count << endl;

            cout << "nodes = " << result.nodes << endl;
            cout << "propagations = " << result.propagations << endl;

            if (! result.isomorphism.empty()) {
                cout << "mapping = ";
                for (auto v : result.isomorphism)
                    cout << "(" << v.first << " -> " << v.second << ") ";
                cout << endl;
            }

            cout << "runtime = " << overall_time.count() << endl;

            for (const auto & s : result.extra_stats)
                cout << s << endl;
        }

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
    catch (const GraphFileError & e) {
        cerr << "Error: " << e.what() << endl;
        cerr << "Maybe try specifying one of --format, --pattern-format, or --target-format?" << endl;
        return EXIT_FAILURE;
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

