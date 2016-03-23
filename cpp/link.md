mac下
brew install binutils

gobjdump
otools

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
gcc main.c -L. -ltest -omain  


objdump -h xxx.o
gobjdump -h xxx.o

readelf -h xx.o
readelf -S xx.o :段信息

gobjdump -d -j .text StaticClass.o : text 段汇编


函数调用过程
http://hutaow.com/blog/2013/10/15/dump-stack/
http://www.findfunaax.com/notes/file/262


ld -r -s -o output.o origin1.o origin2.o
参数-r表示输出可重定位的文件（新生成的.o可以再次做为ld的输入），-s表示忽略输出文件中的符号信息；另外output.o是新生成的.o文件，origin1.o、origin2.o为原始的.o文件，还可以加上更多，或者直接*.o也可以。



头文件不能定义变量，只要被某个模块include 就会定义一个，多个模块连接的时候会冲突。
类A使用类B,类B使用类A, 头文件都用 class A|B 声明，具体cpp文件再include 需要用到的类