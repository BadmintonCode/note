##网络API

####htonx()、ntohx()
_____
字节序转换
```c
#include <netinet/in.h>
uint16_t htons(uint16_t host16bitvalue);//主机到网络
uint32_t htonl(uint32_t host32bitvalue);
uint16_t ntohs(uint16_t net16bitvalue);
uint32_t ntohl(uint32_t net32bitvalue);
```


####inet_addr()
_____
字符串IP地址转换为网络字节序的二进制地址
```c
#include <arpa/inet.h>
in_addr_t inet_addr(const char *str)
```

####struct sockaddr_in
_____
```c
#include <netinet/in.h>
struct sockaddr_in
{
    short sin_family;
    unsigned short sin_port;//网络字节序，htons 转换
    struct in_addr sin_addr;//网络字节序
    unsigned char sin_zero[8];//对齐字段
};
typedef uint32_t in_addr_t;
struct in_addr
{
    in_addr_t s_addr;
};
```
######struct sockaddr

>sockaddr大小和 sockaddr_in一样，可以相互转换。
```
#include <linux/socket.h>
struct sockaddr 
{
    unsigned short sa_family; /* address family, AF_xxx */
    char sa_data[14]; /* 14 bytes of protocol address */
};
```

######socket()

```c
#include <sys/types.h>
#include <sys/socket.h>
int socket(int domain, int type, int protocol);//-1 表示error
```

######bind()
```c
#include <sys/types.h>
#include <sys/socket.h>
int bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen); 
```


######listen()

```c
#include <sys/socket.h>
int listen(int s, int backlog);  //-1 error
```

######accept()
```c
#include <sys/types.h>
#include <sys/socket.h>
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);//-1 error
```

######connect()

```c
#include <sys/types.h>
#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);//-1 error
```


######close()

```c
#include <unistd.h>
int close(int s);//0-succ -1-error

```


######select()


```c
#include <sys/select.h>

void FD_CLR(fd, fd_set *fdset);

void FD_COPY(fd_set *fdset_orig, fd_set *fdset_copy);

int FD_ISSET(fd, fd_set *fdset);

void FD_SET(fd, fd_set *fdset);

void FD_ZERO(fd_set *fdset);

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
         fd_set *restrict errorfds, struct timeval *restrict timeout);
```


######read()、write()
>针对文件描述符
```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fildes, const void *buf, size_t nbyte);
```


######recv()、send()
>针对socket
```c
#include <sys/types.h>
#include <sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);

```


###### bzero()

```c
#include <strings.h>
void bzero(void *s, size_t n);
```


>最多写入size - 1 个字符，第size 个字符为 '\0'
>返回值表示实际要写入在字符串，不包含'\0',如果返回值 >= size 表示 要写入的字符串偏长，超出部分被丢弃。

```c
#include <stdio.h>
 int snprintf(char * restrict str, size_t size, const char * restrict format,
         ...);


```