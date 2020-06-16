#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>
#include<time.h>
int ispri(int x) {
    if (x == 1) return 0;
    for (int i = 2; i * i <= x; i++)
        if (x % i == 0) return 0;
    return 1;
}

int main(int argc, char* argv[])
{
    int n = 100, myid, numprocs;
    int tot = 0, cnt = 0;//tot是总个数，cnt是单个线程计算的个数
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    double start, end;
    MPI_Init(&argc, &argv);        // starts MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);  // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);      // get number of processes

    MPI_Barrier(MPI_COMM_WORLD);
    if (myid == 0)
    {
        scanf_s("%d", &n);
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    start = MPI_Wtime();
    for (int i = myid * 2 + 3; i <= n; i += numprocs * 2) {
        cnt += ispri(i);
        //printf("%d\n",i);
    }


    MPI_Reduce(&cnt, &tot, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0)
        printf("总质数个数为 %d\n...", tot + 1);//2单独特判

    MPI_Barrier(MPI_COMM_WORLD); /* IMPORTANT */
    end = MPI_Wtime();

    MPI_Finalize();
    if (myid == 0) { /* use time on master node */
        printf("Runtime = %fs\n", end - start);
    }

    return 0;
}
