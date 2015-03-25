#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ITERATIONS		1000
#define MAXCOLOR		255

#define VALUES(x, y)	values[(x) + WIDTH*(y)]

int mandelbrot(double, double);
void print();

int* values;
int* colors;
int WIDTH = 1920;
int HEIGHT = 1080;
double START_X = -2.5;
double END_X = 1;
double LENGTH_X = 3.5;
double START_Y = -1;
double END_Y = 1;
double LENGTH_Y = 2;

int main(int argc, char *argv[]) 
{
	if (argc >= 3) 
	{
		WIDTH = atof(argv[1]);
		HEIGHT = atof(argv[2]);
	}
	if (argc >= 6)
	{
		double CENTER_X = atof(argv[3]);
		double CENTER_Y = atof(argv[4]);
		LENGTH_X = atof(argv[5]);
		LENGTH_Y = atof(argv[6]);

		START_X = CENTER_X - (LENGTH_X/2);
		END_X = CENTER_X + (LENGTH_X/2);
		START_Y = CENTER_Y - (LENGTH_Y/2);
		END_Y = CENTER_Y + (LENGTH_Y/2);
	}

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
			VALUES(x, y) = colors[mandelbrot(x, y)];

	// print();
}

int mandelbrot(double Px, double Py) 
{
	double x0 = START_X + (Px/WIDTH) * LENGTH_X;
	double y0 = START_Y + (Py/HEIGHT) * LENGTH_Y;
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