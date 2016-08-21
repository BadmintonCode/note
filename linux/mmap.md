函数原型：
```c
void* mmap ( void * start , size_t len , int prot , int flags , int fd , off_t offset )
```
参数说明：   
```
start：映射区的开始地址，设置为0时表示由系统决定映射区的起始地址。
length：映射区的长度。//长度单位是 以字节为单位，不足一内存页按一内存页处理   

prot：  
期望的内存保护标志，不能与文件的打开模式冲突。是以下的某个值，可以通过or运算合理地组合在一起   
PROT_EXEC //页内容可以被执行   
PROT_READ //页内容可以被读取   
PROT_WRITE //页可以被写入   
PROT_NONE //页不可访问  

flags：
指定映射对象的类型，映射选项和映射页是否可以共享。它的值可以是一个或者多个以下位的组合体  
MAP_FIXED //使用指定的映射起始地址，如果由start和len参数指定的内存区重叠于现存的映射空间，重叠部分将会被丢弃。如果指定的起始地址不可用，操作将会失败。并且起始地址必须落在页的边界上。  
MAP_SHARED //与其它所有映射这个对象的进程共享映射空间。对共享区的写入，相当于输出到文件。直到msync()或者munmap()被调用，文件实际上不会被更新。  
MAP_PRIVATE //建立一个写入时拷贝的私有映射。内存区域的写入不会影响到原文件。这个标志和以上标志是互斥的，只能使用其中一个。  
MAP_DENYWRITE //这个标志被忽略。  
MAP_EXECUTABLE //同上  
MAP_NORESERVE //不要为这个映射保留交换空间。当交换空间被保留，对映射区修改的可能会得到保证。当交换空间不被保留，同时内存不足，对映射区的修改会引起段违例信号。  
MAP_LOCKED //锁定映射区的页面，从而防止页面被交换出内存。  
MAP_GROWSDOWN //用于堆栈，告诉内核VM系统，映射区可以向下扩展。  
MAP_ANONYMOUS //匿名映射，映射区不与任何文件关联。  
MAP_ANON //MAP_ANONYMOUS的别称，不再被使用。  
MAP_FILE //兼容标志，被忽略。  
MAP_32BIT //将映射区放在进程地址空间的低2GB，MAP_FIXED指定时会被忽略。当前这个标志只在x86-64平台上得到支持。  
MAP_POPULATE //为文件映射通过预读的方式准备好页表。随后对映射区的访问不会被页违例阻塞。  
MAP_NONBLOCK //仅和MAP_POPULATE一起使用时才有意义。不执行预读，只为已存在于内存中的页面建立页表入口。

fd：
有效的文件描述词。一般是由open()函数返回，其值也可以设置为-1，此时需要指定flags参数中的MAP_ANON,表明进行的是匿名映射。  

offset：
被映射对象内容的起点。文件映射的偏移量，通常设置为0，代表从文件最前方开始对应，offset必须是分页大小的整数倍

返回值：
成功执行时，mmap()返回被映射区的指针，失败时，mmap()返回MAP_FAILED[其值为(void *)-1]，_
```


* 文件映射   
 可以进程之间共享。
* 匿名内存映射   
 进程共享，通过mmap+fork 子进程继承父进程地址空间。

```
00400000-00401000 r-xp 00000000 fc:11 1318400                            /home/webedit/<user>/mmap.out
00600000-00601000 rw-p 00000000 fc:11 1318400                            /home/webedit/<user>/mmap.out
3fa5e00000-3fa5e20000 r-xp 00000000 fc:01 57373                          /lib64/ld-2.12.so
3fa601f000-3fa6020000 r--p 0001f000 fc:01 57373                          /lib64/ld-2.12.so
3fa6020000-3fa6021000 rw-p 00020000 fc:01 57373                          /lib64/ld-2.12.so
3fa6021000-3fa6022000 rw-p 00000000 00:00 0 
3fa6600000-3fa678b000 r-xp 00000000 fc:01 57375                          /lib64/libc-2.12.so
3fa678b000-3fa698a000 ---p 0018b000 fc:01 57375                          /lib64/libc-2.12.so
3fa698a000-3fa698e000 r--p 0018a000 fc:01 57375                          /lib64/libc-2.12.so
3fa698e000-3fa698f000 rw-p 0018e000 fc:01 57375                          /lib64/libc-2.12.so
3fa698f000-3fa6994000 rw-p 00000000 00:00 0 
7ffbd084c000-7ffbd084f000 rw-p 00000000 00:00 0 
7ffbd085a000-7ffbd085b000 rw-s 00000000 00:04 250996                     /dev/zero (deleted) //匿名映射
7ffbd085b000-7ffbd085c000 rw-s 00000000 fc:11 1318401                    /home/webedit/<user>/test.txt //文件映射
7ffbd085c000-7ffbd085e000 rw-p 00000000 00:00 0 
7fffb579d000-7fffb57be000 rw-p 00000000 00:00 0                          [stack]
7fffb57d7000-7fffb57d8000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]
```


