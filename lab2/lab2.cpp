#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>
#include<time.h>
const int V_MAX = 100;
const int inf = 1e9 + 7;//近似无穷大
const double P = 0.2;
int *d, *v;//车辆的坐标、速度
int *last_d;//上个周期的坐标
int limit;//上一组车辆的最后一个坐标，用来模拟这一组的第一辆车
int n = 100000, myid, numprocs, cycle=2000;//n是车辆数，cycle是模拟的周期数
double start, end;

void init() {//初始化包括读入和车辆数据初始化
    //int l = myid * (n / numprocs), r = (myid + 1) * (n / numprocs);//线程处理的车辆的编号区间
    int cnt = n / numprocs;//一个线程负责的车辆数
    for (int i = 0; i < cnt; i++) {//每个线程初始化一部分车的位置和速度
        last_d[i] = d[i] = n - (cnt * myid + i + 1);
        v[i] = 0;
    }
    limit = myid == 0 ? inf : n - (cnt * myid);
    //上一组车辆的最后一辆的坐标，为了方便处理，0号车前一辆车坐标设为无穷
    /*printf("myid=%d lim=%d\n",myid,limit);
    for (int i = 0; i < cnt; i++) {
        printf("i,d[i],v[i]=%d %d %d\n", i, d[i], v[i]);
    }
    puts("");
    fflush(stdout);*/
    MPI_Barrier(MPI_COMM_WORLD);//同步,完成初始化之后再开始模拟
}

void simulate() {
    srand(time(0) + myid);//初始化随机函数，为了使随机数序列不同加上了进程号
    //int l = myid * (n / numprocs), r = (myid + 1) * (n / numprocs);//线程处理的车辆的编号区间
    int cnt = n / numprocs;//一个线程负责的车辆数
    for (int tim = 0; tim < cycle; tim++) {
        for (int i = 0; i < cnt; i++) {
            int previous = i == 0 ? limit : last_d[i - 1];//前一辆车的坐标
            if (previous - last_d[i] - 1 > v[i]) {//是零号车或者前车距离大于当前速度则加速
                if (v[i] != V_MAX) v[i]++;//并且还没有超过最大速度
            }
            else v[i] = previous - last_d[i] - 1;//否则速大小度变为与前车车距
            if (1.0 * rand() / RAND_MAX < P && v[i]>0)//随机以P的概率减速
                v[i]--;
            d[i] = last_d[i] + v[i];//确定了速度v后向前移动v的距离

        }
        for (int i = 0; i < cnt; i++) {///更新上个周期坐标last_d 准备开始下一周期
            last_d[i] = d[i];
        }
        if (myid != numprocs - 1) {//所有进程向后一个进程发送最后一辆车的位置信息
            //为了避免tag冲突，tag的值为周期数*进程数+当前进程编号
            MPI_Send(&last_d[cnt - 1], 1, MPI_INT, myid + 1, tim * numprocs + myid, MPI_COMM_WORLD);
        }
        if (myid != 0) {//所有进程接受前一个进程发送的最后一辆车的位置信息
            MPI_Recv(&limit, 1, MPI_INT, myid - 1, tim * numprocs + myid - 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        MPI_Barrier(MPI_COMM_WORLD);//要所有进程都结束本周期模拟才开始下一个周期模拟

    }
    int* out_d=NULL, * out_v=NULL;//用于主进程输出结果的缓冲区
    if (myid == 0) {
        out_d = (int*)malloc(n * sizeof(int));
        out_v = (int*)malloc(n * sizeof(int));
    }
    MPI_Gather(d, cnt, MPI_INT, out_d, cnt, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(v, cnt, MPI_INT, out_v, cnt, MPI_INT, 0, MPI_COMM_WORLD);
    if (myid == 0) {
        int* sum = (int*)calloc((V_MAX + 1), sizeof(int));
        for (int i = 0; i < n; i++)
            sum[out_v[i]]++;
        /*for (int i = 0; i <= V_MAX; i++)//输出不同速度的车辆数
            printf("v=%d cnt=%d\n",i,sum[i]);
        for (int i = 0; i < n; i++) {//输出每辆车的最终状态
            printf("i,d[i],v[i]=%d %d %d\n", i, out_d[i], out_v[i]);
        }*/
        
    }
}

int main(int argc, char* argv[])
{

    MPI_Init(&argc, &argv);        // starts MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);  // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);      // get number of processes

    if (myid == 0)
    {
        start = MPI_Wtime();
    }
    d = (int*)malloc(n * sizeof(int));
    last_d = (int*)malloc(n * sizeof(int));
    v = (int*)malloc(n * sizeof(int));
    MPI_Barrier(MPI_COMM_WORLD);

    init();
    simulate();

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    MPI_Finalize();
    if (myid == 0) {
        printf("Runtime = %fs\n", end - start);
    }
    
    return 0;
}
