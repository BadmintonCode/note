
#ELF


工具
```
linux
objdump
readelf

mac(brew install binutils)
otool -tV xxx.o
gobjdump -d -j .text xxx.o
```

查看依赖的动态链接库   

```
readelf -d libleveldb.so #-d Display the dynamic section
-
Dynamic section at offset 0x53cf0 contains 26 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libtcmalloc.so.4]
 0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libgcc_s.so.1]
 0x0000000000000001 (NEEDED)             Shared library: [libpthread.so.0]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000000e (SONAME)             Library soname: [libleveldb.so.1]  //so名字
```

查看链接器路径
``` 
 readelf -x .interp memcached   #-x Dump the contents of section

 Hex dump of section '.interp':
  0x00400238 2f6c6962 36342f6c 642d6c69 6e75782d /lib64/ld-linux-
  0x00400248 7838362d 36342e73 6f2e3200          x86-64.so.2.
```

```
  readelf -x .interp memcached #-S --section-headers   Display the sections' header
 Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .interp           PROGBITS         0000000000400238  00000238
```


```
段：
.text：已编译程序的机器代码。
.rodata：只读数据，比如printf语句中的格式串和开关（switch）语句的跳转表。
.data：已初始化的全局C变量。局部C变量在运行时被保存在栈中，既不出现在.data中，也不出现在.bss节中。
.bss：未初始化的全局C变量。在目标文件中这个节不占据实际的空间，它仅仅是一个占位符。目标文件格式区分初始化和未初始化变量是为了空间效率在：在目标文件中，未初始化变量不需要占据任何实际的磁盘空间。
.symtab：一个符号表（symbol table），它存放在程序中被定义和引用的函数和全局变量的信息。一些程序员错误地认为必须通过-g选项来编译一个程序，得到符号表信息。实际上，每个可重定位目标文件在.symtab中都有一张符号表。然而，和编译器中的符号表不同，.symtab符号表不包含局部变量的表目。
.rel.text：当链接噐把这个目标文件和其他文件结合时，.text节中的许多位置都需要修改。一般而言，任何调用外部函数或者引用全局变量的指令都需要修改。另一方面调用本地函数的指令则不需要修改。注意，可执行目标文件中并不需要重定位信息，因此通常省略，除非使用者显式地指示链接器包含这些信息。
.rel.data：被模块定义或引用的任何全局变量的信息。一般而言，任何已初始化全局变量的初始值是全局变量或者外部定义函数的地址都需要被修改。
.debug：一个调试符号表，其有些表目是程序中定义的局部变量和类型定义，有些表目是程序中定义和引用的全局变量，有些是原始的C源文件。只有以-g选项调用编译驱动程序时，才会得到这张表。
.line：原始C源程序中的行号和.text节中机器指令之间的映射。只有以-g选项调用编译驱动程序时，才会得到这张表。
.strtab：一个字符串表，其内容包括.symtab和.debug节中的符号表，以及节头部中的节名字。字符串表就是以null结尾的字符串序列。


三种文件：

nm xx.o
1) 可重定位的对象文件(Relocatable file)
    .o文件，链接处理后可以生成可执行文件，或者可被共享文件，可以被ar归档成.a静态库文件。
2) 可执行的对象文件(Executable file)

3) 可被共享的对象文件(Shared object file)
    用来做链接处理生成别的 Shared object file 或者 executable file
    运行时，动态链接器拿来和 executable file 一起处理

gcc -fPIC -shared testa.c testb.c testc.c -o libtest.so  
-fPIC：表示编译为位置独立的代码，不用此选项的话编译后的代码是位置相关的所以动态载入时是通过代码拷贝的方式来满足不同进程的需要，而不能达到真正代码段共享的目的。

```
