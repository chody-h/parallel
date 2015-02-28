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

void InitializeRows(int, double*, double*, int*);
void Calculate(double*, double*, double*, double*, int*);
bool Check(double*, double*, double*, int*);
void PrintToFile();

int nproc, iproc;
int num_rows;

int main(int argc, char *argv[])
{
    double starttime;
    int iterations = 0;

	int start_row, remain;
    double* old;
    double* new;
    int* fixed;
    double* row_up;
    double* row_down;
	bool converged = false;
	bool global_converged = false;

    // MPI_Status status = MPI_STATUS_IGNORE;
    MPI_Request request_top, request_bottom;

    MPI_Init(&argc, &argv);
    starttime = When();

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
    fprintf(stderr, "Hello from thread %d! (total %d)\n", iproc, nproc);

    /* decide how much I need to do and where */
    remain = ( (iproc == nproc-1) ? (PLATESIZE % nproc) : 0 );
    num_rows = PLATESIZE/nproc + remain;
    start_row = iproc * num_rows;
    fprintf(stderr, "(%d) I will start on row %d and calculate %d rows.\n", iproc, start_row, num_rows);

	old      = malloc(sizeof(double) * PLATESIZE * num_rows);
	new      = malloc(sizeof(double) * PLATESIZE * num_rows);
	fixed    = malloc(sizeof(int)    * PLATESIZE * num_rows);
    row_up   = malloc(sizeof(double) * PLATESIZE);
    row_down = malloc(sizeof(double) * PLATESIZE);

	InitializeRows(start_row, old, new, fixed);

	// fprintf(stderr, "(%d) Calculating\n", iproc);

	while (!global_converged) 
	{

        /* First, I must get my neighbors' boundary values */
            // int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
            
        // Exchange with above
        if (iproc != 0)
        {
			// fprintf(stderr, "(%d) Exchanging with above...\n", iproc);
            MPI_Send(&old[0], PLATESIZE, MPI_DOUBLE, iproc-1, 0, MPI_COMM_WORLD);
            MPI_Irecv(&row_up[0], PLATESIZE, MPI_DOUBLE, iproc-1, 0, MPI_COMM_WORLD, &request_top);
        }
        // Exchange with below
        if (iproc != nproc-1)
        {
			// fprintf(stderr, "(%d) Exchanging with below...\n", iproc);
            MPI_Send(&old[num_rows-1], PLATESIZE, MPI_DOUBLE, iproc+1, 0, MPI_COMM_WORLD);
            MPI_Irecv(&row_down[0], PLATESIZE, MPI_DOUBLE, iproc+1, 0, MPI_COMM_WORLD, &request_bottom);
        }

        // fprintf(stderr, "(%d) Waiting for exchange...\n", iproc);
        if (iproc != 0) MPI_Wait(&request_top, MPI_STATUS_IGNORE);
        if (iproc != nproc-1) MPI_Wait(&request_bottom, MPI_STATUS_IGNORE);
        // fprintf(stderr, "(%d) Done exchanging.\n", iproc);

        /* Then run calculations */
		Calculate(row_up, row_down, old, new, fixed);

		/* Finally, check if it has converged. If not, swap the pointers. */
		converged = Check(row_up, row_down, old, fixed);
		// int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
		MPI_Allreduce(&converged, &global_converged, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

		/* Swap values */
		double* temp = old;
		old = new;
		new = temp;
	
		iterations++;
		// if (iterations % 40 == 0) 
		// 	fprintf(stderr, ".");
		if (iterations >= 400)
		{
			converged = true;
			global_converged = true;
		}
	}

    if (iproc == 0) 
    	fprintf(stderr, "\nIt took %d iterations and %lf seconds to converge.\n", iterations, When() - starttime);

    MPI_Finalize();
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

void InitializeRows(int start_row, double* old, double* new, int* fixed) 
{
	// fprintf(stderr, "Initializing Rows...\n");
	int row, abs_row, col;

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
	// fprintf(stderr, "Done Initializing Rows.\n");
}

/* Core of hotplate algorithm. */
void Calculate(double* row_up, double* row_down, double* old, double* new, int* fixed)
{
	int row, col;

	// fprintf(stderr, "(%d) top.\n", iproc);
	// TOP ROW
	row = 0;
	if (iproc != 0)
		for (col = 0; col < PLATESIZE; col++)
			if (!FIXED(col, row)) 
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + row_up[col] + 4*OLD(col, row))/8;

	// fprintf(stderr, "(%d) normal.\n", iproc);
	// NORMAL ROWS
	for (row = 1; row < num_rows-1; row++) 
		for (col = 0; col < PLATESIZE; col++)
			if (!FIXED(col, row)) 
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + OLD(col, row-1) + 4*OLD(col, row))/8;

	// fprintf(stderr, "(%d) bottom.\n", iproc);
	// BOTTOM ROW
	row = num_rows-1;
	if (iproc != nproc-1)
		for (col = 0; col < PLATESIZE; col++)
			if (!FIXED(col, row)) 
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + row_down[col] + OLD(col, row-1) + 4*OLD(col, row))/8;
	// fprintf(stderr, "(%d) done.\n", iproc);
}

/* Examines the new data to see if any of it needs to be recalculated. If so, it stops immediately. */
bool Check(double* row_up, double* row_down, double* old, int* fixed)
{
	int row, col;

	// TOP ROW
	row = 0;
	if (iproc != 0)
		for (col = 0; col < PLATESIZE; col++)
			if (FIXED(col, row))
				continue;
			else if (fabs(OLD(col,row) - (OLD(col+1,row) + OLD(col-1,row) + OLD(col,row+1) + row_up[col])/4) > 0.1)
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
			else if (fabs(OLD(col,row) - (OLD(col+1,row) + OLD(col-1,row) + row_down[col] + OLD(col,row-1))/4) > 0.1)
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
