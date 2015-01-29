/*  CS 484 - Parallel Processing
	Project 1 - Hotplate	*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>			/* fabs */
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>



#define PLATESIZE 		4096
#define HOT 			100
#define MILD			50
#define COLD			0

#define OLD(c, r)		old  [(c) + PLATESIZE*(r)]	
#define NEW(c, r)		new  [(c) + PLATESIZE*(r)]	
#define FIXED(c, r)		fixed[(c) + PLATESIZE*(r)]	



typedef enum { false, true } bool;

typedef struct 
{
	pthread_mutex_t count_lock;
	pthread_cond_t ok_to_proceed;
	int count;
} mylib_barrier_t;

typedef struct
{
	int threadcount;
	int tid;
} pthread_arg;



pthread_mutex_t mut;
// pthread_barrier_t barr;
mylib_barrier_t* barriers;

double* old;
double* new;
int* fixed;
int iterations;
bool done;



// Return the current time in seconds, using a double precision number.
double when()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

// Initializes three PLATESIZE X PLATESIZE arrays
// the third array exists to tell whether or not the temperature at a spot can change
void initialize()
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
void convert_to_rgb(double temperature, int* r, int* b)
{
	// red  == 255 0 0
	// blue == 0 0 255

	*r = (int)(2.55 * temperature);
	*b = (int)(255 - (255/100) * temperature);
}

// prints a 2x2 matrix as a blue and red picture. 
// matrix must contain temperature values between 0 and 100
void imagify()
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

// // SERIAL METHOD TO CALCULATE
// void run_calculations(double* old, double* new, int* fixed)
// {
// 	for (int row = 0; row < PLATESIZE; row++)
// 	{
// 		for (int col = 0; col < PLATESIZE; col++)
// 		{
// 			if (!FIXED(col, row))
// 			{
// 				NEW(col, row) = (OLD(col+1, row) + OLD(col-1, row) + OLD(col, row+1) + OLD(col, row-1) + 4 * OLD(col, row))/8;
// 			}
// 		}
// 	}
// }

// // SERIAL METHOD TO CHECK
// Examines the new data to see if any of it needs to be recalculated. If so, it stops immediately.
// bool check_for_steady()
// {
// 	for (int row = 0; row < PLATESIZE; row++)
// 	{
// 		for (int col = 0; col < PLATESIZE; col++)
// 		{
// 			if (FIXED(col, row))
// 			{
// 				continue;
// 			}
// 			else if (fabs(NEW(col,row) - (NEW(col+1,row) + NEW(col-1,row) + NEW(col,row+1) + NEW(col,row-1))/4) > 0.1)
// 			{
// 				return false;
// 			}
// 		}
// 	}
// 	return true;
// }

void mylib_init_linear_barrier(mylib_barrier_t *b)
{
	b->count = 0;
	pthread_mutex_init(&(b->count_lock), NULL);
	pthread_cond_init(&(b->ok_to_proceed), NULL);
}

void mylib_linear_barrier(mylib_barrier_t *b, int num_threads)
{
	pthread_mutex_lock(&(b->count_lock));
	b->count++;
	if (b->count == num_threads)
	{
		b->count = 0;
		pthread_cond_broadcast(&(b->ok_to_proceed));
	}
	else
		while(pthread_cond_wait(&(b->ok_to_proceed), &(b->count_lock)) != 0);

	pthread_mutex_unlock(&(b->count_lock));
}

void mylib_log_barrier(mylib_barrier_t *b, int num_threads)
{

}

void *parallel(void *packaged_argument)
{
	pthread_arg* arg = (pthread_arg*)packaged_argument;
	int num_tasks = arg->threadcount;
	int tid = arg->tid;

	// printf("Hi from thread %d. done: %d, num_tasks: %d, iterations: %d\n", tid, (int)done, num_tasks, iterations);
	int offset = PLATESIZE * tid / num_tasks;
	int max = offset + PLATESIZE / num_tasks;

	while (!done)
	{
		for (int row = offset; row < max; row++)
		{
			for (int col = 0; col < PLATESIZE; col++)
			{
				if (!FIXED(col, row))
				{
					NEW(col, row) = (OLD(col-1, row) + OLD(col+1, row) + OLD(col, row+1) + OLD(col, row-1) + 4 * OLD(col, row))/8;
				}
			}
		}

		// pthread_barrier_wait(&barr);
		// mylib_linear_barrier(&barr, num_tasks);
		mylib_log_barrier(&barr, num_tasks);

		if (tid == 0)
		{
			iterations += 1;
			double* temp = new;
			new = old;
			old = temp;
			done = true;
		}

		// pthread_barrier_wait(&barr);
		// mylib_linear_barrier(&barr, num_tasks);
		mylib_log_barrier(&barr, num_tasks);

		bool my_done = true;
		for (int row = offset; row < max; row++)
		{
			for (int col = 0; col < PLATESIZE; col++)
			{
				if (FIXED(col, row))
				{
					continue;
				}
				else if (fabs(OLD(col,row) - (OLD(col+1,row) + OLD(col-1,row) + OLD(col,row+1) + OLD(col,row-1))/4) > 0.1)
				{
					my_done = false;
				}
			}
		}

		if (my_done == false) 
		{
			pthread_mutex_lock(&mut);
			if (done == true) done = false;
			pthread_mutex_unlock(&mut);
		}

		// pthread_barrier_wait(&barr);
		// mylib_linear_barrier(&barr, num_tasks);
		mylib_log_barrier(&barr, num_tasks);

		// to prevent endless waiting
		// if (iterations == 400) done = true;
	}
}

// Distributes heat on a hotplate
int main(void)
{
	printf("Initializing...\n");
	for (int i = 0; i < 5; i++)
	{
		int threadcount = 1 << i;


		// start timer
		double start = when();


		// pthread_barrier_init(&barr, NULL, threadcount);
		mylib_init_linear_barrier(&barr);
		pthread_mutex_init(&mut, NULL);
		old   = malloc(sizeof(double) * PLATESIZE * PLATESIZE);
		new   = malloc(sizeof(double) * PLATESIZE * PLATESIZE);
		fixed = malloc(sizeof(int)    * PLATESIZE * PLATESIZE);
		initialize(old, new, fixed);
		iterations = 0;
		done = false;


		// distribute the heat
		printf("Calculating with %d threads...\n", threadcount);
		pthread_t threads[threadcount];
		pthread_arg* all = malloc(threadcount * sizeof(pthread_arg));
		for (int i = 0; i < threadcount; i++) 
		{
			(all+i)->threadcount = threadcount;
			(all+i)->tid = i;
			pthread_create(&threads[i], NULL, parallel, (void*) (all+i));
		}

		for (int i = 0; i < threadcount; i++)
			pthread_join(threads[i], NULL);	


		// end timer & print to console
		double end = when();
		printf("Performed %3d iterations in %2.2f seconds using %d threads.\n", iterations, end-start, threadcount);

		// pthread_barrier_destroy(&barr);
		free(old);
		free(new);
		free(fixed);
	}

	return 0;
}
