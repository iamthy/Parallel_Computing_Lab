#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#define maxn 100010
typedef long double ld;
const ld G = 6.67e-11;
const ld Mass = 10000;
int n = 256, myid, numprocs, tim=5000, wid;

void compute_force(int id,ld x[],ld y[],ld fx[],ld fy[]) {//计算id号小球受力
    ld sum_fx = 0, sum_fy = 0;//合力的两个分量
    for (int i = 0; i < n; i++) {
        if (i == id) continue;
        ld r2 = (x[id] - x[i]) * (x[id] - x[i]) + (y[id] - y[i]) * (y[id] - y[i]);//距离平方
        ld r = sqrtl(r2);
        ld f = G * Mass * Mass / r2;//受力
        ld dir_x = (x[i] - x[id]) / r;//受力在x的分量
        ld dir_y = (y[i] - y[id]) / r;//受力在y的分量
        sum_fx += f * dir_x;//计算合力的分量
        sum_fy += f * dir_y;
    }
    fx[id] = sum_fx, fy[id] = sum_fy;
}

void compute_velocities(int id,ld vx[], ld vy[], ld fx[], ld fy[]) {//计算速度
    vx[id] += fx[id] / Mass;
    vy[id] += fy[id] / Mass;
}

void compute_positions(int id, ld x[], ld y[], ld vx[], ld vy[]) {//计算坐标
    x[id] += vx[id];
    y[id] += vy[id];
}

int main(int argc, char* argv[])
{
    
    double start, end;
    //scanf_s("%d%d",&n,&tim);
    wid = sqrt(n);//初始正方形区域宽度
    ld * x = (ld*)malloc(n * sizeof(ld));
    ld * y = (ld*)malloc(n * sizeof(ld));
    ld * fx = (ld*)malloc(n * sizeof(ld));
    ld * fy = (ld*)malloc(n * sizeof(ld));
    ld * vx = (ld*)malloc(n * sizeof(ld));
    ld * vy = (ld*)malloc(n * sizeof(ld));
    for (int i = 0; i < n; i++) {
        x[i] = i / wid/100.0, y[i] = i % wid/100.0;
        vx[i] = 0, vy[i] = 0;
    }
    MPI_Init(&argc, &argv);        // starts MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);  // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);      // get number of processes
    if (myid == 0) {
        start = MPI_Wtime();
    }

    for (int t = 0; t < tim; t++) {
        int l = myid * n / numprocs, r = (myid + 1) * n / numprocs;
        for (int i = l; i < r; i++) {
            compute_force(i, x, y, fx, fy);
            compute_velocities(i, vx, vy, fx, fy);
            compute_positions(i, x, y, vx, vy);
        }
        if (numprocs > 1) {
            for (int j = 0; j < numprocs; j++) {//广播这一周期的计算结果
                MPI_Bcast(&x[n / numprocs * j], n / numprocs, MPI_LONG_DOUBLE, j, MPI_COMM_WORLD);
                MPI_Bcast(&y[n / numprocs * j], n / numprocs, MPI_LONG_DOUBLE, j, MPI_COMM_WORLD);
                MPI_Bcast(&vx[n / numprocs * j], n / numprocs, MPI_LONG_DOUBLE, j, MPI_COMM_WORLD);
                MPI_Bcast(&vy[n / numprocs * j], n / numprocs, MPI_LONG_DOUBLE, j, MPI_COMM_WORLD);
                MPI_Bcast(&fx[n / numprocs * j], n / numprocs, MPI_LONG_DOUBLE, j, MPI_COMM_WORLD);
                MPI_Bcast(&fy[n / numprocs * j], n / numprocs, MPI_LONG_DOUBLE, j, MPI_COMM_WORLD);
            }
        }
    }


    MPI_Barrier(MPI_COMM_WORLD);
    
    if (myid == 0) {
        printf("x       y      vx      vy      fx      fy\n");
        for (int i = 0; i < n; i++) {
            //输出坐标单位是厘米
            printf("%.3lf %.3lf %.3lf %.3lf %.3lf %.3lf\n", x[i]*100, y[i]*100, vx[i]*100, vy[i]*100, fx[i], fy[i]);
        }
        end = MPI_Wtime();
        printf("Runtime = %fs\n", end - start);
    }
    MPI_Finalize();
    

    return 0;
}