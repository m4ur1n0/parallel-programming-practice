/* mm.cpp */

//
// Matrix sum implementation, summing the contents of an 
// NxN matrix.
//
#include <iostream>
#include <string>
#include <sys/sysinfo.h>

#include "alloc2D.h"
#include "sum.h"

using namespace std;


//
// MatrixSum:
//
// Computes and returns the sum of an NxN matrix.
//
double MatrixSum(double** M, int N, int T)
{
  //
  // Setup:
  //
  cout << "Num cores: " << get_nprocs() << endl;
  cout << "Num threads: " << T << endl;
  cout << endl;

  double sum = 0.0;


  // omp_lock_t sum_lock;
  // omp_init_lock(&sum_lock); // deinit later, but i missed how

  double* sums = new double[T];
  for (int i = 0; i < T; i++) {
    sums[i] = 0.0;
  }
  //
  // For every row and column, add to sum:
  //
  #pragma omp parallel for num_threads(T)
  for (int r = 0; r < N; r++)
  {
    
    int id = omp_get_thread_num();

    for (int c = 0; c < N; c++)
    {
      sums[id] += M[r][c];
    }
  }

  // now join the sumi vars we created
  for (int i = 0; i < T; i++) {
    sum += sums[i];
  }

  delete[] sums;
  
  return sum;
}
