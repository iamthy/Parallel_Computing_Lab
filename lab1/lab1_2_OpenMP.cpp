#include <omp.h>
#include<cstdio>
static long num_steps = 200000000;
double step;
#define NUM_THREADS 2
void main()
{
	double sum[NUM_THREADS];
	double tot = 0;
	omp_set_num_threads(NUM_THREADS);
	double start = omp_get_wtime();
	double step = 1.0 / (double)num_steps;
#pragma omp parallel 
	{
		double x;
		int id;
		
		id = omp_get_thread_num();
		sum[id] = 0;
		for (int i = id; i < num_steps; i = i + NUM_THREADS) {
			x = (i + 0.5) * step;
			sum[id] += 4.0 / (1.0 + x * x);
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		tot += sum[i] * step;
		//printf("%d %.10lf\n", i, sum[i]);
	}
	printf("%.10lf\n", tot);
	double end = omp_get_wtime();
	printf("%.6lf s\n", end - start);
}
