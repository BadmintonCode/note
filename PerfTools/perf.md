valgrind：用虚拟机的方式   
http://stackoverflow.com/questions/1656227/how-does-valgrind-work    
perftools：插入代码   



link

```
g++  Solution.cpp  -std=c++11 -lprofiler -ltcmalloc

```

### HEAPCHECK

env HEAPCHECK=normal ./a.out    
pprof ./a.out "/tmp/a.out.8687._main_-end.heap"  --heapcheck --line  --text
```
Using local file ./a.out.   
Using local file /tmp/a.out.8687._main_-end.heap.
Total: 0.0 MB
     0.0  80.0%  80.0%      0.0  80.0% main /home/work/test_gperftools/test_mem.cpp:18
     0.0  13.3%  93.3%      0.0  13.3% main /home/work/test_gperftools/test_mem.cpp:14 (discriminator 2)
     0.0   6.7% 100.0%      0.0   6.7% main /home/work/test_gperftools/test_mem.cpp:16
     0.0   0.0% 100.0%      0.0 100.0% __libc_start_main /build/eglibc-IRFzFh/eglibc-2.19/csu/libc-start.c:287
     0.0   0.0% 100.0%      0.0 100.0% _start ??:?
```

### HEAPPROFILE

env HEAPPROFILE=/tmp/a.hprof ./a.out    
pprof ./a.out "/tmp/a.hprof.0001.heap"  --heapcheck --line  --text  
```                      
Using local file ./a.out.
Using local file /tmp/a.hprof.0001.heap.
Total: 0.0 MB
     0.0  75.0%  75.0%      0.0  75.0% main /home/work/test_gperftools/test_mem.cpp:18
     0.0  12.5%  87.5%      0.0  12.5% main /home/work/test_gperftools/test_mem.cpp:14 (discriminator 2)
     0.0   6.2%  93.8%      0.0   6.2% __static_initialization_and_destruction_0 /home/work/test_gperftools/test_mem.cpp:10
     0.0   6.2% 100.0%      0.0   6.2% main /home/work/test_gperftools/test_mem.cpp:16
     0.0   0.0% 100.0%      0.0   6.2% _GLOBAL__sub_I_gP /home/work/test_gperftools/test_mem.cpp:20
     0.0   0.0% 100.0%      0.0   6.2% __libc_csu_init ??:?
     0.0   0.0% 100.0%      0.0   6.2% __libc_start_main /build/eglibc-IRFzFh/eglibc-2.19/csu/libc-start.c:246
     0.0   0.0% 100.0%      0.0  93.8% __libc_start_main /build/eglibc-IRFzFh/eglibc-2.19/csu/libc-start.c:287
     0.0   0.0% 100.0%      0.0 100.0% _start ??:?
```


### CPUPROFILE

env CPUPROFILE=/tmp/cpu.prof  ./cpuprof_test 
pprof --text ./cpuprof_test  /tmp/cpu.prof 

多线程 在线程开始执行处调用 ProfilerRegisterThread(); 
如果不使用 CPUPROFILE 可以在代码中显示调用：
ProfilerStart("test.prof");
ProfilerStop();

strace 查看系统调用

##perf

1）Hardware Event 是由 PMU 硬件产生的事件，比如 cache 命中，当您需要了解程序对硬件特性的使用情况时，便需要对这些事件进行采样；

2）Software Event 是内核软件产生的事件，比如进程切换，tick 数等 ;

3）Tracepoint event 是内核中的静态 tracepoint 所触发的事件，这些 tracepoint 用来判断程序运行期间内核的行为细节，比如 slab 分配器的分配次数等。

Task-clock-msecs：CPU 利用率，该值高，说明程序的多数时间花费在 CPU 计算上而非 IO。

Context-switches：进程切换次数，记录了程序运行过程中发生了多少次进程切换，频繁的进程切换是应该避免的。

Cache-misses：程序运行过程中总体的 cache 利用情况，如果该值过高，说明程序的 cache 利用不好

CPU-migrations：表示进程 t1 运行过程中发生了多少次 CPU 迁移，即被调度器从一个 CPU 转移到另外一个 CPU 上运行。

Cycles：处理器时钟，一条机器指令可能需要多个 cycles，

Instructions: 机器指令数目。

IPC：是 Instructions/Cycles 的比值，该值越大越好，说明程序充分利用了处理器的特性。

Cache-references: cache 命中的次数



perf stat [excute file]
perf record –e cpu-clock ./test1
perf report