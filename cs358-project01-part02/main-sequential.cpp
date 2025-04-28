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

	// SEQUENTIAL SOLUTION

	int start_vertex = wg.start_vertex();
	// int num_vertices = wg.num_vertices();

	// need to make a list of 'visited'
	std::set<int> visited;
	std::queue<int> next;
	int c = 0;

	visited.insert(start_vertex);
	next.push(start_vertex);

	while(!next.empty()) {
		int v = next.front();
		next.pop();

		vector<int> neighbors = wg.do_work(v);

		c++;
		if (c % 50 == 0) {
			cout << '#';
		}

		for(int i : neighbors) {
			if (!visited.count(i)) {
				visited.insert(i);
				next.push(i);
			}
		}

	}

	cout << endl;

  

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
