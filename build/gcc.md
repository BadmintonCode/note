## gcc


#### gcc -c
____
gcc -c -o xx.o xxx.c
-c: 不连接


#### gcc、g++
____
g++ StaticClass.o  UseA.o  -o debug.o #自动使用c++库   
gcc StaticClass.o  UseA.o  -o debug.o -lstdc++


#### gcc库搜索路径
____
gcc –L /usr/dev/mysql/lib –static –lmysqlclient test.o –o test   
##### 静态库链接时搜索路径顺序：

1. ld会去找GCC命令中的参数-L
2. 再找gcc的环境变量LIBRARY_PATH
3. 再找内定目录 /lib /usr/lib /usr/local/lib 这是当初compile gcc时写在程序内的

##### 动态链接时、执行时搜索路径顺序:

1. 编译目标代码时指定的动态库搜索路径
2. 环境变量LD_LIBRARY_PATH指定的动态库搜索路径
3. 配置文件/etc/ld.so.conf中指定的动态库搜索路径
4. 默认的动态库搜索路径/lib
5. 默认的动态库搜索路径/usr/lib

有关环境变量：   
LIBRARY_PATH环境变量：指定程序静态链接库文件搜索路径   
LD_LIBRARY_PATH环境变量：指定程序动态链接库文件搜索路径


#### pthread、lpthread
_____
-pthread 等价于 -D_REENTRANT -lpthread

_REENTRANT 是一个宏为部分函数定义了可重入的版本
```
`-pthread'
     Adds support for multithreading with the "pthreads" library.  This
     option sets flags for both the preprocessor and linker.
```
ubuntu 下面的处理
```c
#  if !defined _LIBC || defined _LIBC_REENTRANT
/* When using threads, errno is a per-thread value.  */
#   define errno (*__errno_location ())
#  endif
# endif /* !__ASSEMBLER__ */
#endif /* _ERRNO_H */

```

