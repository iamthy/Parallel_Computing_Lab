#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>
#include<time.h>
#define maxn 10010
#define inf 1e9+7

void merge(int* a, int l, int mid, int r) {//把两个有序区间[l,mid],[mid+1,r]归并成一个有序区间
    int len = r - l + 1;//区间长度
    int* buff = (int*)malloc(len * sizeof(int));//缓冲区，用来暂时存放排序后的结果
    int i = l, j = mid + 1, k = 0;
    for (int k = 0; k < len; k++) {//归并两个有序区间
        if (i <= mid && (j > r || a[i] <= a[j]))
            buff[k] = a[i], i++;
        else
            buff[k] = a[j], j++;
    }
    for (int i = 0; i < len; i++)// 放回原数组中
        a[l + i] = buff[i];
    free(buff);
}

void merge_sort(int *a,int l,int r) {//对a数组的[l,r]区间归并排序
    if (l != r) {
        int mid = (l + r) >> 1;
        merge_sort(a,l,mid);
        merge_sort(a,mid+1,r);
        merge(a,l,mid,r);
    }
}

void PSRS(int* a, int n) {//对总个数为n的数组n进行排序
    int myid, numprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);  // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);      // get number of processes
    int len = n / numprocs;//每一个线程处理的区间长度
    int l = myid * len, r = (myid + 1) * len - 1;//处理的区间的左右端点
    //--------------------------Step 1 均匀划分并进行局部排序------------------------------
    merge_sort(a, myid * len, (myid + 1) * len - 1);

    //--------------------------Step 2 正则采样-------------------------------------------
    //每个线程均匀地从负责的区间中选择p（线程个数）样本元素交给根线程
    int* sample = (int*)malloc(numprocs * sizeof(int));//局部采样数组
    for (int i = 0; i < numprocs; i++) {//均匀地选取p个样本
        sample[i] = a[myid * len + i * len / numprocs];
    }
    MPI_Barrier(MPI_COMM_WORLD);//同步，等所有线程都完成采样再继续

    //--------------------------Step 3 采样排序 & Step 4选择主元---------------------------
    //主线程收集所有线程的采集的样本，对样本进行排序，之后等距选择p-1个主元
    int* all_samples = NULL;//主线程0收集的所有的p*p个样本
    int* pivot = NULL;//选择的主元
    if (myid == 0) {
        all_samples = (int*)malloc(numprocs * numprocs * sizeof(int));
    }
    pivot = (int*)malloc((numprocs) * sizeof(int));
    MPI_Gather(sample, numprocs, MPI_INT, all_samples, numprocs, MPI_INT, 0, MPI_COMM_WORLD);
    if (myid == 0) {
        //printf("numprocs=%d\n",numprocs);
        merge_sort(all_samples, 0, numprocs * numprocs - 1);//对样本排序
        for (int i = 0; i < numprocs -1 ; i++)
            pivot[i] = all_samples[(i + 1) * numprocs];//等距选择主元作为后续各进程划分数据的依据
        pivot[numprocs - 1] = inf;//方便后续处理边界情况
    }

    MPI_Bcast(pivot, numprocs, MPI_INT, 0, MPI_COMM_WORLD);//向所有线程广播主元

    //--------------------------Step 5 主元划分--------------------------------------------
    //每个线程根据上一阶段求得的p-1个主元，把自己负责的区间划分成大致均匀的p段
    int* cnt = (int*)malloc(numprocs * sizeof(int));//每一段的数据个数
    int* start = (int*)malloc(numprocs * sizeof(int));//每一段的起点下标
    for (int i = 0,j = l; i < numprocs; i++) {
        cnt[i] = 0;
        for (; j <= r && a[j] < pivot[i]; j++)//注意到前面第p个主元是inf，所以可以不用特判最后一段
            cnt[i]++;
    }
    start[0] = 0;
    for (int i = 1; i < numprocs; i++)
        start[i] = start[i - 1] + cnt[i - 1];
    MPI_Barrier(MPI_COMM_WORLD);

    //--------------------------Step 6 全局交换--------------------------------------------
    //把所有的线程划分好的区间按顺序放到新的数组空间中，顺序是先按线程号放好每个线程的第一段数据，再放每个线程的第二段数据，以此类推
    //第一个线程排序所有线程的第一段数据，第二个线程排序所有线程的第二段数据...

    int* new_cnt = (int*)calloc(numprocs ,sizeof(int)); // 对于第i个线程，记录的是它从各个线程接收的第i段的数据个数
    // 同时向第j个线程发送第i段的个数,具体参见MPI_Alltoall的用法
    MPI_Alltoall(cnt, 1, MPI_INT, new_cnt, 1, MPI_INT, MPI_COMM_WORLD);
    int new_len = 0; // 数据交换后该线程需要处理的数据个数
    for (int i = 0; i < numprocs; i++)
        new_len += new_cnt[i];
    int* data = (int*)malloc(new_len * sizeof(int));// 给交换后要处理的数据分配空间
    int* new_start = (int*)malloc(numprocs * sizeof(int));// 新的数据各段的偏移地址
    new_start[0] = 0;
    for (int i = 1; i < numprocs; i++)
        new_start[i] = new_start[i - 1] + new_cnt[i - 1];
    MPI_Alltoallv(&a[myid*len], cnt, start, MPI_INT, data, new_cnt, new_start, MPI_INT, MPI_COMM_WORLD);// 发送分好段的数据到对应的线程中
    MPI_Barrier(MPI_COMM_WORLD);
    

    //--------------------------Step 7 归并排序--------------------------------------------
    //排完后按顺序合并回一个数组中即最终完成所有数据的排序
    merge_sort(data, 0, new_len - 1);
    MPI_Barrier(MPI_COMM_WORLD);

    /*printf("myid=%d data : ", myid);
    for (int i = 0; i < new_len - 1; i++)
        printf("%d ", data[i]);
    puts("");
    fflush(stdout);*/

    int* root_len = NULL;
    if (!myid)
        root_len = (int *)malloc(numprocs * sizeof(int));//根进程收到的各线程处理的数据个数，用于最终合并
    MPI_Gather(&new_len, 1, MPI_INT, root_len, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int* root_start = NULL;//各个进程处理的数据区间的起始下标,用于收集数据到正确的位置
    if (!myid) {
        root_start = (int*)malloc(numprocs * sizeof(int));
        root_start[0] = 0;
        for (int i = 1; i < numprocs; i++)
            root_start[i] = root_start[i - 1] + root_len[i - 1];
    }

    MPI_Gatherv(data, new_len, MPI_INT, a, root_len, root_start, MPI_INT, 0, MPI_COMM_WORLD);//收集各线程的数据
    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char* argv[])
{
    int n = 500000, myid, numprocs;
    int *a = (int *)malloc((n+1)*sizeof(int));
    
    double start, end;
    srand(time(0));
    for (int i = 0; i < n; i++) {
        a[i] = rand()%100000;
    }
    puts("");
    MPI_Init(&argc, &argv);        // starts MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);  // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);      // get number of processes

    if (myid == 0) {
        /*printf("origin data: ");
        for (int i = 0; i < n; i++) {
            printf("%d ",a[i]);
        }
        puts("");
        fflush(stdout);*/
        start = MPI_Wtime();   
    }
    PSRS(a,n);
    if (myid == 0) {
        end = MPI_Wtime();
        printf("Runtime = %fs\n", end - start);
        /*printf("sorted data: ");
        for (int i = 0; i < n; i++) {
            printf("%d ", a[i]);
        }
        puts("");*/
    }

    MPI_Finalize();

    return 0;
}
