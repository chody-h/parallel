#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

typedef enum {false, true} bool;

double When();
int Compare(const void*, const void*);
int ReverseCompare(const void*, const void*);

int main(int argc, char *argv[])
{
	int* list, num_elements;
	int size, max_capacity, dim, i, j, nrecv;
	int root, partner, pivot;
	int ipiv, isend, nsend;

	double starttime;
	int nproc, iproc, v_iproc, color;

	MPI_Request request;
	MPI_Status status;
	MPI_Comm mycomm;

	num_elements = atoi(argv[1]);

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	dim = (int)(log((double)nproc)/log(2.0));
	size = num_elements;
	max_capacity = size * nproc;

	// initialize list with random numbers [1, 99]
	list = malloc(max_capacity * sizeof(int));
	srand(time(NULL)+iproc*nproc);
	for (i = 0; i < num_elements; i++) 
	{
		list[i] = (rand() % 99) + 1;
		// fprintf(stderr, "(%d) %d\n", iproc, list[i]);
	}

	// quicksort it
	qsort(list, size, sizeof(int), Compare);

	// print lists
	// if (iproc == 0) 
	// 	fprintf(stderr, "\nStarting list:\n");
	// for (i = 0; i < nproc; i++) 
	// {
	// 	if (iproc == i)
	// 	{
	// 		fprintf(stderr, "(%d):  ", iproc);
	// 		for (j = 0; j < size; j++) 
	// 		{
	// 			fprintf(stderr, "%d ", list[j]);
	// 		}
	// 		fprintf(stderr, "\n");
	// 	}
	// 	MPI_Barrier(MPI_COMM_WORLD);
	// }

	starttime = When();

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
	for (i = dim-1; i >= 0; i--) 
	{
		// push or grab the pivot
		if (iproc == 0)
		{
			ipiv = (size-1)/2;
			pivot = list[ipiv];
		}
		MPI_Bcast(&pivot, 1, MPI_INTEGER, root, mycomm);

		// calculate who my partner is
		partner = v_iproc ^ (1 << i);

		// determine what to send - ties go to lower iproc
		if (v_iproc < partner)
		{
			// this is the index of the first one to send
			isend = 0;
			while (isend < size && list[isend] <= pivot) isend++;
			MPI_Isend(&(list[isend]), size-isend, MPI_INTEGER, partner, 0, mycomm, &request);
			size -= (size-isend);
			MPI_Recv(&(list[size]), max_capacity-size, MPI_INTEGER, partner, 0, mycomm, &status);
			MPI_Get_count(&status, MPI_INTEGER, &nrecv);
			size += nrecv;
		}
		else
		{
			// this is the count of how many to send
			isend = size-1;
			while (isend >= 0 && list[isend] > pivot) isend--;
			isend++;
			MPI_Isend(list, isend, MPI_INTEGER, partner, 0, mycomm, &request);
			qsort(list, size, sizeof(int), ReverseCompare);
			size -= isend;
			MPI_Recv(&(list[size]), max_capacity-size, MPI_INTEGER, partner, 0, mycomm, &status);
			MPI_Get_count(&status, MPI_INTEGER, &nrecv);
			size += nrecv;
		}

		// sort
		qsort(list, size, sizeof(int), Compare);

		color = (v_iproc >= pow(2, i));
		MPI_Comm_split(mycomm, color, iproc, &mycomm);
		MPI_Comm_rank(mycomm, &v_iproc);

		// fprintf(stderr, "(%d) pivot: %d\tindex: %d\n", iproc, pivot, isend);
	}

	if (iproc == 0) 
		fprintf(stderr, "Completed in (%f) seconds.\n", When()-starttime);
	MPI_Barrier(MPI_COMM_WORLD);

	// print lists
	// if (iproc == 0) 
	// 	fprintf(stderr, "\nSorted list:\n");
	// for (i = 0; i < nproc; i++) 
	// {
	// 	if (iproc == i)
	// 	{
	// 		for (j = 0; j < size; j++) 
	// 		{
	// 			fprintf(stderr, "%d ", list[j]);
	// 		}
	// 	}
	// 	MPI_Barrier(MPI_COMM_WORLD);
	// }
	// if (iproc == 0)
	// 	fprintf(stderr, "\n");

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

int Compare(const void* a, const void* b)
{
   return ( *(int*)a - *(int*)b );
}

int ReverseCompare(const void* a, const void* b)
{
	return ( *(int*)b - *(int*)a );
}