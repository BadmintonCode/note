# libevent 源码分析

version:1.4.14b

## IO事件处理

### 数据结构

#### eventop
定义如下，该结构体定义了一类事件操作的接口。
具体的实现依赖于各个平台，目前有select、epoll等。全部放在了一个静态全局数组eventops中。  

>event-internal.h

```c
struct eventop {
    const char *name;
    void *(*init)(struct event_base *);   //初始化
    int (*add)(void *, struct event *);    //注册
    int (*del)(void *, struct event *);    //删除
    int (*dispatch)(struct event_base *, void *, struct timeval *); //事件分发
    void (*dealloc)(struct event_base *, void *);  //释放资源
    int need_reinit;/* set if we need to reinitialize the event base */
};
```

>event.c
```c
static const struct eventop *eventops[] = {
#ifdef HAVE_EVENT_PORTS
    &evportops,
#endif
#ifdef HAVE_WORKING_KQUEUE
    &kqops,
#endif
#ifdef HAVE_EPOLL
    &epollops,
#endif
#ifdef HAVE_DEVPOLL
    &devpollops,
#endif
#ifdef HAVE_POLL
    &pollops,
#endif
#ifdef HAVE_SELECT
    &selectops,
#endif
#ifdef WIN32
    &win32ops,
#endif
    NULL
};
```



#### event_base

事件操作器，一个核心结构体，用来管理注册事件队列，就绪事件队列。

>event-internal.h
```c
struct event_base {
    const struct eventop *evsel;  //操作事件的接口
    void *evbase;
    int event_count;        /* counts number of total events */
    int event_count_active; /* counts number of active events */
    int event_gotterm;      /* Set to terminate loop */
    int event_break;        /* Set to terminate loop immediately */
    struct event_list **activequeues;
    int nactivequeues;
    struct evsignal_info sig;
    struct event_list eventqueue;
    struct timeval event_tv;
    struct min_heap timeheap;
    struct timeval tv_cache;
};
```
evsel:指向全局数组eventops中的一个元素。也就是事件操作接口。   
evbase:与具体的evsel实现相关的数据结构，保存对应的一些信息，比如：select对应的数据结构就是
```c
struct selectop {
    int event_fds;      /* Highest fd in fd set */
    int event_fdsz;
    fd_set *event_readset_in;
    fd_set *event_writeset_in;
    fd_set *event_readset_out;
    fd_set *event_writeset_out;
    struct event **event_r_by_fd;
    struct event **event_w_by_fd;
};
```
event_count_active:表示就绪的事件数，在event_queue_insert()中，将就绪事件添加到就绪队列时，会增加该计数。   
activequeues:事件二维列表，下标是优先级，按优先级保存就绪的事件。   
eventqueue:保存所有的注册事件。   

#### event

用来描述关注的事件。

```c
struct event {
    TAILQ_ENTRY (event) ev_next;
    TAILQ_ENTRY (event) ev_active_next;
    TAILQ_ENTRY (event) ev_signal_next;
    unsigned int min_heap_idx;  /* for managing timeouts */
    struct event_base *ev_base;  //所属event_base
    int ev_fd;  
    short ev_events;  //关注的事件
    short ev_ncalls;//表示事件就绪时，调用的次数，在event_active()中设置。
    short *ev_pncalls;  /* Allows deletes in callback */
    struct timeval ev_timeout;
    int ev_pri;     /* smaller numbers are higher priority */
    void (*ev_callback)(int, short, void *arg);
    void *ev_arg;
    int ev_res;     //当前就绪的事件类型，被当做参数传递给ev_callback，见event_process_active()
    int ev_flags;   //当前的状态
};
```
ev_events:表示关注的事件，可能的值有：
```c
#define EV_TIMEOUT  0x01
#define EV_READ     0x02   
#define EV_WRITE    0x04
#define EV_SIGNAL   0x08
//如果有设置，表示永久关注此事件，可以见event_process_active(),没有设置，只关注一次。
#define EV_PERSIST  0x10  

```
ev_flags:表明其当前的状态，可能的值有:

```c
#define EVLIST_TIMEOUT  0x01
#define EVLIST_INSERTED 0x02 //新事件注册会设置，见event_add()
#define EVLIST_SIGNAL   0x04
#define EVLIST_ACTIVE   0x08  //如果事件被添加到 就绪队列，会设置该值，见event_queue_insert()
#define EVLIST_INTERNAL 0x10
#define EVLIST_INIT 0x80      //被初始化，见event_set()
#define EVLIST_ALL  (0xf000 | 0x9f)  
```




###函数


#### event_init

初始化全局的current_base(在event.c中定义)。

```c
struct event_base * event_init(void)
{
    struct event_base *base = event_base_new();
    if (base != NULL)
        current_base = base;
    return (base);
}

```
#### event_base_new

