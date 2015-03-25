#include <stdio.h>

#define WIDTH	500
#define HEIGHT	500

int mandelbrot(double, double);

int* values;
int* colors;

int main() 
{
	values = (int*)malloc(WIDTH * HEIGHT * sizeof(int));
	for (int x = 0; x < WIDTH; x++)
	{
		for (double y = 0; y < HEIGHT; y++) 
		{
			values[x, y] = mandelbrot(x, y);
		}
	}
}

int mandelbrot(double Px, double Py) 
{
	double x0 = -2.5 + (Px/WIDTH) * 3.5;
	double y0 = -1 + (Py/HEIGHT) * 2;
	double x = 0.0;
	double y = 0.0;
	int iteration = 0;
	int max_iteration = 1000;
	while (x*x + y*y < 4 && iteration < max_iteration)
	{
		double xtemp = x*x - y*y + x0;
		y = 2*x*y + y0
		x = xtemp;
		iteration++;
	}
	return iteration;
}

int print()
{

}