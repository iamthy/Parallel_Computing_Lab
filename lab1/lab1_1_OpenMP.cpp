#include <omp.h>
#include<cstdio>
static long num_steps = 100000;
double step;
#define NUM_THREADS 4 
void main()
{
	int sum[NUM_THREADS], tot = 0;
	omp_set_num_threads(NUM_THREADS);
	double start = omp_get_wtime();
#pragma omp parallel 
	{
		double x;
		int id;
		id = omp_get_thread_num();
		sum[id] = 0;
		for (int i = id *2+3; i < num_steps; i = i + NUM_THREADS*2) {
			bool flag = 1;
			for (int j = 2; j < i; j++) {
				if (i % j == 0) {
					flag = 0;
					break;
				}
			}
			if (flag) sum[id]++;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		tot += sum[i];
		//printf("%d %d\n", i, sum[i]);
	}
	printf("%d\n", tot+1);
	double end = omp_get_wtime();
	printf("%.5lf s\n", end - start);
}
