##ubuntu、mac 处理 coredump

#### 生成

* 取消限制：ulimit -c unlimited
* mac下面生成放在目录 ~/Library/Logs/DiagnosticReports
* mac取消隐藏文件defaults write com.apple.finder AppleShowAllFiles TRUE
* 程序没有权限不会生成`chown -R 账号名  文件或者目录`,`chgrp -R 账号名  文件或者目录`


#### 验证是否是core文件：

```
readelf -h [core文件]        
查看type字段 或者 file [core文件]
```

#### gdb 查看core：


* gdb [程序可执行文件]    [core文件]
* bt 查看堆栈
  如果需要更详细的信息，代码需要重新-g编译，如下这些命令好像需要 -g 才可以用。

```
bt: 打印程序崩溃时的堆栈信息，包括导致crash的对应的frame number，文件名和行数
frame [frame number]: 打印出该行代码
info locals: 打印局部变量信息
print [local variable]: 打印某个局部变量
list: 打印相关代码
quit: 退出gdb命令行
```




