#include <mpi.h>
#include <stdio.h>
#include <sys/time.h>

#define PLATESIZE		4096
#define HOT				100
#define MILD			50
#define COLD			0

#define OLD(c, r)		old  [(c) + PLATESIZE*(r)]
#define NEW(c, r)		new  [(c) + PLATESIZE*(r)]
#define FIXED(c, r)		fixed[(c) + PLATESIZE*(r)]

typedef enum {false, true} bool;

double When();
void InitializeRows();
void Calculate();
void Swap();
void Reduce();
void PrintToFile();

int main(int argc, char *argv[])
{
    // declare algorithm variables here
	int start_row, num_rows, remain;
	double* old, new;
	int* fixed;

    double starttime;

    int nproc, iproc;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    starttime = When();

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
    fprintf(stderr,"%d: Hello from %d of %d\n", iproc, iproc, nproc);

    /* decide how much i need to do and where */
    remain = ( (iproc == nproc-1) ? (size % nproc) : 0);
    num_rows = size/nproc + remain;
    start_row = iproc * num_rows;
	InitializeRows(start_row, num_rows, old, new, fixed);

	while () 
	{
		Send();
		Receive();

		Calculate();

		Swap();
	}

	Reduce();
	PrintToFile();
}

// Return the current time in seconds, using a double precision number.
double When()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

void InitializeRows(int start_row, int num_rows, double* old, double* new, int* fixed) 
{
	old   = malloc(sizeof(double) * num_rows * PLATESIZE);
	new   = malloc(sizeof(double) * num_rows * PLATESIZE);
	fixed = malloc(sizeof(int)    * num_rows * PLATESIZE);

	for (int row = 0; row < num_rows; row++) 
	{
		int abs_row = start_row + row;
		for (int col = 0; col < PLATESIZE; col++) 
		{
			// sides & top == cold & fixed
			if (col == 0 || col + 1 == PLATESIZE || abs_row == 0)
			{
				OLD(col, row) = COLD;
				NEW(col, row) = COLD;
				FIXED(col, row) = true;
			}
			// bottom == hot & fixed
			else if (abs_row + 1 == PLATESIZE)
			{
				OLD(col, row) = HOT;
				NEW(col, row) = HOT;
				FIXED(col, row) = true;
			}
			// line == hot & fixed
			else if (abs_row == 400 && (0 < col && col < 330))
			{
				OLD(col, row) = HOT;
				NEW(col, row) = HOT;
				FIXED(col, row) = true;
			}
			// spot = hot & fixed
			else if (abs_row == 200 && col == 500)
			{
				OLD(col, row) = HOT;
				NEW(col, row) = HOT;
				FIXED(col, row) = true;
			}
			// everywhere else = mild & not fixed
			else
			{
				OLD(col, row) = MILD;
				NEW(col, row) = MILD;
				FIXED(col, row) = false;
			}
		}
	}
}
