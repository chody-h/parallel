#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIDTH			1920
#define HEIGHT			1080
#define ITERATIONS		1000
#define MAXCOLOR		255

#define VALUES(x, y)	values[(x) + WIDTH*(y)]

int mandelbrot(double, double);
void print();

int* values;
int* colors;

int main() 
{
	values = (int*)malloc(WIDTH * HEIGHT * sizeof(int));
	colors = (int*)malloc(ITERATIONS * sizeof(int));

	int i;
	colors[0] = 255;
	for (i = 1; i < ITERATIONS; i++) 
	{
		colors[i] = 255 - 255*log(i)/log(ITERATIONS);
		// fprintf(stderr, "%d ", colors[i]);
	}

	fprintf(stderr, "Beginning mandelbrot calculations.\n");

	int x, y;
	for (y = 0; y < HEIGHT; y++)
		for (x = 0; x < WIDTH; x++) 
			VALUES(x, y) = mandelbrot(x, y);

	fprintf(stderr, "iteration count: %d vs %d\n", VALUES(1079, 500), VALUES(1080, 500));
	fprintf(stderr, "comparison     : %d vs %d\n", mandelbrot(1079, 500), mandelbrot(1079, 500));

	print();
}

int mandelbrot(double Px, double Py) 
{
	double x0 = -2.5 + (Px/WIDTH) * 3.5;
	double y0 = -1 + (Py/HEIGHT) * 2;
	double x = 0.0;
	double y = 0.0;
	int iteration = 0;
	while (x*x + y*y < 4 && iteration < ITERATIONS)
	{
		double xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;
		x = xtemp;
		iteration++;
	}
	return iteration;
}

void print()
{
	fprintf(stderr, "Now printing to image file.\n");
	FILE *f = fopen("mandelbrot.ppm", "w");
	if (f != NULL)
	{
		fprintf(f, "P3\n");
		fprintf(f, "#mandelbrot.ppm\n");
		fprintf(f, "%d %d\n", WIDTH, HEIGHT);
		fprintf(f, "255\n\n");

		int x, y;
		for (y = HEIGHT-1; y >= 0; y--)
			for (x = 0; x < WIDTH; x++) 
				fprintf(f, "%d %d %d\n", VALUES(x, y), VALUES(x, y), VALUES(x, y));

		fclose(f);

		printf("Done printing.\n");
	}
	else
		printf("Error opening file.\n");
}