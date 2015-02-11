/*  CS 484 - Parallel Processing
	Project 3 - Reduce and Broadcast	*/
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

#define MESSAGE_SIZE	65000

int source = 0;
int destination = 0;

double When()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

// Last time we used this to compile...
// mpicc broadcast-reduce.c -lm -o broadcast
// mpirun -np 8 ./broadcast

void broadcast (int argc, char *argv[])
{
	int iproc, nproc, i;
	char host[255], message[55];
	MPI_Status status;
	MPI_Request request;

	gethostname(host, 253);
    printf("I am running on %s\n", host);
 	fflush(stdout);

	// MPI Stuff
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);	// initialize nproc (number of processors)
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc);	// initialize iproc (processor ID)

	int mask, my_id, my_virtual_id, virtual_dest, real_dest, virtual_source, real_source, dimension;
	dimension = ceil(log2(nproc));
	mask = pow(2, dimension) - 1;
	my_id = iproc;
	my_virtual_id = my_id ^ source;

	for (i = (dimension - 1); i >= 0; i--) 
	{
		sprintf(message, "%d: Hello\n", iproc);
		int twoToTheI = (int)pow(2, i);
		mask = mask ^ twoToTheI;
		// participating
		if ((my_virtual_id & mask) == 0) 
		{	
			// sending
			if ((my_virtual_id & twoToTheI) == 0) 
			{	
				virtual_dest = my_virtual_id ^ twoToTheI;
				real_dest = virtual_dest ^ source;
				printf("Iteration: %d : SEND : %d to %d\n", dimension - i, my_id, real_dest);
				MPI_Isend(message, 35, MPI_CHAR, real_dest, 0, MPI_COMM_WORLD, &request);
			}
			else 
			{
				virtual_source = my_virtual_id ^ twoToTheI;
				real_source = virtual_source ^ source;
				MPI_Recv(message, 35, MPI_CHAR, real_source, 0, MPI_COMM_WORLD, &status);
				printf("%s", message);
				// printf("Iteration: %d : RECEIVE : %d to %d\n", dimension - i, real_source, my_id);
			}
		}
	}

	MPI_Finalize();
}

void reduce (int argc, char *argv[])
{
	int iproc, nproc, i;
	char host[255];
	double start, end;

	int message[MESSAGE_SIZE];
	int k;
	for (k = 0; k < MESSAGE_SIZE; k++) 
	{
		message[k] = k;
	}

	MPI_Status status;
	MPI_Request request;

	gethostname(host, 253);
    printf("I am running on %s\n", host);
 	fflush(stdout);

	// MPI Stuff
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);	// initialize nproc (number of processors)
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc);	// initialize iproc (processor ID)
	MPI_Barrier(MPI_COMM_WORLD);
	start = When();

	int mask, my_id, msg_destination, msg_source, dimension;
	dimension = ceil(log2(nproc));
	mask = 0;
	my_id = iproc;

	for (i = 0; i < dimension; i++) 
	{
		int twoToTheI = (int)pow(2, i);

		// participating
		if ((my_id & mask) == 0) 
		{	
			// sending
			if ((my_id & twoToTheI) != 0) 
			{
				msg_destination = my_id ^ twoToTheI;
				MPI_Isend(message, MESSAGE_SIZE, MPI_INT, msg_destination, 0, MPI_COMM_WORLD, &request);
				printf("Iteration: %d : SEND : %d to %d\n", i + 1, my_id, msg_destination);
			}
			else 
			{
				int tempArray[MESSAGE_SIZE];
				msg_source = my_id ^ twoToTheI;
				MPI_Recv(tempArray, MESSAGE_SIZE, MPI_INT, msg_source, 0, MPI_COMM_WORLD, &status);
				int j;
				for (j = 0; j < MESSAGE_SIZE; j++) 
				{
					message[j] += tempArray[j];
				}
			}dfaskdj
		}
		mask = mask ^ twoToTheI;
	}
	if (iproc == destination) 
	{
		end = When();
		printf("Time: %f\n", end - start);
	}

	MPI_Finalize();
}

int main(int argc, char *argv[])
{
	printf("======= Broadcasting ======\n");
	fflush(stdout);
	broadcast();

	printf("====== Reducing ======\n");
	fflush(stdout);
	reduce();
}