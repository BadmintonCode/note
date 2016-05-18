## gcc、gdb

###安装

可以安装指定版本，再通过软连接指向。
```
sudo apt-get install gcc-4.4 
sudo apt-get install g++-4.4 
```

### 生成core文件


ulimit -c unlimited  或者 ulimit -c 字节数   

修改core路径   
```
vi /etc/sysctl.conf   
kernel.core_pattern = /var/core/core_%e_%p  或者 ./core_%e_%p (当前目录)   
kernel.core_uses_pid = 0   
sysctl -p 使设置生效   

gdb <程序可执行文件> <coredump转储文件>
```

### gdb命令

```
- continue    继续运行程序直到下一个断点（类似于VS里的F5）
- next        逐过程步进，不会进入子函数（类似VS里的F10）
- setp        逐语句步进，会进入子函数（类似VS里的F11）
- until        运行至当前语句块结束
- finish    运行至函数结束并跳出，并打印函数的返回值（类似VS的Shift+F11）
```

```
info thread 打印所有线程信息
- break <linespec> thread <threadno>
- break <linespec> thread <threadno> if ...

linespec指定了断点设置在的源程序的行号。threadno指定了线程的ID，注意，这个ID是GDB分配的，你可以通过"info threads"命令来查看正在运行程序中的线程信息。如果你不指定thread <threadno>则表示你的断点设在所有线程上面。你还可以为某线程指定断点条件。如：
```

```
 (gdb) break frik.c:13 thread 28 if bartab > lim
- break <function>    在进入指定函数时停住
- break <linenum>    在指定行号停住。
- break +/-offset    在当前行号的前面或后面的offset行停住。offiset为自然数。
- break filename:linenum    在源文件filename的linenum行处停住。
- break ... if <condition>    ...可以是上述的参数，condition表示条件，在条件成立时停住。比如在循环境体中，可以设置break if i=100，表示当i为100时停住程序。

可以通过info breakpoints [n]命令查看当前断点信息。此外，还有如下几个配套的常用命令：

- delete    删除所有断点
- delete breakpoint [n]    删除某个断点
- disable breakpoint [n]    禁用某个断点
- enable breakpoint [n]    使能某个断点
```

```
info source 查看源文件路径
disassemble 汇编码
```

### gcc -c
____
gcc -c -o xx.o xxx.c
-c: 不连接


### gcc、g++
____
g++ StaticClass.o  UseA.o  -o debug.o #自动使用c++库   
gcc StaticClass.o  UseA.o  -o debug.o -lstdc++


### gcc库搜索路径
____
gcc –L /usr/dev/mysql/lib –static –lmysqlclient test.o –o test   

静态库链接时搜索路径顺序：

1. ld会去找GCC命令中的参数-L
2. 再找gcc的环境变量LIBRARY_PATH
3. 再找内定目录 /lib /usr/lib /usr/local/lib 这是当初compile gcc时写在程序内的

动态链接时、执行时搜索路径顺序:

1. 编译目标代码时指定的动态库搜索路径
2. 环境变量LD_LIBRARY_PATH指定的动态库搜索路径
3. 配置文件/etc/ld.so.conf中指定的动态库搜索路径
4. 默认的动态库搜索路径/lib
5. 默认的动态库搜索路径/usr/lib

有关环境变量：   
LIBRARY_PATH环境变量：指定程序静态链接库文件搜索路径   
LD_LIBRARY_PATH环境变量：指定程序动态链接库文件搜索路径


### pthread、lpthread
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

