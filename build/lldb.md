##lldb

#### 安装
```
 sudo apt-get install lldb-3.5   
 alias lldb="lldb-3.5"  #~/.bashrc
```

#### coredump设置
```
ulimit -a 
ulimit -c unlimited
/cores/目录下

lldb -c  [corefile]
```
#### attach
```
 (lldb) process attach --pid 9939  
 (lldb) process attach --name Safari


  r # 第一次run
  c # 设置断点等参数后继续运行
```

#### 查看代码

```
 l|list  
 l|list $line  回到第几行
 list $funcname  
 list $filename  好像文件名不用包含路径
```

#### 断点

```
  breakpoint set --file $file --line $line
  breakpoint set --name $funcname  #c函数
  breakpoint set --method $funcname #c++ 方法
  breakpoint set -n "-[SKTGraphicView alignLeftEdges:]"  #根据函数调用语句下断点


  breakpoint list
  breakpoint disable $id
  breakpoint enable $id
  breakpoint delete $id
```
#### 运行
```
  run
  next | n
  step | s   #进入
  finish     #步出
```

查看遍历
```
   bt  打印当前栈帧
   po  输出指针指向的对象
   p   输出基础变量
   frame select [number] # 使用bt 后跳转到某一帧
   frame variable [var] #查看某个变量，支持指针操作
   frame variable   #查看所有帧变量
```
#### 查看变量
```
    frame variable this->root
    (simple::TreeNode<int> *) this->root = 0x0000000100104b80
```

#### watch 
```
 # 找到一个可以访问的变量，设置即可
 watch set var global
 watch modify -c '(global==5)'
 watch list
```


#### thread

```
thread list
thread select 2
```

#### clang

```
clang++ Hello.cpp -o Hello.out  -std=c++11 -stdlib=libc++ -g
```