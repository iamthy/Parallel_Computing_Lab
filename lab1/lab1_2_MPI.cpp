#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>
#include<time.h>

int main(int argc, char* argv[])
{
    int n=1000, myid, numprocs;
    double h;//数值积分矩形的宽度
    double sum;//一个线程的积分和
    double ans_pi=0;//最终计算得到的pi的值
    double x;
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
    h = 1.0 / n;
    sum = 0;
    
    for (int i = myid + 1; i <= n; i += numprocs) {
        x = h * (i - 0.5);
        sum += 4.0 / (1 + x * x);
    }
    sum *= h;

    MPI_Reduce(&sum,&ans_pi,1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD); 
    end = MPI_Wtime();

    MPI_Finalize();
    if (myid == 0) { /* use time on master node */
        printf("pi = %.7lf\n",ans_pi);
        printf("Runtime = %fs\n", end - start);
    }
    
    return 0;
}
