#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include <unistd.h>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "match.hpp"
#include "argloader.hpp"
#include "argraph.hpp"
#include "argedit.hpp"
#include "nodesorter.hpp"
#include "probability_strategy.hpp"
#include "vf3_sub_state.hpp"
#include "nodesorter.hpp"
#include "nodeclassifier.hpp"

template<> long long VF3SubState<int,int,Empty,Empty>::instance_count = 0;
static long long state_counter = 0;

template<typename State>
bool match_with_timeout(State &s, int *pn, node_id c1[], node_id c2[], unsigned long long & nodes,
        std::atomic<bool> & abort) {
    if (abort)
        return false;

    ++nodes;

    if (s.IsGoal())
    {
        *pn = s.CoreLen();
        s.GetCoreSet(c1, c2);
        return true;
    }

    if (s.IsDead())
        return false;

    node_id n1 = NULL_NODE, n2 = NULL_NODE;
    bool found = false;

    while (!found && s.NextPair(&n1, &n2, n1, n2))
    {
        if (s.IsFeasiblePair(n1, n2))
        {
            State s1(s);
            s1.AddPair(n1, n2);
            found = match_with_timeout(s1, pn, c1, c2, nodes, abort);
        }
    }

    return found;
}

int main(int argc, char** argv)
{

  char *pattern, *target;

  state_counter = 0;
  int n = 0;

  if (argc != 4)
  {
	  std::cout << "Usage: " << argv[0] << " pattern target timeout \n";
	  return -1;
  }
  
  pattern = argv[1];
  target = argv[2];
  
  std::ifstream graphInPat(pattern);
  std::ifstream graphInTarg(target);

  StreamARGLoader<int, Empty> pattloader(graphInPat);
  StreamARGLoader<int, Empty> targloader(graphInTarg);

  ARGraph<int, Empty> patt_graph(&pattloader);
  ARGraph<int, Empty> targ_graph(&targloader);
  
  int nodes1, nodes2;
  nodes1 = patt_graph.NodeCount();
  nodes2 = targ_graph.NodeCount();
  node_id *n1, *n2;
  n1 = new node_id[nodes1];
  n2 = new node_id[nodes2];

  auto start_time = std::chrono::steady_clock::now();
  NodeClassifier<int, Empty> classifier(&targ_graph);
  NodeClassifier<int, Empty> classifier2(&patt_graph, classifier);
  std::vector<int> class_patt = classifier2.GetClasses();
  std::vector<int> class_targ = classifier.GetClasses();

  VF3NodeSorter<int, Empty, SubIsoNodeProbability<int, Empty> > sorter(&targ_graph);
  std::vector<node_id> sorted = sorter.SortNodes(&patt_graph);

  VF3SubState<int, int, Empty, Empty>s0(&patt_graph, &targ_graph, class_patt.data(),
          class_targ.data(), classifier.CountClasses(), sorted.data());

  std::thread timeout_thread;
  std::mutex timeout_mutex;
  std::condition_variable timeout_cv;
  std::atomic<bool> abort;
  abort.store(false);
  bool aborted = false;
  timeout_thread = std::thread([&] {
          auto abort_time = std::chrono::steady_clock::now() + std::chrono::seconds(atoi(argv[3]));
          {
              /* Sleep until either we've reached the time limit,
               * or we've finished all the work. */
              std::unique_lock<std::mutex> guard(timeout_mutex);
              while (! abort.load()) {
                  if (std::cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                      /* We've woken up, and it's due to a timeout. */
                      aborted = true;
                      break;
                      }
                  }
          }
          abort.store(true);
          });

  unsigned long long nodes = 0;
  bool found = match_with_timeout<VF3SubState<int, int, Empty, Empty> >(s0, &n, n1, n2, nodes, abort);
  auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

  {
      std::unique_lock<std::mutex> guard(timeout_mutex);
      abort.store(true);
      timeout_cv.notify_all();
  }
  timeout_thread.join();

  if (found)
      std::cout << "true " << nodes << " " << time << std::endl;
  else if (aborted)
      std::cout << "aborted " << nodes << " " << time << std::endl;
  else
      std::cout << "false " << nodes << " " << time << std::endl;

  return 0;
}

