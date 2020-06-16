# 实验四

## 实验题目：利用MPI实现并行排序算法

>实验环境：
>
>操作系统：Windows 10家庭中文版
>
>编译器：gcc 8.1.0
>
>IDE：Visual Studio Community 2019 16.4.5
>
>CPU：Intel i5-7300HQ 2.50GHz (4核)
>
>内存：8.00GB

### 算法设计与分析

PSRS算法是一种并行排序算法，它的核心思想是把数据均匀分为p份，每个处理器各排序一段数据。然后从每个处理器处理的数据中进行均匀采样，再从采样的元素中均匀地选择p-1个元素作为划分主元。各处理器再根据主元把自己排序的数据分为p段，之后各处理器交换数据，第i个处理器通过归并排序的方式把所有处理器的第i段有序数据合并成一个有序数据段，最后发送回主进程即完成排序。

### 核心代码

核心代码实现共分为以下几个阶段：

#### Step 1 均匀划分并进行局部排序

这一步骤直接调用归并排序或者其他快速排序即可。

```c++
merge_sort(a, myid * len, (myid + 1) * len - 1);
```

#### Step 2 正则采样

```c++
//每个线程均匀地从负责的区间中选择p（线程个数）样本元素交给根线程
int* sample = (int*)malloc(numprocs * sizeof(int));//局部采样数组
for (int i = 0; i < numprocs; i++) {//均匀地选取p个样本
    sample[i] = a[myid * len + i * len / numprocs];
}
MPI_Barrier(MPI_COMM_WORLD);//同步，等所有线程都完成采样再继续
```

#### Step 3 采样排序 & Step 4选择主元

```c++
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
```

#### Step 5 主元划分

```c++
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
```

#### Step 6 全局交换

```c++
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
```
#### Step 7 归并排序

```c++
merge_sort(data, 0, new_len - 1);
MPI_Barrier(MPI_COMM_WORLD);
```

### 实验结果

n=64时运行结果如下：

<img src="E:\STUDY\Parallel Computing\lab4\1.jpg" style="zoom:67%;" />

可以看到原先随机的数据在排序过后变为有序序列了。

​																							运行时间(单位：秒)

| 规模/进程数   | 1        | 2        | 4        |
| ------------- | -------- | -------- | -------- |
| $10^5$        | 0.084059 | 0.043658 | 0.024648 |
| $5\times10^5$ | 0.453207 | 0.225673 | 0.126189 |
| $10^6$        | 0.898544 | 0.469125 | 0.239484 |
| $5\times10^6$ | 5.069723 | 2.617183 | 1.233257 |

​																								加速比

| 规模/进程数   | 1    | 2     | 4     |
| ------------- | ---- | ----- | ----- |
| $10^5$        | 1    | 1.925 | 3.410 |
| $5\times10^5$ | 1    | 2.008 | 3.591 |
| $10^6$        | 1    | 1.915 | 3.752 |
| $5\times10^6$ | 1    | 1.937 | 4.111 |

### 分析与总结

从上面的数据可以看出，PSRS算法在随机数据规模较大时能达到近似等于进程数的加速比，是一个比较有效的并行算法。在此次实验中，我使用MPI实现了PSRS算法，进一步熟悉了MPI的各种通信方式，为进一步学习编写并行程序打下了基础。