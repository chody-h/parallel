/*  CS 484 - Parallel Processing
	Project 1 - Hotplate	*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>			/* fabs */
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#define PLATESIZE 		4096
#define HOT 			100
#define MILD			50
#define COLD			0

#define OLD(c, r)		old  [(c) + PLATESIZE*(r)]	
#define NEW(c, r)		new  [(c) + PLATESIZE*(r)]	
#define FIXED(c, r)		fixed[(c) + PLATESIZE*(r)]	

typedef enum { false, true } bool;


// Return the current time in seconds, using a double precision number.
double when()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

// Initializes three PLATESIZE X PLATESIZE arrays
// the third array exists to tell whether or not the temperature at a spot can change
void initialize(double* old, double* new, int* fixed)
{
	for (int row = 0; row < PLATESIZE; row++)
	{
		for (int col = 0; col < PLATESIZE; col++)
		{
			// performance enhancer? - check if it's fixed in one go
			// could definitely be optimized a lot more
			if (   (col == 0 || col + 1 == PLATESIZE || row == 0) 
				|| (row + 1 == PLATESIZE) 
				|| (row == 400 && (0 < col && col < 330)) 
				|| (row == 200 && col == 500))
			{
				// sides & top == cold & fixed
				if (col == 0 || col + 1 == PLATESIZE || row == 0)
				{
					OLD(col, row) = COLD;
					NEW(col, row) = COLD;
					FIXED(col, row) = true;
				}
				// bottom == hot & fixed
				else if (row + 1 == PLATESIZE)
				{
					OLD(col, row) = HOT;
					NEW(col, row) = HOT;
					FIXED(col, row) = true;
				}
				// line == hot & fixed
				else if (row == 400 && (0 < col && col < 330))
				{
					OLD(col, row) = HOT;
					NEW(col, row) = HOT;
					FIXED(col, row) = true;
				}
				// spot = hot & fixed
				else if (row == 200 && col == 500)
				{
					OLD(col, row) = HOT;
					NEW(col, row) = HOT;
					FIXED(col, row) = true;
				}
				else
				{
					printf("Impossible.");
					break;
				}
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

// converts a temperature between 0 and 100 to corresponding rgb values
void convert_to_rgb(double temp, int* r, int* b)
{
	// red  == 255 0 0
	// blue == 0 0 255

	*r = (int)(2.55 * temp);
	*b = (int)(255 - (255/100) * temp);
}

// prints a 2x2 matrix as a blue and red picture. 
// matrix must contain temperature values between 0 and 100
void imagify(double* new)
{
	printf("Now printing to image file...\n");
	FILE *f = fopen("plate.ppm", "w");
	if (f != NULL)
	{
		fprintf(f, "P3\n");
		fprintf(f, "#plate.ppm\n");
		fprintf(f, "%d %d\n", PLATESIZE, PLATESIZE);
		fprintf(f, "255\n\n");

		for (int row = 0; row < PLATESIZE; row++)
		{
			for (int col = 0; col < PLATESIZE; col++)
			{
				double temp = NEW(col, row);
				int r = 0, b = 0;
				convert_to_rgb(temp, &r, &b);
				fprintf(f, "%d %d %d\n", r, 0, b);
			}
		}
		fclose(f);

		printf("Done printing to file.\n");
	}
	else
	{
		printf("Error opening file.\n");
	}
}

// SERIAL METHOD TO CALCULATE
void run_calculations(double* old, double* new, int* fixed)
{
	for (int row = 0; row < PLATESIZE; row++)
	{
		for (int col = 0; col < PLATESIZE; col++)
		{
			if (!FIXED(col, row))
			{
				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + OLD(col, row-1) + 4 * OLD(col, row))/8;
			}
		}
	}
}

// Examines the new data to see if any of it needs to be recalculated. If so, it stops immediately.
bool check_for_steady(double* new, int* fixed)
{
	for (int row = 0; row < PLATESIZE; row++)
	{
		for (int col = 0; col < PLATESIZE; col++)
		{
			if (FIXED(col, row))
			{
				continue;
			}
			else if (fabs(NEW(col,row) - (NEW(col+1,row) + NEW(col-1,row) + NEW(col,row+1) + NEW(col,row-1))/4) > 0.1)
			{
				return false;
			}
		}
	}
	return true;
}

// calculates new temperatures from the old data set in parallel
int distribute_temperature(double* old, double* new, int* fixed)
{
	int iterations = 0;
	bool done = false;

	while (!done)
	{
		#pragma omp parallel for shared(old, new, fixed)
		for (int row = 0; row < PLATESIZE; row++)
		{
			for (int col = 0; col < PLATESIZE; col++)
			{
				if (!FIXED(col, row))
				{
					NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + OLD(col, row-1) + 4 * OLD(col, row))/8;
				}
			}
		}

		iterations += 1;
		double* temp = new;
		new = old;
		old = temp;

		done = check_for_steady(old, fixed);
	}

	return iterations;
}

// Distributes heat on a hotplate
int main(void)
{
	printf("Initializing...\n");
	for (int i = 0; i < 5; i++)
	{
		int threadcount = 1 << i;
		omp_set_num_threads(threadcount);

		// start timer
		double start = when();

		double* old = malloc(sizeof(double) * PLATESIZE * PLATESIZE);
		double* new = malloc(sizeof(double) * PLATESIZE * PLATESIZE);
		int*  fixed = malloc(sizeof(int)    * PLATESIZE * PLATESIZE);
		initialize(old, new, fixed);

		// distribute the heat
		printf("Calculating with %d threads...\n", threadcount);
		int i = distribute_temperature(old, new, fixed);

		// end timer & print to console
		double end = when();
		printf("Performed %3d iterations in %2.2f seconds using %d threads.\n", i, end-start, threadcount);

		free(old);
		free(new);
		free(fixed);
	}

	// print hotplate to .ppm file
	// imagify(new);

	return 0;
}