创建一个 event_base。

```c
struct event_base *event_base_new(void)
{
    int i;
    struct event_base *base;
    if ((base = calloc(1, sizeof(struct event_base))) == NULL)
        event_err(1, "%s: calloc", __func__);
    event_sigcb = NULL;
    event_gotsig = 0;
    detect_monotonic();
    gettime(base, &base->event_tv);
    
    min_heap_ctor(&base->timeheap);
    TAILQ_INIT(&base->eventqueue);
    base->sig.ev_signal_pair[0] = -1;
    base->sig.ev_signal_pair[1] = -1;
    
    base->evbase = NULL;
    for (i = 0; eventops[i] && !base->evbase; i++) {//初始化 base->evbase，在eventops中选择
        base->evsel = eventops[i];  
        base->evbase = base->evsel->init(base);
    }
    if (base->evbase == NULL)
        event_errx(1, "%s: no event mechanism available", __func__);

    return (base);
}
```
#### event_base_free释放资源
```c
void event_base_free(struct event_base *base)
```

#### event_set

构造事件

```c
void event_set(struct event *ev, int fd, short events,
      void (*callback)(int, short, void *), void *arg)
{
    /* Take the current base - caller needs to set the real base later */
    ev->ev_base = current_base;

    ev->ev_callback = callback;
    ev->ev_arg = arg;
    ev->ev_fd = fd;
    ev->ev_events = events;
    ev->ev_res = 0;
    ev->ev_flags = EVLIST_INIT;
    ev->ev_ncalls = 0;
    ev->ev_pncalls = NULL;

    min_heap_elem_init(ev);

    /* by default, we put new events into the middle priority */
    if(current_base)
        ev->ev_pri = current_base->nactivequeues/2;

}
```


#### event_add

添加事件：调用底层事件接口关注事件，同时调用event_queue_insert()将事件添加到注册队列。

```c
int event_add(struct event *ev, const struct timeval *tv)
{
    struct event_base *base = ev->ev_base;  //event 需要预先指定event_base
    const struct eventop *evsel = base->evsel;
    void *evbase = base->evbase;
    int res = 0;
    //如果是 读、写、信号事件，并且没有添加到 已注册队列或者就绪队列，则添加。
    if ((ev->ev_events & (EV_READ|EV_WRITE|EV_SIGNAL)) &&
        !(ev->ev_flags & (EVLIST_INSERTED|EVLIST_ACTIVE))) {
        res = evsel->add(evbase, ev);
        if (res != -1)
            event_queue_insert(base, ev, EVLIST_INSERTED);//插入到注册队列
    }
    return (res);
}
```


#### event_del

删除事件：调用底层事件接口移除事件，同时调用event_queue_remove()移除就绪队列、注册事件队列中此事件。
```c
int event_del(struct event *ev)
{
    struct event_base *base;
    const struct eventop *evsel;
    void *evbase;

    /* An event without a base has not been added */
    if (ev->ev_base == NULL)
        return (-1);

    base = ev->ev_base;
    evsel = base->evsel;
    evbase = base->evbase;


    /* See if we are just active executing this event in a loop */
    if (ev->ev_ncalls && ev->ev_pncalls) {
        /* Abort loop */
        *ev->ev_pncalls = 0;
    }

    if (ev->ev_flags & EVLIST_TIMEOUT)
        event_queue_remove(base, ev, EVLIST_TIMEOUT);

    if (ev->ev_flags & EVLIST_ACTIVE)
        event_queue_remove(base, ev, EVLIST_ACTIVE);

    if (ev->ev_flags & EVLIST_INSERTED) {
        event_queue_remove(base, ev, EVLIST_INSERTED);
        return (evsel->del(evbase, ev));
    }

    return (0);
}
```


#### event_base_loop

事件轮询：
调用底层事件接口的dipatch()将已经就绪的事件添加到就绪队列，然后对就绪队列中每个事件调用event_process_active()
处理事件。
其中flag有2个参数。

```c
#define EVLOOP_ONCE 0x01    //如果某次循环有就绪事件，并且处理完，则直接退出
#define EVLOOP_NONBLOCK 0x02   //如果某次循环没有就绪事件，则直接退出
```

```c
int event_base_loop(struct event_base *base, int flags)
{
    const struct eventop *evsel = base->evsel;
    void *evbase = base->evbase;
    struct timeval tv;
    struct timeval *tv_p;
    int res, done;
    done = 0;
    while (!done) {
        /* update last old time */
        gettime(base, &base->event_tv);
        /* clear time cache */
        base->tv_cache.tv_sec = 0;
        res = evsel->dispatch(base, evbase, tv_p);//把已经就绪的事件添加到就绪队列中


        if (base->event_count_active) {// 
            event_process_active(base);//处理就绪事件
            if (!base->event_count_active && (flags & EVLOOP_ONCE))
            //event_count_active == 0 表示事件处理完
                done = 1;
        } else if (flags & EVLOOP_NONBLOCK)
            done = 1;
    }

    return (0);
}
```


