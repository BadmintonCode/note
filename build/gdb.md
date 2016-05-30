#GDB

##core

```
ulimit -c unlimited  或者 ulimit -c 字节数

vi /etc/sysctl.conf
kernel.core_pattern = /var/core/core_%e_%p  或者 ./core_%e_%p (当前目录)
kernel.core_uses_pid = 0
sysctl –p # 有的地方说这里带上文件名，但是测试过ubuntu不行
gdb <程序可执行文件> <coredump转储文件>
直接bt即可  （在mac上lldb测试不能直接bt）


使用 kill  -s SIGSEGV <pid> 给一个信号处理程序发送 信号 也可以产生core
```

##gdb 启动
```
因为指定了 <program>包含了符号表，所以可以通过list切换文件。
gdb <program>
gdb <program> <corefile>
gdb <program> <pid>

gdb -q a
(gdb) attach <pid>
(gdb) bt
(gdb) detach

gdb -q a <pid>
# https://github.com/hellogcc/100-gdb-tips/blob/master/src/attach-process.md
```

## 源码路径
```
(gdb) dir <directory>     添加path
(gdb) show dir            显示添加的path
(gdb) dir                 清除添加的path

-d -directory <directory>  启动时，带源文件搜索目录

在一个调试会话中，GDB维护了一个源代码查找目录列表，默认值是编译目录和当前工作目录。
当GDB需要一个源文件的时候，它依次在这些目录中查找，直到找到一个或者抛出错误。

info source 查看源文件路径
file <file> 加载并读取符号表
exec-file <file> 加载不读取符号表
disassemble 汇编码
```


```
continue    继续运行程序直到下一个断点（类似于VS里的F5）
next        逐过程步进，不会进入子函数（类似VS里的F10）
setp        逐语句步进，会进入子函数（类似VS里的F11）
until        运行至当前语句块结束
finish    运行至函数结束并跳出，并打印函数的返回值（类似VS的Shift+F11）
```

## 多线程
```
info thread
thread <ID>
break <linespec> thread <threadno>
break <linespec> thread <threadno> if ...

linespec指定了断点设置在的源程序的行号。threadno指定了线程的ID，注意，这个ID是GDB分配的，你可以通过"info threads"命令来查看
正在运行程序中的线程信息。如果你不指定thread <threadno>则表示你的断点设在所有线程上面。你还可以为某线程指定断点条件。
如：(gdb) break frik.c:13 thread 28 if bartab > lim

set scheduler-locking off|on|step
在使用step或者continue命令调试当前被调试线程的时候，其他线程也是同时执行
的，怎么只让被调试程序执行呢？通过这个命令就可以实现这个需求。

- off 不锁定任何线程，也就是所有线程都执行，这是默认值。
- on 只有当前被调试程序会执行。
- step 在单步的时候，除了next过一个函数的情况(熟悉情况的人可能知道，这其实是一个设置断点然后continue的行为)以外，只有当前线程会执行。
```
##断点
```
break <function>    在进入指定函数时停住
break <linenum>    在指定行号停住。
break +/-offset    在当前行号的前面或后面的offset行停住。offiset为自然数。
break filename:linenum    在源文件filename的linenum行处停住。
break ... if <condition>    ...可以是上述的参数，condition表示条件，在条件成立时停住。比如在循环境体中，可以设置break if i=100，表示当i为100时停住程序。

可以通过info breakpoints [n]命令查看当前断点信息。此外，还有如下几个配套的常用命令：

delete    删除所有断点
delete breakpoint [n]    删除某个断点
disable breakpoint [n]    禁用某个断点
enable breakpoint [n]    使能某个断点

```
## ELF
```
不管是中间的某个o文件，还是最后链接以后的文件，都可以通过下面的命令发现路径信息
# readelf -p .debug_str ../eventloop.o    |fgrep eventloop
  [  4850]  eventloop.cc
# readelf -p .debug_str ../eventloop.o    |fgrep <path>
  [  2123]  /home/<path>/src

http://blog.csdn.net/jiafu1115/article/details/31790757
https://sourceware.org/gdb/onlinedocs/gdb/Source-Path.html
```