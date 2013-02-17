#include <stdlib.h>
#include <stdio.h>

#include "common.h"

// calculates \sum_K v_i'*A*v_i
double dosum(Matrix A, Matrix v)
{
  double alpha=0;
  Matrix temp = createMatrix(A->rows, max_threads());
#pragma omp parallel for schedule(static) reduction(+:alpha)
  for( int i=0;i<v->cols;++i ) {
    MxV(temp->col[get_thread()],A,v->col[i]);
    alpha += innerproduct(temp->col[get_thread()],v->col[i]);
  }
  freeMatrix(temp);
#ifdef HAVE_MPI
  double s2=alpha;
  MPI_Allreduce(&s2, &alpha, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
#endif

  return alpha;
}

int main(int argc, char** argv)
{
  int rank, size;
  init_app(argc, argv, &rank, &size);

  if (argc < 3) {
    printf("need two parameters, the matrix size and the number of vectors\n");
    close_app();
    return 1;
  }
  int N=atoi(argv[1]);
  int K=atoi(argv[2]);

  Matrix A = createMatrix(N,N);
  // identity matrix
  for (int i=0;i<N;++i)
    A->data[i][i] = 1.0;
  
  int* ofs;
  int* cols;
  splitVector(K, size, &cols, &ofs);
  Matrix v = createMatrix(N,cols[rank]);
  // fill with column number
  for (int i=0;i<cols[rank];++i)
    for (int j=0;j<N;++j)
      v->data[i][j] = i+ofs[rank];

  double time = WallTime();
  double sum = dosum(A,v);

  if (rank == 0) {
    printf("sum: %f\n", sum);
    printf("elapsed: %f\n", WallTime()-time);
  }

  freeMatrix(v);
  freeMatrix(A);
  free(cols);
  free(ofs);

  close_app();
  return 0;
}
