#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

#define NUM_ELEMENTS	3

typedef enum {false, true} bool;

double When();
int CompareInt(const void*, const void*);

int main(int argc, char *argv[])
{
	int* list;
	int size, dim, i, j, nrecv;
	int root, partner, pivot;
	int ipiv, isend, nsend;

	double starttime;
	int nproc, iproc, v_iproc;

	MPI_Request request;
	MPI_Status status;
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
		// fprintf(stderr, "(%d) %d\n", iproc, list[i]);
	}
	size = NUM_ELEMENTS;

	// quicksort it
	qsort(list, size, sizeof(int), CompareInt);

	// while not done
		// hypercube calculations
			// root broadcasts pivot
			// figure out who to communicate with
		// send half of list
		// receive half of list
		// merge lists
	MPI_Comm_split(MPI_COMM_WORLD, 0, iproc, &mycomm);
	root = 0;
	v_iproc = iproc;
	// for (i = dim-1; i != 0; i--) 
	// {
		// push or grab the pivot
		if (iproc == 0)
		{
			ipiv = (size-1)/2;
			pivot = list[ipiv];
		}
		MPI_Bcast(&pivot, 1, MPI_INTEGER, root, mycomm);

		// calculate who my partner is
		partner = v_iproc ^ (1 << (dim-1));

		// determine what to send - ties go to lower iproc
		if (v_iproc < partner)
		{
			// this is the index of the first one to send
			isend = 0;
			while (isend < size && list[isend] <= pivot) isend++;
			MPI_Isend(list[isend], size-isend, MPI_INTEGER, partner, 0, mycomm, &request);
			size -= (size-isend);
			MPI_Recv(list[size], NUM_ELEMENTS-size, MPI_INTEGER, partner, 0, mycomm, &status);
			MPI_Get_count(&status, MPI_INTEGER, &nrecv);
			size += nrecv;
		}
		else
		{
			// this is the count of how many to send
			isend = size-1;
			while (isend >= 0 && list[isend] > pivot) isend--;
			isend++;
			MPI_Isend(list[0], isend, MPI_INTEGER, partner, 0, mycomm, &request);
			size -= isend;
			MPI_Recv(list[size], NUM_ELEMENTS-size, MPI_INTEGER, partner, 0, mycomm, &status);
			MPI_Get_count(&status, MPI_INTEGER, &nrecv);
			size += nrecv;
		}

		// sort
		qsort(list, size, sizeof(int), CompareInt);

		fprintf(stderr, "(%d) pivot: %d\tindex: %d\n", iproc, pivot, isend);
	// }

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