底层事件接口的dispatch操作如下，这里选择的是select.c，主要选择就绪事件。

```c
static int
select_dispatch(struct event_base *base, void *arg, struct timeval *tv)
{
    
    for (j = 0; j <= sop->event_fds; ++j) {
        struct event *r_ev = NULL, *w_ev = NULL;
        if (++i >= sop->event_fds+1)
            i = 0;

        res = 0;
        if (FD_ISSET(i, sop->event_readset_out)) {
            r_ev = sop->event_r_by_fd[i];//读事件
            res |= EV_READ;
        }
        if (FD_ISSET(i, sop->event_writeset_out)) {
            w_ev = sop->event_w_by_fd[i];//写事件
            res |= EV_WRITE;
        }
        //把写就绪事件放入队列
        if (r_ev && (res & r_ev->ev_events)) {
            event_active(r_ev, res & r_ev->ev_events, 1);
        }
        //如果写就绪事件w_ev与读就绪事件r_ev不是同一个事件，也需要放入队列
        if (w_ev && w_ev != r_ev && (res & w_ev->ev_events)) {
            event_active(w_ev, res & w_ev->ev_events, 1);
        }
    }
    check_selectop(sop);

    return (0);
}
```


event_active()设置就绪事件的ev_res(表示就绪的事件类型)成员，把事件添加到就绪队列。

```
void event_active(struct event *ev, int res, short ncalls)
{
    /* We get different kinds of events, add them together */
    if (ev->ev_flags & EVLIST_ACTIVE) {//已经在就绪事件队列里面，则不处理。
        ev->ev_res |= res;
        return;
    }

    ev->ev_res = res;//就绪的读写事件
    ev->ev_ncalls = ncalls;
    ev->ev_pncalls = NULL;
    event_queue_insert(ev->ev_base, ev, EVLIST_ACTIVE);
}

```


event_queue_insert() 用于将就绪事件添加到队列。

```c
void event_queue_insert(struct event_base *base, struct event *ev, int queue)
{
    if (ev->ev_flags & queue) {
        /* Double insertion is possible for active events */
        if (queue & EVLIST_ACTIVE)
            return;

        event_errx(1, "%s: %p(fd %d) already on queue %x", __func__,
               ev, ev->ev_fd, queue);
    }

    if (~ev->ev_flags & EVLIST_INTERNAL)
        base->event_count++;

    ev->ev_flags |= queue;
    switch (queue) {
    case EVLIST_INSERTED:
        TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
        break;
    case EVLIST_ACTIVE:
        base->event_count_active++;
        TAILQ_INSERT_TAIL(base->activequeues[ev->ev_pri],
            ev,ev_active_next);
        break;
    case EVLIST_TIMEOUT: {
        min_heap_push(&base->timeheap, ev);
        break;
    }
    default:
        event_errx(1, "%s: unknown queue %x", __func__, queue);
    }
}
```

event_process_active() 用于触发激活事件上的回调。

```c
static void event_process_active(struct event_base *base)
{
    struct event *ev;
    struct event_list *activeq = NULL;
    int i;
    short ncalls;

    for (i = 0; i < base->nactivequeues; ++i) {
        if (TAILQ_FIRST(base->activequeues[i]) != NULL) {
            activeq = base->activequeues[i];
            break;
        }
    }
    for (ev = TAILQ_FIRST(activeq); ev; ev = TAILQ_FIRST(activeq)) {
        if (ev->ev_events & EV_PERSIST)
            event_queue_remove(base, ev, EVLIST_ACTIVE);
        else
            event_del(ev);
        
        /* Allows deletes to work */
        ncalls = ev->ev_ncalls;
        ev->ev_pncalls = &ncalls;
        while (ncalls) {
            ncalls--;
            ev->ev_ncalls = ncalls;
            (*ev->ev_callback)((int)ev->ev_fd, ev->ev_res, ev->ev_arg);//事件就绪时候的回调
            if (event_gotsig || base->event_break)
                return;
        }
    }
}
```


## 信号事件处理

### 数据结构

```c
struct evsignal_info {
    struct event ev_signal;
    int ev_signal_pair[2];
    int ev_signal_added;
    volatile sig_atomic_t evsignal_caught;
    struct event_list evsigevents[NSIG];
    sig_atomic_t evsigcaught[NSIG];
#ifdef HAVE_SIGACTION
    struct sigaction **sh_old;
#else
    ev_sighandler_t **sh_old;
#endif
    int sh_old_max;
};
```
