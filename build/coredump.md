#ubuntu、mac 处理 coredump

######取消限制
ulimit -c unlimited


mac下面生成放在目录 ~/Library/Logs/DiagnosticReports
取消隐藏文件defaults write com.apple.finder AppleShowAllFiles TRUE

程序没有权限不会生成

chown -R 账号名  文件或者目录

chgrp -R 账号名  文件或者目录


验证是否是core文件： 
readelf -h core文件   查看type字段
或者 file  core文件

gdb 查看core：
代码需要重新-g编译

gdb 程序文件 core文件

bt 查看堆栈



