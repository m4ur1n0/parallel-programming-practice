/* main.cpp */

//
// Parallelizes a generic "work graph" where work is randomly
// distributed across the vertices in the graph. Naive 
// parallelization works, but doesn't scale. A much more 
// dynamic solution is needed.
// 
// Usage:
//   work [-?] [-t NumThreads]
//
// Author:
//   theo maurino
//   Northwestern University
// 
// Initial template:
//   Prof. Joe Hummel
//   Northwestern University
//

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <chrono>
#include <random>
#include <sys/sysinfo.h>
#include <omp.h>
#include <set>
#include <queue>
#include <atomic>
#include <mutex>

#include "workgraph.h"

using namespace std;


//
// Globals:
//
static int _numThreads = 1;  // default to sequential execution

//
// Function prototypes:
//
static void ProcessCmdLineArgs(int argc, char* argv[]);

void parallelWork(WorkGraph& wg) {

	// ok we want to
		// distribute work early
		// give each thread a local queue to work thru, which they can refresh atomically to get more tasks
		// only synchronize when necessary (visited set, next queue)

		// i think we want threads to be able to steal work from other threads if they finish earlu (thats the dynamic of it)

	std::queue<int> global_q;
	std::set<int> visited;
	std::vector<std::queue<int>> local_queues(_numThreads); // make one local q per rthread
	std::vector<std::mutex> q_mutexes(_numThreads);

	// std::atomic_flag lock = ATOMIC_FLAG_INIT; // may not need
	std::atomic<bool> done(false);
	// bool done(false);
	std::mutex visited_lock;
	std::atomic<int> work_counter(0);

	int start_v = wg.start_vertex();

	{
		std::lock_guard<std::mutex> lock_visited(visited_lock);
		visited.insert(start_v);
	}
	{
		std::lock_guard<std::mutex> lock_queue(q_mutexes[0]); // do i have to init this?
		local_queues[0].push(start_v);
		work_counter++;
	}

	#pragma omp parallel num_threads(_numThreads)
	{
		int tid = omp_get_thread_num();

		// if (tid == 0) {

		// }

		while (!done) {
			// go thru the local queue first obv
			bool processed_work = false;

			{
				std::lock_guard<std::mutex> lock(q_mutexes[tid]);

				if (!local_queues[tid].empty()) {
					int v = local_queues[tid].front(); // should i make a copy here instead of double memory hit each time? -- maybe i can't write right if i do
					local_queues[tid].pop();

					processed_work = true;

					vector<int> neighbors = wg.do_work(v);

					// lock before adding neighbors to local q
					std::lock_guard<std::mutex> lock_visited(visited_lock);
					int new_work = 0;
					for (int i : neighbors) {
						// for each neighbor, check and mark visited, add to todos
						if (visited.insert(i).second) { // evals to true if its a new addition

							// std::lock_guard<std::mutex> curr_q_lock(q_mutexes[tid]);
							local_queues[tid].push(i); // i do NOT like how many memory hits this is......
							new_work++;
						}
					}

					work_counter.fetch_add(new_work - 1, std::memory_order_relaxed);

				}
			}
			
			if (!processed_work) { // i.e. we have run out of work in local q -- steal from someone else

				bool found_new_work = false;

				for (int victimThread = 0; victimThread < _numThreads; victimThread++) {
					if (victimThread == tid) continue;

					std::unique_lock<std::mutex> victim_lock(
						q_mutexes[victimThread], std::try_to_lock
					);

					if (victim_lock.owns_lock()) {
						std::queue<int>& stealing_from_queue = local_queues[victimThread];
						if (!stealing_from_queue.empty()) {
							// now is when we steal HALF of another thread's work

							int steal_size = stealing_from_queue.size() / 2;
							std::lock_guard<std::mutex> own_q_lock(q_mutexes[tid]);

							while (steal_size-- > 0 && !stealing_from_queue.empty()) {
								int v = stealing_from_queue.front();
								stealing_from_queue.pop();
								local_queues[tid].push(v);
							}

							found_new_work = true;
							break; // get to processing the work we just stole
						}
					}
				}


				// if we go thru all those guys and find nothing, we're done!
				if (!found_new_work) {
					if (work_counter.load() == 0) {
						done = true;
					}
				}

			}
		}

	}


}


//
// main:
//
int main(int argc, char *argv[])
{
	cout << "** Work Graph Application **" << endl;
	cout << endl;

	//
	// Set defaults, process environment & cmd-line args:
	//
	ProcessCmdLineArgs(argc, argv);

	WorkGraph wg;  // NOTE: wg MUST be created in sequential code

	cout << "Graph size:   " << wg.num_vertices() << " vertices" << endl;
	cout << "Start vertex: " << wg.start_vertex() << endl;
	cout << "# of threads: " << _numThreads << endl;
	cout << endl;

	cout << "working";
	cout.flush();

  auto start = chrono::high_resolution_clock::now();


	//
	// TODO: solve all the vertices in the graph. This code just
	// solves the start vertex.
	//

	// SEQUENTIAL

	// int start_vertex = wg.start_vertex();
	// // int num_vertices = wg.num_vertices();

	// // need to make a list of 'visited'
	// std::set<int> visited;
	// std::queue<int> next;
	// int c = 0;

	// visited.insert(start_vertex);
	// next.push(start_vertex);

	// while(!next.empty()) {
	// 	int v = next.front();
	// 	next.pop();

	// 	vector<int> neighbors = wg.do_work(v);

	// 	c++;
	// 	if (c % 50 == 0) {
	// 		cout << '#';
	// 	}

	// 	for(int i : neighbors) {
	// 		if (!visited.count(i)) {
	// 			visited.insert(i);
	// 			next.push(i);
	// 		}
	// 	}

	// }

	// cout << endl;

	// PARALLEL
	parallelWork(wg);

  

	auto stop = chrono::high_resolution_clock::now();
	auto diff = stop - start;
	auto duration = chrono::duration_cast<chrono::milliseconds>(diff);

	cout << endl;
	cout << endl;

	cout << endl;
	cout << "** Done!  Time: " << duration.count() / 1000.0 << " secs" << endl;
	cout << "** Execution complete **" << endl;
  cout << endl;

	return 0;
}


//
// processCmdLineArgs:
//
static void ProcessCmdLineArgs(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{

		if (strcmp(argv[i], "-?") == 0)  // help:
		{
			cout << "**Usage: work [-?] [-t NumThreads]" << endl << endl;
			exit(0);
		}
		else if ((strcmp(argv[i], "-t") == 0) && (i+1 < argc))  // # of threads:
		{
			i++;
			_numThreads = atoi(argv[i]);
		}
		else  // error: unknown arg
		{
			cout << "**Unknown argument: '" << argv[i] << "'" << endl;
			cout << "**Usage: work [-?] [-t NumThreads]" << endl << endl;
			exit(0);
		}

	}//for
}
