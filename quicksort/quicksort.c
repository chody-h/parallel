#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

#define NUM_ELEMENTS	3

typedef enum {false, true} bool;

double When();
int CompareInt(const void*, const void*);
int MergeLists(int*, int, int*, int);

int main(int argc, char *argv[])
{
	int* list;
	int size, i, dim;
	int root, partner, pivot, piv_idx;

	double starttime;
	int nproc, iproc, v_iproc;

	MPI_Request request;
	MPI_Comm mycomm;

	MPI_Init(&argc, &argv);
	starttime = When();

	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	// fprintf(stderr, "\nHello from thread %d (of %d)!\n", iproc, nproc);

	// initialize list with random numbers [1, 99]
	dim = (int)(log((double)nproc)/log(2.0));
	fprintf(stderr, "\nHello from thread %d (of %d)!\nDimension: %d\n", iproc, nproc, dim);
	size = nproc * NUM_ELEMENTS;
	list = malloc(size * sizeof(int));
	srand(time(NULL)+iproc);
	for (i = 0; i < size; i++) 
	{
		if (i < NUM_ELEMENTS)
		{
			list[i] = (rand() % 99) + 1;
		}
		else
		{
			list[i] = 0;
		}
		// fprintf(stderr, "(%d) %d\n", iproc, list[i]);
	}
	size = NUM_ELEMENTS;

	// quicksort it
	qsort(list, NUM_ELEMENTS, sizeof(int), CompareInt);

	// while not done
		// hypercube calculations
			// root broadcasts pivot
			// figure out who to communicate with
		// send half of list
		// receive half of list
		// merge lists
	MPI_Comm_split(MPI_COMM_WORLD, 0, iproc, &mycomm);
	root = 0;
	for (i = 0; i < dim; i++) 
	{
		if (iproc == 0)
		{
			piv_idx = (size-1)/2;
			pivot = list[piv_idx];
		}
		MPI_Bcast(&pivot, 1, MPI_INTEGER, root, mycomm);
	}

	// print lists
	for (i = 0; i < size; i++) 
	{
		fprintf(stderr, "(%d) %d\n", iproc, list[i]);
	}

	free(list);
	MPI_Finalize();

	return 0;
}

double When()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

int CompareInt(const void* a, const void* b)
{
   return ( *(int*)a - *(int*)b );
}

// returns the size of the new list. new list is stored in A
int MergeLists(int* a, int length_a, int* b, int length_b)
{
	return 0;
}