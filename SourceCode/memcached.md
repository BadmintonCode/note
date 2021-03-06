#Memcached
version:1.4.24
## 

![image](https://github.com/elwin0214/note/blob/master/SourceCode/memcached.jpg?raw=true)

## slab

#### 初始化
`main()`会调用`slabs_init()`初始化。如果是预先分配，会事先把需要占用的内存一次找OS malloc，否则在需要的时候再去malloc。 

```c
settings.item_size_max = 1024 * 1024;//slab内存页的大小。单位是字节，-I指定
settings.maxbytes = 64 * 1024 * 1024; //memcached能够使用的最大内存，-m指定  
settings.factor = 1.25; //item的扩容因子 ，-f指定
settings.chunk_size = 48; //最小的一个item能存储多少字节的数据(set、add命令中的数据)  
```


```c
//主要变量
static slabclass_t slabclass[MAX_NUMBER_OF_SLAB_CLASSES];
static size_t mem_limit = 0;
static size_t mem_malloced = 0;/*记录已经分配的内存量*/
static int power_largest;/*指向管理最大item的slabclass_t*/

static void *mem_base = NULL; /*预先分配时有意义，表示预先分配的内存的地址*/
static void *mem_current = NULL;/*预先分配时有意义，表示还可以使用的内存地址*/
static size_t mem_avail = 0; /*预先分配时有意义，表明还有多少内存可以使用*/
```

初始化的时候会计算每个slabclass_t中负责的item大小。
```c
unsigned int size = sizeof(item) + settings.chunk_size;//最小的item存储大小

while(...)
{
if (size % CHUNK_ALIGN_BYTES) size += CHUNK_ALIGN_BYTES - (size % CHUNK_ALIGN_BYTES);//对齐
slabclass[i].size = size;
slabclass[i].perslab = settings.item_size_max / slabclass[i].size;//计算item个数
size *= factor;
}
```

#### slab锁
这里涉及到2个锁，其中slab_lock，从slabclass_t中获取item，释放item时都会对它加锁。
```c
//slab.c
static pthread_mutex_t slabs_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t slabs_rebalance_lock = PTHREAD_MUTEX_INITIALIZER;
```

#### 分配item流程
获取item时候，先根据item大小定位到对应的`slabclass_t`，在成员`slots`对应的列表中查找，如果查找不到再申请分配内存。
分配到内存后，划分为item，放入slabclass_t中。

```c
//slab.c
//分配item
void *slabs_alloc(size_t size, unsigned int id, unsigned int *total_chunks) {
    void *ret;
    pthread_mutex_lock(&slabs_lock);
    ret = do_slabs_alloc(size, id, total_chunks);
    pthread_mutex_unlock(&slabs_lock);
    return ret;
}
//释放item
void slabs_free(void *ptr, size_t size, unsigned int id) {
    pthread_mutex_lock(&slabs_lock);
    do_slabs_free(ptr, size, id);
    pthread_mutex_unlock(&slabs_lock);
}
```

**注意**：item用来存储实际的数据，始终都由slab来管理，在LRU、HashTable里面存放的只是指向item的指针。

## LRU

#### 链表

链表按item大小分类。

```c

//items.c
static item *heads[LARGEST_ID];
static item *tails[LARGEST_ID];
static unsigned int sizes[LARGEST_ID];//LRU链表元素个数

```


####链表锁

```c
//thread.c
pthread_mutex_t lru_locks[POWER_LARGEST];
```
item_link_q 和 item_unlink_q 均会对链表加锁。
```
/*对LRU链接加锁，将item添加到LRU头部*/
static void item_link_q(item *it) {
    pthread_mutex_lock(&lru_locks[it->slabs_clsid]);
    do_item_link_q(it);
    pthread_mutex_unlock(&lru_locks[it->slabs_clsid]);
}
/*对LRU链接加锁，并移除item*/
static void item_unlink_q(item *it) {
    pthread_mutex_lock(&lru_locks[it->slabs_clsid]);
    do_item_unlink_q(it);
    pthread_mutex_unlock(&lru_locks[it->slabs_clsid]);
}
```

#### item过期处理

`do_item_get()` ：发现item失效、或者过期都会回收到slab中。   
`do_item_alloc()`：在配置`settings.lru_maintainer_thread=false`的情况下，执行流程如下：

* 1）从链表尾部开始，回收过期、失效的item到slab池中，循环5次；
* 2）调用`slab_alloc()` 分配，如果分配成功`return`返回，如果分配不成功，下一步；
* 3）从链表尾部开始，回收过期、失效的item，回收一个`continue;`；踢掉尾部元素（LRU，并不一定过期），踢掉一个`break;`，循环5次；

上述过程循环5次，直到能成功分配item。

**注意**：对于`exptime=0` 的item，不会过期，但是如果空间不够还是会被踢掉。
```c
//见lru_pull_tail()
settings.evict_to_free //如果设置为0 则不会踢掉 exptime=0 的item。
```

## 哈希表
#### 初始化
main()中调用assoc_init初始化hashtable，长度为`1<<settings.hashpower_init`。

#### 扩容

* `main()`会调用`start_assoc_maintenance_thread()`启动一个线程执行`assoc_maintenance_thread()`，线程会等待。
当调用`assoc_insert(item *it, const uint32_t hv)`发现item总数是hashtable长度的1.5倍时，调用`assoc_start_expand()`唤醒等待的线程。   
* 等待的线程唤醒后，调用`assoc_expand`分配一个大一倍的hashtable，然后对每个桶加锁迁移（不是一次迁移整个表，防止阻塞woker线程）。迁移由序号最小的桶开始。   
* 在迁移过程中，插入、查找、删除操作会判断当前key所对应的旧桶是"没有迁移"，还是"已经迁移"，分别在旧hashtable表或者新表中处理。
"正在迁移"的情况不会出现，因为woker线程会在对应的锁上等待。（加锁策略见下面）


```c
//assoc.c
//主要变量
unsigned int hashpower = HASHPOWER_DEFAULT; //默认16， 1<<hashpower决定了表的长度

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

static item** primary_hashtable = 0;
static item** old_hashtable = 0;
static unsigned int hash_items = 0;//hashtable中的元素数
static bool expanding = false;//标记是否正在迁移
static bool started_expanding = false;
static unsigned int expand_bucket = 0;//正在迁移的桶

 
//插入元素 新旧表的判断处理 int assoc_insert(item *it, const uint32_t hv) 
unsigned int oldbucket;
if (expanding &&
    (oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
{
    it->h_next = old_hashtable[oldbucket];
    old_hashtable[oldbucket] = it;
} else {
    it->h_next = primary_hashtable[hv & hashmask(hashpower)];
    primary_hashtable[hv & hashmask(hashpower)] = it;
}

// hash 扩容，一倍大小
static void assoc_expand(void) {
    old_hashtable = primary_hashtable;

    primary_hashtable = calloc(hashsize(hashpower + 1), sizeof(void *));
    if (primary_hashtable) {
        if (settings.verbose > 1)
            fprintf(stderr, "Hash table expansion starting\n");
        hashpower++;
        expanding = true;
        expand_bucket = 0;
        STATS_LOCK();
        stats.hash_power_level = hashpower;
        stats.hash_bytes += hashsize(hashpower) * sizeof(void *);
        stats.hash_is_expanding = 1;
        STATS_UNLOCK();
    } else {
        primary_hashtable = old_hashtable;
        /* Bad news, but we can keep running. */
    }
}

```

#### 哈希表锁
在woker线程初始化时，会根据线程数设定锁的数量。即使hashtable扩容，锁的数量不变，并且锁的数量小于hash桶的数量。
```c
//thread.c
//根据线程数定义锁的数量
if (nthreads < 3) {
    power = 10;
} else if (nthreads < 4) {
    power = 11;
} else if (nthreads < 5) {
    power = 12;
} else {
    power = 13;
}
if (power >= hashpower) { //
  fprintf(stderr, "Hash table power size (%d) cannot be equal to or less than item lock table (%d)\n", hashpower, power);
  fprintf(stderr, "Item lock table grows with `-t N` (worker threadcount)\n");
  fprintf(stderr, "Hash table grows with `-o hashpower=N` \n");
  exit(1);// 锁的数量不超过hash表的大小
}
...
item_lock_count = hashsize(power);
item_lock_hashpower = power;
for (i = 0; i < item_lock_count; i++) {//初始化hashtable锁
    pthread_mutex_init(&item_locks[i], NULL);
}
```

加锁时，根据key的hash值找到对应的lock加锁，`hv & hashmask(item_lock_hashpower)` 。 

```c
//thread.c
//worker线程操作如何使用锁
item *item_get(const char *key, const size_t nkey) {
    item *it;
    uint32_t hv;
    hv = hash(key, nkey);
    item_lock(hv);
    it = do_item_get(key, nkey, hv);
    item_unlock(hv);
    return it;
}
void item_lock(uint32_t hv) {
    mutex_lock(&item_locks[hv & hashmask(item_lock_hashpower)]);//将hv映射到其中一个lock
}

```

*  woker线程和迁移线程使用同样的锁，防止并发修改，桶与锁是多对一的关系     
*  woker线程访问桶中的某个key时，使用的锁和迁移该桶使用的是同一个锁     
*  同一个key在新旧表使用的是同一把锁，因为hash值不变


## 读写操作


一般在处理读请求时，为了性能考虑，并不是直接拷贝一份数据，而是直接传递item给connection处理，在这个过程中不会一直加锁，
为了防止item被delete命令删除掉，这里使用引用计数处理。在connection处理完数据写入后才会释放引用，最后判断是否无引用，进而删除item。


### GET
worker 线程检测到get请求后执行流程如下：
```c
void process_get_command(conn *c, token_t *tokens, size_t ntokens, bool return_cas)
->item *item_get(const char *key, const size_t nkey)  
    ->hv = hash(key, nkey);
    ->item_lock(hv);    
    ->it = do_item_get(key, nkey, hv);
        ->item *it = assoc_find(key, nkey, hv);
        ->if(NULL != it)
            ->refcount_incr(&it->refcount);//增加引用计数
        ->如果isflushed或者过期
            ->do_item_unlink(it, hv);//在hashtable中删除item，LRU中移除item，调用do_item_remove()。
            ->do_item_remove(it);//引用减1，如果等于0，还回到slab
        ->否则
            ->it->it_flags |= ITEM_FETCHED|ITEM_ACTIVE;
    ->item_unlock(hv); 
->void item_update(item *item)//上一步获取的item存在才会继续调用下去，更新访问时间，移动到LRU头部
    ->item_lock(hv);
    ->do_item_update(item);
        ->if (it->time < current_time - ITEM_UPDATE_INTERVAL) 
            ->if ((it->it_flags & ITEM_LINKED) != 0) //在LRU链表中
                ->it->time = current_time;
                ->if (!settings.lru_maintainer_thread) 
                    ->item_unlink_q(it);
                        ->pthread_mutex_lock(&lru_locks[it->slabs_clsid]);
                        ->do_item_unlink_q(it);//从LRU中删除
                    ->item_link_q(it);
                        ->pthread_mutex_lock(&lru_locks[it->slabs_clsid]);
                        ->do_item_link_q(it);//添加到LRU数组中 
->conn_set_state(c, conn_mwrite);
```
drive_machine 中把数据写完到socket才释放item的引用。
```c
drive_machine(conn *c)
->switch (transmit(c)) {//发送数据给Client
    case TRANSMIT_COMPLETE:
        ->if (c->state == conn_mwrite) {  
            ->conn_release_items(c);
                ->item_remove(c->item);
```

### SET
worker 线程检测到set请求后执行流程如下：
为了避免数据拷贝，对SET的处理类似于GET请求。

```c  
void process_update_command(conn *c, token_t *tokens, const size_t ntokens, int comm, bool handle_cas) 
->it = item_alloc(key, nkey, flags, realtime(exptime), vlen);
->conn_set_state(c, conn_nread);
```

```c
item *item_alloc(char *key, size_t nkey, int flags, rel_time_t exptime, int nbytes) 
->item *do_item_alloc(char *key, const size_t nkey, const int flags,
                   const rel_time_t exptime, const int nbytes,
                    const uint32_t cur_hv)
    ->void *slabs_alloc(size_t size, unsigned int id, unsigned int *total_chunks) 
        ->pthread_mutex_lock(&slabs_lock);
        ->ret = do_slabs_alloc(size, id, total_chunks);
            ->如果slab中的item不够，调用do_slabs_newslab(id)
            ->如果slab中的item足够，拿出一个item
                ->it->it_flags &= ~ITEM_SLABBED;
                ->it->refcount = 1;//引用计数设置为1
        ->pthread_mutex_unlock(&slabs_lock);   
```


数据读完以后再调用 complete_nread_ascii，如下代码，即使当前set的item正在被某个client读取，这里并没有把item删除掉
，而是在新的item放入LRU之前，把旧的item在hashtable、LRU链表中删除。
```c
void complete_nread_ascii(conn *c)
->item *it = c->item;
->int comm = c->cmd;
->ret = store_item(it, comm, c);
    ->item_lock(hv);
    ->ret = do_store_item(item, comm, c, hv);
        ->item *old_it = do_item_get(key, it->nkey, hv);
        ->if (old_it != NULL) item_replace(old_it, it, hv);//将旧item从LRU和hashtable中移除，新的item添加到LRU和hashtable
        ->else do_item_link(it, hv)//在hashtable、LRU中添加item，计数加1.
        -> if (old_it != NULL) do_item_remove(old_it);        
        -> if (new_it != NULL) do_item_remove(new_it);  
    ->item_unlock(hv);
->item_remove(c->item); 

```     

##UDP

对UDP的处理与TCP处理不一样，UDP无连接概念，对于本地建立的fd，会dup出多个来分发到不同的线程，（如下：）
猜测同一个UDP包，在多线程环境下，只能有一个FD可以收到，这样就不会有并发的问题。

```c
//memcached.cc
static int server_socket(const char *interface, int port,
                         enum network_transport transport,
                         FILE *portnumber_file)
if (IS_UDP(transport)) {
    int c;

    for (c = 0; c < settings.num_threads_per_udp; c++) {
        /* Allocate one UDP file descriptor per worker thread;
         * this allows "stats conns" to separately list multiple
         * parallel UDP requests in progress.
         *
         * The dispatch code round-robins new connection requests
         * among threads, so this is guaranteed to assign one
         * FD to each thread.
         */
        int per_thread_fd = c ? dup(sfd) : sfd;
        dispatch_conn_new(per_thread_fd, conn_read,
                          EV_READ | EV_PERSIST,
                          UDP_READ_BUFFER_SIZE, transport);
    }
}                          

```
如果一个应用数据，需要拆分成多个UDP包发送的话，如何组装的问题？可以看看memcached 协议说明。

```
//https://github.com/memcached/memcached/blob/master/doc/protocol.txt
In the current implementation, requests must be contained in a single UDP datagram, but
responses may span several datagrams. 

The frame header is 8 bytes long, as follows (all values are 16-bit integers
in network byte order, high byte first):

0-1 Request ID
2-3 Sequence number
4-5 Total number of datagrams in this message
6-7 Reserved for future use; must be 0
```
memcached 的connection 里面有一个字段表示的是当前的请求ID，在回复的时候，可以分多个UDP包回复。
```c
//memcached.cc
static enum try_read_result try_read_udp(conn *c) 

c->request_id = buf[0] * 256 + buf[1];
```