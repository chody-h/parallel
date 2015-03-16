#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>	// random 
#include <math.h>		// log
#include <string.h> 	// memcpy

typedef enum {false, true} bool;

double When();
int Compare(const void*, const void*);
int ReverseCompare(const void*, const void*);

int main(int argc, char *argv[])
{
	bool debug;
	int *list, *recv_buf, *temp;
	double size;
	int dim, i, j, nrecv;
	int root, partner, pivot, count;
	int ipiv, isend, nsend;

	double starttime;
	int nproc, iproc, v_iproc, color;

	MPI_Request request;
	MPI_Status status;
	MPI_Comm mycomm;

	size = atof(argv[1]);
	debug = (argc == 3);

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	dim = (int)(log((double)nproc)/log(2.0));

	// initialize list with random numbers [1, 99]
	if (!(list = malloc(sizeof(int) * size))) return -1;
	srand(time(NULL)+iproc*nproc);
	for (i = 0; i < size; i++) 
	{
		list[i] = (rand() % 99) + 1;
		// fprintf(stderr, "(%d) %d\n", iproc, list[i]);
	}

	// quicksort it
	qsort(list, size, sizeof(int), Compare);

	// print lists
	if (debug && iproc == 0) 
		fprintf(stderr, "\nStarting list:\n");
	if (debug)
	{
		for (i = 0; i < nproc; i++) 
		{
			if (iproc == i)
			{
				fprintf(stderr, "(%d):  ", iproc);
				for (j = 0; j < size; j++) 
				{
					fprintf(stderr, "%d ", list[j]);
				}
				fprintf(stderr, "\n");
			}
			MPI_Barrier(MPI_COMM_WORLD);
		}
	}
// int debugnum = 0;
	starttime = When();

/**********************GO!************************/

	mycomm = MPI_COMM_WORLD;
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

		// left hand side
		if (v_iproc < partner)
		{
			// this is the index of the first one to send
			isend = 0;
			while (isend < size && list[isend] <= pivot) isend++;

			// send & receive the count, malloc receive buffer
			count = size-isend;
			MPI_Isend(&count, 1, MPI_INTEGER, partner, 0, mycomm, &request);
			MPI_Recv(&count, 1, MPI_INTEGER, partner, 0, mycomm, &status);
			if (!(recv_buf = malloc(sizeof(int) * count))) return -1;

			// send & receive the data
			MPI_Isend(&(list[isend]), size-isend, MPI_INTEGER, partner, 0, mycomm, &request);
			size -= (size-isend);
			MPI_Recv(recv_buf, count, MPI_INTEGER, partner, 0, mycomm, &status);
		}
		// right hand side
		else
		{
			// this is the count of how many to send
			isend = size-1;
			while (isend >= 0 && list[isend] > pivot) isend--;
			isend++;

			// send & receive the count, malloc receive buffer
			count = isend;
			MPI_Isend(&count, 1, MPI_INTEGER, partner, 0, mycomm, &request);
			MPI_Recv(&count, 1, MPI_INTEGER, partner, 0, mycomm, &status);
			if (!(recv_buf = malloc(sizeof(int) * count))) return -1;

			// send and receive the data
			MPI_Isend(list, isend, MPI_INTEGER, partner, 0, mycomm, &request);
			qsort(list, size, sizeof(int), ReverseCompare);
			size -= isend;
			MPI_Recv(recv_buf, count, MPI_INTEGER, partner, 0, mycomm, &status);
		}

		// combine the original (truncated) and received arrays
		if (!(temp = malloc(sizeof(int) * (size + count)))) return -1;
		memcpy(temp, list, sizeof(int)*size);
		free(list);
		memcpy(temp+(int)size, recv_buf, sizeof(int)*count);
		free(recv_buf);

		list = temp;
		size += count;

		qsort(list, size, sizeof(int), Compare);
// fprintf(stderr, "(%d)debugger: %d\n", iproc, debugnum);

		// divide the hypercube
		color = (v_iproc >= pow(2, i));
		MPI_Comm_split(mycomm, color, 0, &mycomm);
		MPI_Comm_rank(mycomm, &v_iproc);
	}

/**********************STOP!************************/

	if (iproc == 0) 
		fprintf(stderr, "Completed in (%f) seconds.\n", When()-starttime);

	// print lists
	if (debug && iproc == 0) 
		fprintf(stderr, "\nSorted list:\n");
	if (debug)
	{
		for (i = 0; i < nproc; i++) 
		{
			if (iproc == i)
			{
				for (j = 0; j < size; j++) 
				{
					fprintf(stderr, "%d ", list[j]);
				}
			}
			MPI_Barrier(MPI_COMM_WORLD);
		}
	}
	if (debug && iproc == 0)
		fprintf(stderr, "\n");

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