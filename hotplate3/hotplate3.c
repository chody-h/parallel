#include <mpi.h>
#include <stdlib.h>
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
double fabs(double d);

void InitializeRows();
void Calculate();
bool Check();
void PrintToFile();

int nproc, iproc;
bool converged, global_converged;
int num_rows;

int main(int argc, char *argv[])
{
    double starttime;
    int iterations = 0;

    // MPI_Status status;
    MPI_Request request;

    MPI_Init(&argc, &argv);
    starttime = When();

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
    fprintf(stderr, "%d: Hello from %d of %d\n", iproc, iproc, nproc);

    double* old, new;
    int* fixed;

	InitializeRows(old, new, fixed);

    double* row_up = malloc(sizeof(double) * PLATESIZE);
    double* row_down = malloc(sizeof(double) * PLATESIZE);

	while (!converged) 
	{
        /* First, I must get my neighbors' boundary values */
        	// int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
            // int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
            
        // Exchange with above
        if (iproc != 0)
        {
            MPI_Isend(&old[0], PLATESIZE, MPI_DOUBLE, iproc-1, 0, MPI_COMM_WORLD, &request);
            MPI_Irecv(&row_up, PLATESIZE, MPI_DOUBLE, iproc-1, 0, MPI_COMM_WORLD, &request);
        }
        // Exchange with below
        else if (iproc != nproc-1)
        {
            MPI_Isend(&old[num_rows-1], PLATESIZE, MPI_DOUBLE, iproc+1, 0, MPI_COMM_WORLD, &request);
            MPI_Irecv(&row_down, PLATESIZE, MPI_DOUBLE, iproc+1, 0, MPI_COMM_WORLD, &request);
        }

        /* Then run calculations */
		Calculate(row_up, row_down);

		/* Finally, check if it has converged. If not, swap the pointers. */
		converged = Check(num_rows, row_up, row_down, old, fixed);
		// int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
		MPI_Allreduce(&converged, &converged, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
	
		iterations++;
	}

    if (iproc == 0) 
    	fprintf(stderr, "It took %d iterations and %lf seconds to converge.\n", iterations, When() - starttime);

	// TODO: reduce all rows to a single node for printing
	// if (iproc == 0) PrintToFile();
}

/* Return the current time in seconds, using a double precision number. */
double When()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

double fabs(double d)
{
	return (d > 0.0) ? d : -d ;
}

void InitializeRows(double* old, double* new, int* fixed) 
{
	int start_row, remain;
	int row, abs_row, col;

    /* decide how much I need to do and where */
    remain = ( (iproc == nproc-1) ? (PLATESIZE % nproc) : 0 );
    num_rows = PLATESIZE/nproc + remain;
    start_row = iproc * num_rows;

	old   = malloc(sizeof(double) * num_rows * PLATESIZE);
	new   = malloc(sizeof(double) * num_rows * PLATESIZE);
	fixed = malloc(sizeof(int)    * num_rows * PLATESIZE);

	for (row = 0; row < num_rows; row++) 
	{
		abs_row = start_row + row;
		for (col = 0; col < PLATESIZE; col++) 
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

/* Core of hotplate algorithm. */
void Calculate()
{
	int row, col;

	// TOP ROW
	row = 0;
	if (iproc != 0)
		for (col = 0; col < PLATESIZE; col++)
			if (!FIXED(col, row))
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + row_up[col] + 4 * OLD(col, row))/8;

	// NORMAL ROWS
	for (row = 1; row < num_rows-1; row++)
		for (col = 0; col < PLATESIZE; col++)
			if (!FIXED(col, row))
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + OLD(col, row-1) + 4 * OLD(col, row))/8;

	// BOTTOM ROW
	row = num_rows-1;
	if (iproc != nproc-1)
		for (col = 0; col < PLATESIZE; col++)
			if (!FIXED(col, row))
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + row_down[col] + OLD(col, row-1) + 4 * OLD(col, row))/8;
}

/* Examines the new data to see if any of it needs to be recalculated. If so, it stops immediately. */
bool Check()
{
	int row, col;

	// TOP ROW
	row = 0;
	if (iproc != 0)
		for (col = 0; col < PLATESIZE; col++)
			if (FIXED(col, row))
				continue;
			else if (fabs(OLD(col,row) - (OLD(col+1,row) + OLD(col-1,row) + OLD(col,row+1) + OLD(col,row-1))/4) > 0.1)
				return false;

	// NORMAL ROWS
	for (row = 1; row < num_rows-1; row++)
		for (col = 0; col < PLATESIZE; col++)
			if (FIXED(col, row))
				continue;
			else if (fabs(OLD(col,row) - (OLD(col+1,row) + OLD(col-1,row) + OLD(col,row+1) + OLD(col,row-1))/4) > 0.1)
				return false;

	// BOTTOM ROW
	row = num_rows-1;
	if (iproc != nproc-1)
		for (col = 0; col < PLATESIZE; col++)
			if (FIXED(col, row))
				continue;
			else if (fabs(OLD(col,row) - (OLD(col+1,row) + OLD(col-1,row) + OLD(col,row+1) + OLD(col,row-1))/4) > 0.1)
				return false;

	return true;
}

void PrintToFile()
{
	// File *f;
	// double temp;
	// int r, b;

	// printf("Now printing to image file...\n");
	// f = fopen("plate.ppm", "w");
	// if (f != NULL)
	// {
	// 	fprintf(f, "P3\n");
	// 	fprintf(f, "#plate.ppm\n");
	// 	fprintf(f, "%d %d\n", PLATESIZE, PLATESIZE);
	// 	fprintf(f, "255\n\n");

	// 	for (int row = 0; row < PLATESIZE; row++)
	// 	{
	// 		for (int col = 0; col < PLATESIZE; col++)
	// 		{
	// 			temp = NEW(col, row);
	// 			r = (int)(2.55 * temp);
	// 			b = (int)(255 - (255/100) * temp);
	// 			fprintf(f, "%d %d %d\n", r, 0, b);
	// 		}
	// 	}
	// 	fclose(f);

	// 	printf("Done printing to file.\n");
	// }
	// else
	// 	printf("Error opening file.\n");
}
