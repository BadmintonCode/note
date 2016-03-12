##网络API

##### htonx()/ntohx()

字节序转换

```c
#include <netinet/in.h>
uint16_t htons(uint16_t host16bitvalue);//主机到网络
uint32_t htonl(uint32_t host32bitvalue);
uint16_t ntohs(uint16_t net16bitvalue);
uint32_t ntohl(uint32_t net32bitvalue);
```


##### inet_ntop()/inet_pton()

```c
#include <arpa/inet.h>
const char *inet_ntop(int af, const void *src,char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);
```

```
inet_ntop()
inet_ntop() takes the address family in the af parameter (either AF_INET or AF_INET6).
The src parameter should be a pointer to either a struct in_addr or struct in6_addr
containing the address you wish to convert to a string. Finally dst and size are 
the pointer to the destination string and the maximum length of that string.
What should the maximum length of the dst string be? What is the maximum length for 
IPv4 and IPv6 addresses? Fortunately there are a couple of macros to help you out.
The maximum lengths are: INET_ADDRSTRLEN and INET6_ADDRSTRLEN.

inet_pton()
The src parameter is a pointer to a string containing the IP address in printable form.
Lastly the dst parameter points to where the result should be stored, which is 
probably a struct in_addr or struct in6_addr.

```



##### struct sockaddr_in/struct sockaddr

sockaddr大小和 sockaddr_in一样，可以相互转换。

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


```c
#include <linux/socket.h>
struct sockaddr 
{
    unsigned short sa_family; /* address family, AF_xxx */
    char sa_data[14]; /* 14 bytes of protocol address */
};
```

Example:

```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(9996);
const char *host = "127.0.0.1";
inet_pton(AF_INET, host, &(server_addr.sin_addr));

char client_host[32];
inet_ntop(AF_INET, &(client_addr.sin_addr), client_host, sizeof(client_host));
```


##### socket()


```c
#include <sys/types.h>
#include <sys/socket.h>
int socket(int domain, int type, int protocol);//-1 表示error
```

##### bind()
```c
#include <sys/types.h>
#include <sys/socket.h>
int bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen); 
```


##### listen()

```c
#include <sys/socket.h>
int listen(int s, int backlog);  //-1 error
```

##### accept()
```c
#include <sys/types.h>
#include <sys/socket.h>
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);//-1 error
```

##### connect()

```c
#include <sys/types.h>
#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);//-1 error
```


##### close()

```c
#include <unistd.h>
int close(int s);//0-succ -1-error

```


##### select()

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


##### read()/write()
针对文件描述符

```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fildes, const void *buf, size_t nbyte);
```


##### recv()、send()
针对socket

```c
#include <sys/types.h>
#include <sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);

```


##### bzero()

```c
#include <strings.h>
void bzero(void *s, size_t n);
```
###### snprintf()

```c
#include <stdio.h>
int snprintf(char * restrict str, size_t size, const char * restrict format,...);
```

```
最多写入size - 1 个字符，第size 个字符为 '\0'   
返回值表示实际要写入在字符串，不包含'\0',如果返回值 >= size 表示 要写入的字符串偏长，超出部分被丢弃。
```

##### eventfd 

```
类似于socket pair
```