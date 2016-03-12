
######pthread_create
```c
#include <pthread.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
```

######pthread_join


int pthread_join(pthread_t thread, void **retval);
```
retval:保存退出状态，比如线程调用pthread_exit() 或者 pthread_cancel()   
多个线程同时join一个线程，未定义   
发起join的线程被取消后，被join的线程仍然是可以被join的   
http://man7.org/linux/man-pages/man3/pthread_join.3.html   

```


######pthread_join /pthread_detach

```
一个线程要么是joinable 或者 detached。   
一个joinable的线程，可以被别的线程调用 pthread_join   
detached 线程的资源自动释放，不能去join一个detached的线程以获取状态   
一个detached的线程可以有daemon线程的功能   
一个新创建的线程是joinable的状态，除非修改attr（pthread_attr_setdetachstate()）   
如果不调用这2个函数中的一个，资源没办法回收   
http://man7.org/linux/man-pages/man3/pthread_create.3.html
http://stackoverflow.com/questions/24816518/not-using-pthread-detach-or-pthread-join-will-not-clean-up-the-resources-for-ot
```
