/*  CS 484 - Parallel Processing
	Project 1 - Hotplate	*/

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#define PLATESIZE 		4096
#define HOT 			100
#define MILD			50
#define COLD			0

#define OLD[c, r]		old[c+PLATESIZE*r]	
#define NEW[c, r]		new[c+PLATESIZE*r]	
#define FIXED[c, r]		fixed[c+PLATESIZE*r]	


// Return the current time in seconds, using a double precision number.
double when()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

// Initializes three PLATESIZE X PLATESIZE arrays
// the third array exists to tell whether or not the temperature at a spot can change
void initialize(int* old, int* new, int* fixed)
{
	for (int row = 0; row < PLATESIZE; row++)
	{
		for (int col = 0; col < PLATESIZE; col++)
		{
			// performance enhancer - check if it's fixed first, then determine which fixed it is
			if (   (col == 0 || col + 1 == PLATESIZE || row == 0) 
				|| (row + 1 == PLATESIZE) 
				|| (row == 400 && (0 < col && col < 330)) 
				|| (row == 200 && col == 500))
			{
				// sides & top == cold & fixed
				if (col == 0 || col + 1 == PLATESIZE || row == 0)
				{
					OLD[col, row] = COLD;
					NEW[col, row] = COLD;
					FIXED[col, row] = 1;
				}
				// bottom == hot & fixed
				else if (row + 1 == PLATESIZE)
				{
					OLD[col, row] = HOT;
					NEW[col, row] = HOT;
					FIXED[col, row] = 1;
				}
				// line == hot & fixed
				else if (row == 400 && (0 < col && col < 330))
				{
					OLD[col, row] = HOT;
					NEW[col, row] = HOT;
					FIXED[col, row] = 1;
				}
				// spot = hot & fixed
				else if (row == 200 && col == 500)
				{
					OLD[col, row] = HOT;
					NEW[col, row] = HOT;
					FIXED[col, row] = 1;
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
				OLD[col, row] = MILD;
				NEW[col, row] = MILD;
				FIXED[col, row] = 0;
			}
		}
	}
}

// TODO
int distribute_temperature()
{
	int iterations = 0;

	return iterations;
}

// Distributes heat on a hotplate
int main(void)
{
	// start timer
	double start = when();

	int* old = malloc(sizeof(int) * PLATESIZE * PLATESIZE);
	int* new = malloc(sizeof(int) * PLATESIZE * PLATESIZE);
	int* fixed = malloc(sizeof(int) * PLATESIZE * PLATESIZE);
	initialize(old, new, fixed);

	// perform calculations & checks
	int i = distribute_temperature();

	// end timer & print to console
	double end = when();
	printf("Performed %3i iterations in %2.2f seconds.\n", i, end-start);

	// print hotplate to .ppm file
	// TODO imagify();

	return 0;
}